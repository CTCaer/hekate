#requires -Version 5.1
<#
.SYNOPSIS
Save a hekate read-only eMMC USB export directly to this Windows PC.
.EXAMPLE
.\tools\backup-nand.ps1 -OutputDirectory D:\SwitchBackup
.EXAMPLE
.\tools\backup-nand.ps1 -Part GPP -EjectOnly
Request eject for a read-only export after other backup tools have finished.
#>
[CmdletBinding()]
param(
    [string]$OutputDirectory,
    [ValidateSet('All', 'BOOT0', 'BOOT1', 'GPP')]
    [string]$Part = 'All',
    [switch]$EjectOnly
)

function Get-NandUsbDisk {
    param([string]$Partition, [string]$ExpectedSerial, [switch]$AllowMissing)

    # The inquiry strings and serial format come from usb_gadget_ums.c.
    # Never accept SD/emuMMC exports or an arbitrary user-supplied disk number.
    $matches = @(Get-CimInstance Win32_DiskDrive -ErrorAction Stop | Where-Object {
        $_.Model -match ('^hekate\s+eMMC\s+' + $Partition + '(?:\s|$)')
    })
    if ($AllowMissing -and $matches.Count -eq 0) { return $null }
    if ($matches.Count -ne 1) {
        throw "Expected exactly one hekate eMMC $Partition USB disk; found $($matches.Count). Select that partition in USB Tools and retry."
    }
    $device = $matches[0]
    $identity = [regex]::Match(([string]$device.SerialNumber).Trim(),
        ('^([0-9a-fA-F]{4,8})\s+eMMC\s+' + $Partition + '$'))
    if (!$identity.Success) { throw 'Cannot identify the eMMC serial from the USB disk.' }
    $serial = $identity.Groups[1].Value.ToUpperInvariant()
    if ($ExpectedSerial -and $serial -ne $ExpectedSerial) {
        throw 'This eMMC belongs to a different console. Keep all three parts from the same console.'
    }
    try {
        $disk = Get-Disk -Number $device.Index -ErrorAction Stop
    } catch {
        # The disk can appear in WMI before the storage provider registers it.
        if ($AllowMissing -and $_.CategoryInfo.Category -eq 'ObjectNotFound') { return $null }
        throw
    }
    if (!$disk.IsReadOnly) { throw 'Enable Read-Only in hekate USB Tools, then reconnect the export.' }
    if ($disk.IsBoot -or $disk.IsSystem) { throw 'Refusing to read a Windows boot/system disk.' }
    [long]$size = $disk.Size
    if ($size -le 0 -or ($size % 512) -ne 0) { throw 'Invalid USB disk size.' }
    return [pscustomobject]@{
        Path = [string]$device.DeviceID
        PnpDeviceId = [string]$device.PNPDeviceID
        Size = $size
        Serial = $serial
    }
}

function Wait-NandUsbDisk {
    param([string]$Partition, [string]$ExpectedSerial)

    $label = if ($Partition -eq 'GPP') { 'eMMC RAW GPP' } else { "eMMC $Partition" }
    Write-Host "Select $label on the Switch. Waiting for automatic detection (Ctrl+C to stop)..."
    while ($true) {
        # Only absence/registration delay is retried. Ambiguous, writable, or
        # mismatched-console exports must still fail before any copying.
        $disk = Get-NandUsbDisk -Partition $Partition -ExpectedSerial $ExpectedSerial -AllowMissing
        if ($null -ne $disk) {
            Write-Host "Detected $label. Starting backup."
            return $disk
        }
        Start-Sleep -Milliseconds 500
    }
}

