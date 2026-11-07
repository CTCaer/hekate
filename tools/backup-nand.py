#!/usr/bin/env python3
r"""Back up read-only hekate eMMC exports on Windows. No pip packages needed.

Run as Administrator:
    python tools/backup-nand.py -OutputDirectory D:\SwitchBackup
    python tools/backup-nand.py -Part GPP -EjectOnly
"""

import argparse
import ctypes
from ctypes import wintypes
from dataclasses import dataclass
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time


@dataclass(frozen=True)
class NandUsbDisk:
    path: str
    pnp_device_id: str
    size: int
    serial: str


# Use the same Windows providers as the PowerShell helper. No user input is
# interpolated into this command; Python validates and selects the partition.
INVENTORY_SCRIPT = r"""
$ErrorActionPreference = 'Stop'
[Console]::OutputEncoding = [Text.UTF8Encoding]::new($false)
$items = @(Get-CimInstance Win32_DiskDrive | Where-Object {
    $_.Model -match '^hekate\s+eMMC\s+(BOOT0|BOOT1|GPP)(?:\s|$)'
} | ForEach-Object {
    $device = $_
    $disk = $null
    try { $disk = Get-Disk -Number $device.Index -ErrorAction Stop }
    catch { if ($_.CategoryInfo.Category -ne 'ObjectNotFound') { throw } }
    [pscustomobject]@{
        Model = $device.Model
        SerialNumber = $device.SerialNumber
        Path = $device.DeviceID
        PnpDeviceId = $device.PNPDeviceID
        Ready = ($null -ne $disk)
        Size = $disk.Size
        IsReadOnly = $disk.IsReadOnly
        IsBoot = $disk.IsBoot
        IsSystem = $disk.IsSystem
    }
})
ConvertTo-Json -InputObject $items -Compress
"""


def get_disk_inventory():
    powershell = Path(os.environ['SystemRoot']) / 'System32/WindowsPowerShell/v1.0/powershell.exe'
    result = subprocess.run(
        [str(powershell), '-NoProfile', '-NonInteractive', '-Command', INVENTORY_SCRIPT],
        capture_output=True, encoding='utf-8', errors='replace', check=False,
    )
    if result.returncode:
        raise RuntimeError('Windows disk discovery failed: ' + result.stderr.strip())
    return json.loads(result.stdout.lstrip('\ufeff'))


def get_nand_usb_disk(partition, expected_serial=None, allow_missing=False):
    if partition not in ('BOOT0', 'BOOT1', 'GPP'):
        raise ValueError('Invalid eMMC partition.')
    matches = [d for d in get_disk_inventory()
               if re.match(r'^hekate\s+eMMC\s+' + partition + r'(?:\s|$)',
                           d.get('Model') or '', re.I)]
    if allow_missing and not matches:
        return None
    if len(matches) != 1:
        raise RuntimeError(f'Expected exactly one hekate eMMC {partition} USB disk; found {len(matches)}.')
    device = matches[0]
    identity = re.fullmatch(r'([0-9a-f]{4,8})\s+eMMC\s+' + partition,
                            (device.get('SerialNumber') or '').strip(), re.I)
    if not identity:
        raise RuntimeError('Cannot identify the eMMC serial from the USB disk.')
    serial = identity[1].upper()
    if expected_serial and serial != expected_serial.upper():
        raise RuntimeError('This eMMC belongs to a different console. Keep all three parts from the same console.')
    if not device['Ready']:
        if allow_missing:
            return None
        raise RuntimeError('The USB disk is not registered yet. Reconnect the export and retry.')
    if not device['IsReadOnly']:
        raise RuntimeError('Enable Read-Only in hekate USB Tools, then reconnect the export.')
    if device['IsBoot'] or device['IsSystem']:
        raise RuntimeError('Refusing to read a Windows boot/system disk.')
    size = int(device['Size'])
    if size <= 0 or size % 512:
        raise RuntimeError('Invalid USB disk size.')
    return NandUsbDisk(device['Path'], device['PnpDeviceId'], size, serial)