function Copy-NandImage {
    param([string]$SourcePath, [long]$Size, [string]$Destination)

    if ($Size -le 0 -or ($Size % 512) -ne 0) { throw 'Invalid image size; expected complete 512-byte sectors.' }
    $partial = $Destination + '.partial'
    $checksum = $Destination + '.sha256'
    foreach ($path in @($Destination, $partial, $checksum)) {
        if (Test-Path -LiteralPath $path) { throw "File already exists: $path" }
    }
    $source = $null
    $target = $null
    $hash = [System.Security.Cryptography.SHA256]::Create()
    try {
        # Source is opened with FileAccess.Read only, including for physical disks.
        $source = [System.IO.FileStream]::new($SourcePath, [System.IO.FileMode]::Open,
            [System.IO.FileAccess]::Read, [System.IO.FileShare]::ReadWrite)
        $target = [System.IO.FileStream]::new($partial, [System.IO.FileMode]::CreateNew,
            [System.IO.FileAccess]::Write, [System.IO.FileShare]::None)
        $buffer = New-Object byte[] (1MB)
        [long]$copied = 0
        $timer = [System.Diagnostics.Stopwatch]::StartNew()
        [long]$lastProgress = -1000
        while ($copied -lt $Size) {
            # Select the Int64 overload before narrowing the bounded chunk size.
            $count = [int][Math]::Min([long]$buffer.Length, $Size - $copied)
            $read = $source.Read($buffer, 0, $count)
            if ($read -eq 0) { throw "USB source ended after $copied of $Size bytes." }
            $target.Write($buffer, 0, $read)
            [void]$hash.TransformBlock($buffer, 0, $read, $buffer, 0)
            $copied += $read
            if ($timer.ElapsedMilliseconds - $lastProgress -ge 500) {
                Write-Progress -Activity "Backing up $([IO.Path]::GetFileName($Destination))" `
                    -Status ("{0:N0} / {1:N0} MiB" -f ($copied / 1MB), ($Size / 1MB)) `
                    -PercentComplete ([int](100.0 * $copied / $Size))
                $lastProgress = $timer.ElapsedMilliseconds
            }
        }
        [void]$hash.TransformFinalBlock($buffer, 0, 0)
        $expectedHash = [BitConverter]::ToString($hash.Hash).Replace('-', '').ToLowerInvariant()
        $target.Flush($true)
        $target.Dispose()
        $target = $null
        $source.Dispose()
        $source = $null
        Write-Progress -Activity 'Verifying saved backup' -Status $partial
        $actualHash = (Get-FileHash -LiteralPath $partial -Algorithm SHA256 -ErrorAction Stop).Hash.ToLowerInvariant()
        if ($actualHash -ne $expectedHash) { throw 'Saved backup checksum does not match the USB data.' }
        # Publish the normal filename only after a complete, verified transfer.
        [System.IO.File]::Move($partial, $Destination)
        $checksumFile = [System.IO.FileStream]::new($checksum, [System.IO.FileMode]::CreateNew)
        try {
            $line = $expectedHash + '  ' + [IO.Path]::GetFileName($Destination) + "`n"
            $bytes = [System.Text.Encoding]::ASCII.GetBytes($line)
            $checksumFile.Write($bytes, 0, $bytes.Length)
        } finally { $checksumFile.Dispose() }
        Write-Host "Saved and verified: $Destination"
    } finally {
        if ($target) { $target.Dispose() }
        if ($source) { $source.Dispose() }
        $hash.Dispose()
        Write-Progress -Activity 'Backing up NAND' -Completed
    }
}