def wait_nand_usb_disk(partition, expected_serial=None):
    label = 'eMMC RAW GPP' if partition == 'GPP' else 'eMMC ' + partition
    print(f'Select {label} on the Switch. Waiting for automatic detection (Ctrl+C to stop)...')
    while True:
        disk = get_nand_usb_disk(partition, expected_serial, allow_missing=True)
        if disk is not None:
            print(f'Detected {label}. Starting backup.')
            return disk
        time.sleep(0.5)


def open_nand_source(path):
    if not re.fullmatch(r'\\\\\.\\PhysicalDrive[0-9]+', str(path), re.I):
        return open(path, 'rb', buffering=0)
    # Match FileShare.ReadWrite from the PowerShell version, while requesting
    # only read access. Transfer handle ownership to Python's file object.
    import msvcrt
    kernel = ctypes.WinDLL('kernel32', use_last_error=True)
    create = kernel.CreateFileW
    create.argtypes = [wintypes.LPCWSTR, wintypes.DWORD, wintypes.DWORD,
                       ctypes.c_void_p, wintypes.DWORD, wintypes.DWORD, wintypes.HANDLE]
    create.restype = wintypes.HANDLE
    handle = create(str(path), 0x80000000, 3, None, 3, 0, None)
    if handle == ctypes.c_void_p(-1).value:
        raise ctypes.WinError(ctypes.get_last_error())
    try:
        fd = msvcrt.open_osfhandle(handle, os.O_RDONLY | os.O_BINARY)
    except BaseException:
        kernel.CloseHandle.argtypes = [wintypes.HANDLE]
        kernel.CloseHandle(handle)
        raise
    try:
        return os.fdopen(fd, 'rb', buffering=0)
    except BaseException:
        os.close(fd)
        raise


def hash_file(path):
    digest = hashlib.sha256()
    with open(path, 'rb') as saved:
        for chunk in iter(lambda: saved.read(1024 * 1024), b''):
            digest.update(chunk)
    return digest.hexdigest()


def copy_nand_image(source_path, size, destination):
    if size <= 0 or size % 512:
        raise ValueError('Invalid image size; expected complete 512-byte sectors.')
    destination = Path(destination)
    partial = Path(str(destination) + '.partial')
    checksum = Path(str(destination) + '.sha256')
    for path in (destination, partial, checksum):
        if path.exists():
            raise FileExistsError(f'File already exists: {path}')
    digest = hashlib.sha256()
    copied = 0
    last_progress = -1.0
    try:
        with open_nand_source(source_path) as source, partial.open('xb') as target:
            while copied < size:
                chunk = source.read(min(1024 * 1024, size - copied))
                if not chunk:
                    raise RuntimeError(f'USB source ended after {copied} of {size} bytes.')
                target.write(chunk)
                digest.update(chunk)
                copied += len(chunk)
                now = time.monotonic()
                if now - last_progress >= 0.5:
                    print(f'\rBacking up {destination.name}: {copied / 1048576:,.0f} / '
                          f'{size / 1048576:,.0f} MiB ({100 * copied / size:.0f}%)',
                          end='', file=sys.stderr, flush=True)
                    last_progress = now
            target.flush()
            os.fsync(target.fileno())
    finally:
        print(file=sys.stderr)
    print('Verifying saved backup...')
    expected = digest.hexdigest()
    if hash_file(partial) != expected:
        raise RuntimeError('Saved backup checksum does not match the USB data.')
    # On Windows rename fails if the destination exists, including a file
    # created since the initial checks. Keep the partial on any failure.
    os.rename(partial, destination)
    with checksum.open('x', encoding='ascii', newline='\n') as saved:
        saved.write(f'{expected}  {destination.name}\n')
    print(f'Saved and verified: {destination}')


def initialize_nand_usb_native():
    cfg = ctypes.WinDLL('cfgmgr32')
    cfg.CM_Locate_DevNodeW.argtypes = [ctypes.POINTER(wintypes.DWORD), wintypes.LPWSTR, wintypes.ULONG]
    cfg.CM_Get_Parent.argtypes = [ctypes.POINTER(wintypes.DWORD), wintypes.DWORD, wintypes.ULONG]
    cfg.CM_Get_Device_IDW.argtypes = [wintypes.DWORD, wintypes.LPWSTR, wintypes.ULONG, wintypes.ULONG]
    cfg.CM_Request_Device_EjectW.argtypes = [wintypes.DWORD, ctypes.POINTER(wintypes.DWORD),
                                           wintypes.LPWSTR, wintypes.ULONG, wintypes.ULONG]
    for name in ('CM_Locate_DevNodeW', 'CM_Get_Parent', 'CM_Get_Device_IDW', 'CM_Request_Device_EjectW'):
        getattr(cfg, name).restype = wintypes.DWORD
    return cfg


def request_nand_usb_eject(device_path, pnp_device_id):
    if not re.fullmatch(r'\\\\\.\\PhysicalDrive[0-9]+', device_path, re.I):
        raise ValueError('Expected a Windows physical disk path for eject.')
    if not pnp_device_id or not pnp_device_id.strip():
        raise ValueError('Missing USB disk PnP identity; removal was not requested.')
    cfg = initialize_nand_usb_native()
    node = wintypes.DWORD()
    result = cfg.CM_Locate_DevNodeW(ctypes.byref(node), pnp_device_id, 0)
    if result:
        raise RuntimeError(f'Cannot locate the USB disk (CONFIGRET {result}).')
    for _ in range(16):
        identity = ctypes.create_unicode_buffer(512)
        result = cfg.CM_Get_Device_IDW(node.value, identity, len(identity), 0)
        if result:
            raise RuntimeError(f'Cannot identify the USB parent (CONFIGRET {result}).')
        # Eject only the known hekate USB ancestor, never a hub or controller.
        if identity.value.upper().startswith('USB\\VID_11EC&PID_A7E0\\'):
            veto = wintypes.DWORD()
            veto_name = ctypes.create_unicode_buffer(512)
            result = cfg.CM_Request_Device_EjectW(node.value, ctypes.byref(veto), veto_name, len(veto_name), 0)
            if result:
                raise RuntimeError(f'Windows refused device removal (CONFIGRET {result}, '
                                   f'veto {veto.value}: {veto_name.value}). Close other disk users and retry.')
            return
        parent = wintypes.DWORD()
        if cfg.CM_Get_Parent(ctypes.byref(parent), node.value, 0):
            break
        node = parent
    raise RuntimeError('No matching hekate USB device found; removal was not requested.')


def eject_nand_usb_disk(partition, expected_disk):
    current = get_nand_usb_disk(partition, expected_disk.serial)
    if current != expected_disk:
        raise RuntimeError('The USB disk changed since backup started; eject was not requested.')
    request_nand_usb_eject(current.path, current.pnp_device_id)
    print('Windows accepted safe removal of the hekate USB device.')


def check_administrator():
    if os.name != 'nt':
        raise RuntimeError('This helper requires Windows.')
    shell = ctypes.WinDLL('shell32')
    shell.IsUserAnAdmin.argtypes = []
    shell.IsUserAnAdmin.restype = wintypes.BOOL
    if not shell.IsUserAnAdmin():
        raise RuntimeError('Run your terminal as Administrator to read the USB physical disk.')