function Initialize-NandUsbNative {
    # Add-Type classes persist for the PowerShell session. Use a distinct type
    # from the older media-eject helper, whose Eject argument was a file path.
    if ('Hekate.NandUsbDeviceRemovalV1' -as [type]) { return }
    Add-Type -TypeDefinition @'
using System;
using System.Text;
using System.Runtime.InteropServices;

namespace Hekate {
    public static class NandUsbDeviceRemovalV1 {
        [DllImport("cfgmgr32.dll", CharSet = CharSet.Unicode, ExactSpelling = true)]
        private static extern uint CM_Locate_DevNodeW(out uint node, string id, uint flags);
        [DllImport("cfgmgr32.dll", ExactSpelling = true)]
        private static extern uint CM_Get_Parent(out uint parent, uint node, uint flags);
        [DllImport("cfgmgr32.dll", CharSet = CharSet.Unicode, ExactSpelling = true)]
        private static extern uint CM_Get_Device_IDW(uint node, StringBuilder id, uint length, uint flags);
        [DllImport("cfgmgr32.dll", CharSet = CharSet.Unicode, ExactSpelling = true)]
        private static extern uint CM_Request_Device_EjectW(uint node, out uint veto,
            StringBuilder vetoName, uint length, uint flags);

        public static void Eject(string diskInstanceId) {
            uint node;
            uint result = CM_Locate_DevNodeW(out node, diskInstanceId, 0);
            if (result != 0)
                throw new InvalidOperationException("Cannot locate the USB disk (CONFIGRET " + result + ").");
            // Remove the hekate USB device, as Safely Remove Hardware does.
            // Stop at its device node: never request removal of a USB hub.
            for (int depth = 0; depth < 16; depth++) {
                StringBuilder id = new StringBuilder(512);
                result = CM_Get_Device_IDW(node, id, (uint)id.Capacity, 0);
                if (result != 0)
                    throw new InvalidOperationException("Cannot identify the USB parent (CONFIGRET " + result + ").");
                if (id.ToString().StartsWith(@"USB\VID_11EC&PID_A7E0\", StringComparison.OrdinalIgnoreCase)) {
                    uint veto;
                    StringBuilder vetoName = new StringBuilder(512);
                    result = CM_Request_Device_EjectW(node, out veto, vetoName, (uint)vetoName.Capacity, 0);
                    if (result != 0)
                        throw new InvalidOperationException("Windows refused device removal (CONFIGRET " +
                            result + ", veto " + veto + ": " + vetoName + "). Close other disk users and retry.");
                    return;
                }
                uint parent;
                if (CM_Get_Parent(out parent, node, 0) != 0)
                    break;
                node = parent;
            }
            throw new InvalidOperationException("No matching hekate USB device found; removal was not requested.");
        }
    }
}
'@ -ErrorAction Stop
}

function Request-NandUsbEject {
    param([string]$DevicePath, [string]$PnpDeviceId)
    if ($DevicePath -notmatch '^\\\\\.\\PhysicalDrive[0-9]+$') {
        throw 'Expected a Windows physical disk path for eject.'
    }
    if ([string]::IsNullOrWhiteSpace($PnpDeviceId)) { throw 'Missing USB disk PnP identity; removal was not requested.' }
    Initialize-NandUsbNative
    [Hekate.NandUsbDeviceRemovalV1]::Eject($PnpDeviceId)
}

function Eject-NandUsbDisk {
    param([string]$Partition, $ExpectedDisk)
    # Recheck the device after a long backup: disk numbers can be reused if
    # a cable was unplugged or another device was attached in the meantime.
    $current = Get-NandUsbDisk -Partition $Partition -ExpectedSerial $ExpectedDisk.Serial
    if ($current.Path -ne $ExpectedDisk.Path -or $current.Size -ne $ExpectedDisk.Size -or $current.PnpDeviceId -ne $ExpectedDisk.PnpDeviceId) {
        throw 'The USB disk changed since backup started; eject was not requested.'
    }
    Request-NandUsbEject -DevicePath $current.Path -PnpDeviceId $current.PnpDeviceId
    Write-Host 'Windows accepted safe removal of the hekate USB device.'
}

function Invoke-NandUsbBackup {
    param([string]$Directory, [string]$SelectedPart, [switch]$OnlyEject)

    $ErrorActionPreference = 'Stop'
    if (!$OnlyEject -and !$Directory) { throw 'Specify -OutputDirectory on a PC drive with enough free space.' }
    if ($OnlyEject -and $SelectedPart -eq 'All') { throw 'For -EjectOnly specify -Part BOOT0, BOOT1, or GPP.' }
    $principal = [Security.Principal.WindowsPrincipal]::new([Security.Principal.WindowsIdentity]::GetCurrent())
    if (!$principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
        throw 'Run PowerShell as Administrator to read the USB physical disk.'
    }
    if ($OnlyEject) {
        $disk = Get-NandUsbDisk -Partition $SelectedPart
        Eject-NandUsbDisk -Partition $SelectedPart -ExpectedDisk $disk
        return
    }
    $root = [IO.Directory]::CreateDirectory($ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($Directory)).FullName
    $parts = if ($SelectedPart -eq 'All') { @('BOOT0', 'BOOT1', 'GPP') } else { @($SelectedPart) }
    $serial = $null
    Write-Host 'Connect your Switch over USB. Open Tools > USB Tools and enable Read-Only.'
    Write-Host 'Cancel any Windows request to format or initialize the USB disk.'
    foreach ($partition in $parts) {
        $disk = Wait-NandUsbDisk -Partition $partition -ExpectedSerial $serial
        $serial = $disk.Serial
        $folder = [IO.Directory]::CreateDirectory((Join-Path $root $serial)).FullName
        $drive = [IO.DriveInfo]::new([IO.Path]::GetPathRoot($folder))
        if ($drive.DriveFormat -eq 'FAT32' -and $disk.Size -gt [uint32]::MaxValue) {
            throw 'Use NTFS or exFAT for the PC destination; RAW GPP will not fit in a single FAT32 file.'
        }
        if ($drive.AvailableFreeSpace -lt ($disk.Size + 1MB)) { throw 'Not enough free space on the PC destination.' }
        $name = if ($partition -eq 'GPP') { 'rawnand.bin' } else { $partition }
        Copy-NandImage -SourcePath $disk.Path -Size $disk.Size -Destination (Join-Path $folder $name)
        # Copy-NandImage has closed its handles and verified the saved file.
        # A failed copy throws before this point, so it never triggers eject.
        try {
            Eject-NandUsbDisk -Partition $partition -ExpectedDisk $disk
        } catch {
            Write-Warning ("Backup is saved and verified, but automatic eject failed: " + $_)
            Write-Host 'Use Windows eject if available, or VOL+ and VOL- together on the Switch after disk activity stops.'
        }
        Write-Host 'Wait for Disk ejected on the Switch, then close the USB session before selecting the next part.'
    }
    if ($SelectedPart -eq 'All') { Write-Host "Full NAND backup saved in $folder" }
    else { Write-Host 'A full NAND backup requires BOOT0, BOOT1, and rawnand.bin from the same console.' }
}

# Dot sourcing exposes functions for local tests without accessing hardware.
if ($MyInvocation.InvocationName -ne '.') {
    try { Invoke-NandUsbBackup -Directory $OutputDirectory -SelectedPart $Part -OnlyEject:$EjectOnly }
    catch {
        if ($EjectOnly) {
            Write-Error "Eject request failed: $_" -ErrorAction Continue
        } else {
            Write-Error "Backup stopped: $_ Existing files were kept; .partial files are incomplete backups." -ErrorAction Continue
        }
        exit 1
    }
}