def get_destination_filesystem(folder):
    kernel = ctypes.WinDLL('kernel32', use_last_error=True)
    kernel.GetVolumePathNameW.argtypes = [wintypes.LPCWSTR, wintypes.LPWSTR, wintypes.DWORD]
    kernel.GetVolumePathNameW.restype = wintypes.BOOL
    kernel.GetVolumeInformationW.argtypes = [wintypes.LPCWSTR, wintypes.LPWSTR, wintypes.DWORD,
        ctypes.POINTER(wintypes.DWORD), ctypes.POINTER(wintypes.DWORD), ctypes.POINTER(wintypes.DWORD),
        wintypes.LPWSTR, wintypes.DWORD]
    kernel.GetVolumeInformationW.restype = wintypes.BOOL
    root = ctypes.create_unicode_buffer(32768)
    if not kernel.GetVolumePathNameW(str(folder), root, len(root)):
        raise ctypes.WinError(ctypes.get_last_error())
    filesystem = ctypes.create_unicode_buffer(64)
    if not kernel.GetVolumeInformationW(root.value, None, 0, None, None, None, filesystem, len(filesystem)):
        raise ctypes.WinError(ctypes.get_last_error())
    return filesystem.value


def invoke_nand_usb_backup(directory=None, selected_part='All', only_eject=False):
    if not only_eject and not directory:
        raise ValueError('Specify -OutputDirectory on a PC drive with enough free space.')
    if selected_part not in ('All', 'BOOT0', 'BOOT1', 'GPP'):
        raise ValueError('Invalid eMMC partition.')
    if only_eject and selected_part == 'All':
        raise ValueError('For -EjectOnly specify -Part BOOT0, BOOT1, or GPP.')
    check_administrator()
    if only_eject:
        eject_nand_usb_disk(selected_part, get_nand_usb_disk(selected_part))
        return
    root = Path(directory).resolve()
    root.mkdir(parents=True, exist_ok=True)
    parts = ('BOOT0', 'BOOT1', 'GPP') if selected_part == 'All' else (selected_part,)
    serial = None
    print('Connect your Switch over USB. Open Tools > USB Tools and enable Read-Only.')
    print('Cancel any Windows request to format or initialize the USB disk.')
    for partition in parts:
        disk = wait_nand_usb_disk(partition, serial)
        serial = disk.serial
        folder = root / serial
        folder.mkdir(exist_ok=True)
        if get_destination_filesystem(folder).upper() == 'FAT32' and disk.size > 0xffffffff:
            raise RuntimeError('Use NTFS or exFAT for the PC destination; RAW GPP will not fit in a single FAT32 file.')
        if shutil.disk_usage(folder).free < disk.size + 1024 * 1024:
            raise RuntimeError('Not enough free space on the PC destination.')
        name = 'rawnand.bin' if partition == 'GPP' else partition
        copy_nand_image(disk.path, disk.size, folder / name)
        # Never request removal until copying and read-back verification succeed.
        try:
            eject_nand_usb_disk(partition, disk)
        except Exception as error:
            print(f'WARNING: Backup is saved and verified, but automatic eject failed: {error}', file=sys.stderr)
            print('Use Windows eject if available, or VOL+ and VOL- together on the Switch after disk activity stops.')
        print('Wait for Disk ejected on the Switch, then close the USB session before selecting the next part.')
    if selected_part == 'All':
        print(f'Full NAND backup saved in {folder}')
    else:
        print('A full NAND backup requires BOOT0, BOOT1, and rawnand.bin from the same console.')


def parse_args(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('-OutputDirectory', '--output-directory')
    parser.add_argument('-Part', '--part', type=lambda s: s.upper() if s.lower() != 'all' else 'All',
                        choices=('All', 'BOOT0', 'BOOT1', 'GPP'), default='All')
    parser.add_argument('-EjectOnly', '--eject-only', action='store_true')
    return parser.parse_args(argv)


def main(argv=None):
    args = parse_args(argv)
    try:
        invoke_nand_usb_backup(args.output_directory, args.part, args.eject_only)
    except KeyboardInterrupt:
        print('\nStopped. Existing files were kept; .partial files are incomplete backups.', file=sys.stderr)
        return 130
    except Exception as error:
        if args.eject_only:
            print(f'Eject request failed: {error}', file=sys.stderr)
        else:
            print(f'Backup stopped: {error} Existing files were kept; .partial files are incomplete backups.', file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
