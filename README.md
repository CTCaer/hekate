# hekate - Nyx

![Image of Hekate](https://user-images.githubusercontent.com/3665130/60391760-bc1e8c00-9afe-11e9-8b7a-b065873081b2.png)


Custom Graphical Nintendo Switch bootloader, firmware patcher, tools, and many more.



- [Features](#features)
- [Bootloader folders and files](#bootloader-folders-and-files)
- [Bootloader configuration](#bootloader-configuration)
  * [hekate global Configuration keys/values](#hekate-global-configuration-keysvalues-when-entry-is-config)
  * [Boot entry key/value combinations](#boot-entry-keyvalue-combinations)
  * [Boot entry key/value combinations for Exosphère](#boot-entry-keyvalue-combinations-for-exosphère)
  * [Payload storage](#payload-storage)
  * [Nyx Configuration keys/values](#nyx-configuration-keysvalues-nyxini)



## Features

- **Fully Configurable and Graphical** with Touchscreen and Joycon input support
- **Launcher Style, Background and Color Themes**
- **HOS (Switch OS) Bootloader** -- For CFW Sys/Emu, OFW Sys and Stock Sys
- **Android & Linux Bootloader**
- **Payload Launcher**
- **eMMC/emuMMC Backup/Restore Tools**
- **SD Card Partition Manager** -- Prepares and formats SD Card for any combo of HOS (Sys/emuMMC), Android and Linux
- **emuMMC Creation & Manager** -- Can also migrate and fix existing emuMMC
- **Switch Android & Linux flasher**
- **USB Mass Storage (UMS) for SD/eMMC/emuMMC** -- Converts Switch into a SD Card Reader
- **USB Gamepad** -- Converts Switch with Joycon into a USB HID Gamepad
- **Hardware and Peripherals info** (SoC, Fuses, RAM, Display, Touch, eMMC, SD, Battery, PSU, Charger)
- **Many other tools** like Archive Bit Fixer, Touch Calibration, SD/eMMC Benchmark, AutoRCM enabler and more


## Bootloader folders and files

| Folder/File              | Description                                                           |
| ------------------------ | --------------------------------------------------------------------- |
| bootloader               | Main folder.                                                          |
|  \|__ bootlogo.bmp       | It is used if no `logopath` key is found. User provided. Can be skipped. |
|  \|__ hekate_ipl.ini     | Main bootloader configuration and boot entries in `Launch` menu.      |
|  \|__ nyx.ini            | Nyx GUI configuration                                                 |
|  \|__ patches.ini        | Add external patches. Can be skipped. A template can be found [here](./res/patches_template.ini) |
|  \|__ update.bin         | If newer, it is loaded at boot. Normally for modchips. Auto updated and created at first boot. |
| bootloader/ini/          | For individual inis. `More configs` menu. Autoboot is supported.   |
| bootloader/res/          | Nyx user resources. Icons and more.                                   |
|  \|__ background.bmp     | Nyx - Custom background. User provided.                               |
|  \|__ icon_switch.bmp    | Nyx - Default icon for CFWs.                                          |
|  \|__ icon_payload.bmp   | Nyx - Default icon for Payloads.                                      |
| bootloader/sys/          | hekate and Nyx system modules folder. !Important!                     |
|  \|__ emummc.kipm        | emuMMC KIP1 module.                                                   |
|  \|__ libsys_lp0.bso     | LP0 (sleep mode) module.                                              |
|  \|__ libsys_minerva.bso | Minerva Training Cell. Used for DRAM Frequency training.              |
|  \|__ nyx.bin            | Nyx - hekate's GUI.                                                   |
|  \|__ res.pak            | Nyx resources package.                                                |
|  \|__ thk.bin            | Atmosphère Tsec Hovi Keygen.                                          |
|  \|__ /l4t/              | Folder with firmware relevant to L4T (Linux/Android).                 |
| bootloader/screenshots/  | Folder where Nyx screenshots are saved                                |
| bootloader/payloads/     | For the `Payloads` menu. All CFW bootloaders, tools, Linux payloads are supported. Autoboot only supported by including them into an ini. |
| bootloader/libtools/     | Reserved                                                              |



## Bootloader configuration

The bootloader can be configured via 'bootloader/hekate_ipl.ini' (if it is present on the SD card). Each ini section represents a boot entry, except for the special section 'config' that controls the global configuration.


There are four possible type of entries. "**[ ]**": Boot entry, "**{ }**": Caption, "**#**": Comment, "*newline*": .ini cosmetic newline.


**You can find a template [Here](./res/hekate_ipl_template.ini)**


### hekate Global Configuration keys/values (when entry is *[config]*):

| Config option      | Description                                                    |
| ------------------ | -------------------------------------------------------------- |
| autoboot=0         | 0: Disable, #: Boot entry number to auto boot.                 |
| autoboot_list=0    | 0: Read `autoboot` boot entry from hekate_ipl.ini, 1: Read from ini folder (ini files are ASCII ordered). |
| bootwait=3         | 0: Disable (It also disables bootlogo. Having **VOL-** pressed since injection goes to menu.), #: Time to wait for **VOL-** to enter menu. Max: 20s. |
| noticker=0         | 0: Animated line is drawn during custom bootlogo, signifying time left to skip to menu. 1: Disable. |
| autohosoff=1       | 0: Disable, 1: If woke up from HOS via an RTC alarm, shows logo, then powers off completely, 2: No logo, immediately powers off.|
| autonogc=1         | 0: Disable, 1: Automatically applies nogc patch if unburnt fuses found and a >= 4.0.0 HOS is booted. |
| bootprotect=0      | 0: Disable, 1: Protect bootloader folder from being corrupted by disallowing reading or editing in HOS. |
| updater2p=0        | 0: Disable, 1: Force updates (if needed) the reboot2payload binary to be hekate. |
| backlight=100      | Screen backlight level. 0-255.                                 |


### Boot entry key/value combinations:

| Config option          | Description                                                |
| ---------------------- | ---------------------------------------------------------- |
| warmboot={FILE path}   | Replaces the warmboot binary                               |
| secmon={FILE path}     | Replaces the security monitor binary                       |
| kernel={FILE path}     | Replaces the kernel binary                                 |
| kip1={FILE path}       | Replaces/Adds kernel initial process. Multiple can be set. |
| kip1={FOLDER path}/*   | Loads every .kip/.kip1 inside a folder. Compatible with single kip1 keys. |
| pkg3={FILE path}       | Takes an Atmosphere `package3` binary and `extracts` all needed parts from it. kips, exosphere, warmboot and mesophere. |
| fss0={FILE path}       | Same as above. !Deprecated! |
| pkg3ex=1               | Enables loading of experimental content from a PKG3/FSS0 storage |
| pkg3kip1skip={KIP name} | Skips loading a kip from `pkg3`/`fss0`. Allows multiple and `,` as separator. The name must exactly match the name in `PKG3`. |
| exofatal={FILE path}   | Replaces the exosphere fatal binary for Mariko             |
| ---------------------- | ---------------------------------------------------------- |
| kip1patch=patchname    | Enables a kip1 patch. Allows multiple and `,` as separator. If actual patch is not found, a warning will show up. |
| emupath={FOLDER path}  | Forces emuMMC to use the selected one. (=emuMMC/RAW1, =emuMMC/SD00, etc). emuMMC must be created by hekate because it uses the raw_based/file_based files. |
| emummcforce=1          | Forces the use of emuMMC. If emummc.ini is disabled or not found, then it causes an error. |
| emummc_force_disable=1 | Disables emuMMC, if it's enabled.                           |
| stock=1                | OFW via hekate bootloader. Disables unneeded kernel patching and CFW kips when running stock. `If emuMMC is enabled, emummc_force_disable=1` is required. emuMMC is not supported on stock. If additional KIPs are needed other than OFW's, you can define them with `kip1` key. No kip should be used that relies on Atmosphère patching, because it will hang. If `NOGC` is needed, use `kip1patch=nogc`. |
| fullsvcperm=1          | Disables SVC verification (full services permission). Doesn't work with Mesosphere as kernel. |
| debugmode=1            | Enables Debug mode. Obsolete when used with exosphere as secmon. |
| kernelprocid=1         | Enables stock kernel process id send/recv patching. Not needed when `pkg3`/`fss0` is used. |
| ---------------------- | ---------------------------------------------------------- |
| payload={FILE path}    | Payload launching. Tools, Android/Linux, CFW bootloaders, etc. Any key above when used with that, doesn't get into account. |
| ---------------------- | ---------------------------------------------------------- |
| l4t=1                  | L4T Linux/Android native launching.                        |
| boot_prefixes={FOLDER path} | L4T bootstack directory.                              |
| ram_oc=0               | L4T RAM Overclocking. Check README_CONFIG.txt for more info. |
| ram_oc_vdd2=1100       | L4T RAM VDD2 Voltage. Set VDD2 (T210B01) or VDD2/VDDQ (T210) voltage. 1050-1175. |
| ram_oc_vddq=600        | L4T RAM VDDQ Voltage. Set VDDQ (T210B01). 550-650.         |
| uart_port=0            | Enables logging on serial port for L4T uboot/kernel.       |
| sld_type=0x31444C53    | Controls the type of seamless display support. 0x0: Disable, 0x31444C53: L4T seamless display. |
| Additional keys        | Each distro supports more keys. Check README_CONFIG.txt  for more info. |
| ---------------------- | ---------------------------------------------------------- |
| bootwait=3             | Overrides global bootwait from `[config]`.                 |
| id=IDNAME              | Identifies boot entry for forced boot via id. Max 7 chars. |
| logopath={FILE path}   | If it exists, it will load the specified bitmap. Otherwise `bootloader/bootlogo.bmp` will be used if exists |
| icon={FILE path}       | Force Nyx to use the icon defined here. If this is not found, it will check for a bmp named as the boot entry ([Test 2] -> `bootloader/res/Test 2.bmp`). Otherwise defaults will be used. |


**Note1**: When using the wildcard (`/*`) with `kip1` you can still use the normal `kip1` after that to load extra single kips.

**Note2**: When using PKG3/FSS0 it parses exosphere, warmboot and all core kips. You can override the first 2 by using `secmon`/`warmboot` after defining `pkg3`/`fss0`.
You can define `kip1` to load an extra kip or many via the wildcard (`/*`) usage.

**Warning**: Careful when you override *pkg3/fss core* kips with `kip1`.
That's in case the kips are incompatible between them. If compatible, you can override `pkg3`/`fss0` kips with no issues (useful for testing with intermediate kip changes). In such cases, the `kip1` line must be **after** `pkg3`/`fss0` line.


### Boot entry key/value combinations for Exosphère:

| Config option          | Description                                                |
| ---------------------- | ---------------------------------------------------------- |
| nouserexceptions=1     | Disables usermode exception handlers when paired with Exosphère. |
| userpmu=1              | Enables user access to PMU when paired with Exosphère.     |
| cal0blank=1            | Overrides Exosphère config `blank_prodinfo_{sys/emu}mmc`. If that key doesn't exist, `exosphere.ini` will be used. |
| cal0writesys=1         | Overrides Exosphère config `allow_writing_to_cal_sysmmc`. If that key doesn't exist, `exosphere.ini` will be used. |
| usb3force=1            | Overrides system settings mitm config `usb30_force_enabled`. If that key doesn't exist, `system_settings.ini` will be used. |


**Note**: `cal0blank`, `cal0writesys`, `usb3force`, as stated override the `exosphere.ini` or `system_settings.ini`. 0: Disable, 1: Enable, Key Missing: Use original value.


**Note2**: `blank_prodinfo_{sys/emu}mmc`, `allow_writing_to_cal_sysmmc` and `usb30_force_enabled` in `exosphere.ini` and `system_settings.ini` respectively, are the only atmosphere config keys that can affect hekate booting configuration externally, **if** the equivalent keys in hekate config are missing.


### Payload storage:

hekate has a boot storage in the binary that helps it configure it outside of BPMP environment:

| Offset / Name           | Description                                                       |
| ----------------------- | ----------------------------------------------------------------- |
| '0x94' boot_cfg         | bit0: `Force AutoBoot`, bit1: `Show launch log`, bit2: `Boot from ID`, bit3: `Boot to emuMMC`. |
| '0x95' autoboot         | If `Force AutoBoot`, 0: Force go to menu, else boot that entry.   |
| '0x96' autoboot_list    | If `Force AutoBoot` and `autoboot` then it boots from ini folder. |
| '0x97' extra_cfg        | When menu is forced: bit5: `Run UMS`.                             |
| '0x98' xt_str[128]      | Depends on the set cfg bits.                                      |
| '0x98' ums[1]           | When `Run UMS` is set, it will launch the selected UMS. 0: SD, 1: eMMC BOOT0, 2: eMMC BOOT1, 3: eMMC GPP, 4: emuMMC BOOT0, 5: emuMMC BOOT1, 6: emuMMC GPP,  |
| '0x98' id[8]            | When `Boot from ID` is set, it will search all inis automatically and find the boot entry with that id and boot it. Must be NULL terminated. |
| '0xA0' emummc_path[120] | When `Boot to emuMMC` is set, it will override the current emuMMC (boot entry or emummc.ini). Must be NULL terminated. |


### Nyx Configuration keys/values (nyx.ini):

| Config option      | Description                                                |
| ------------------ | ---------------------------------------------------------- |
| themebg=2d2d2d     | Sets Nyx background color in HEX. EXPERIMENTAL.            |
| themecolor=167     | Sets Nyx color of text highlights.                         |
| entries5col=0      | 1: Sets Launch entry columns from 4 to 5 per line. For a total of 10 entries. |
| timeoffset=100     | Sets time offset in HEX. Must be in epoch format           |
| timedst=0          | Enables automatic daylight saving hour adjustment          |
| homescreen=0       | Sets home screen. 0: Home menu, 1: All configs (merges Launch and More configs), 2: Launch, 3: More Configs. |
| verification=1     | 0: Disable Backup/Restore verification, 1: Sparse (block based, fast and mostly reliable), 2: Full (sha256 based, slow and 100% reliable). |
| ------------------ | ------- The following options can only be edited in nyx.ini ------- |
| umsemmcrw=0        | 1: eMMC/emuMMC UMS will be mounted as writable by default. |
| jcdisable=0        | 1: Disables Joycon driver completely.                      |
| jcforceright=0     | 1: Forces right joycon to be used as main mouse control.   |
| bpmpclock=1        | 0: Auto, 1: 589 MHz, 2: 576 MHz, 3: 563 MHz, 4: 544 MHz, 5: 408 MHz. Use 2 to 5 if Nyx hangs or some functions like UMS/Backup Verification fail. |


```
hekate  (c) 2018,      naehrwert, st4rk.
        (c) 2018-2025, CTCaer.

Nyx GUI (c) 2019-2025, CTCaer.

Thanks to: derrek, nedwill, plutoo, shuffle2, smea, thexyz, yellows8.
Greetings to: fincs, hexkyz, SciresM, Shiny Quagsire, WinterMute.

Open source and free packages used:
 - Littlev Graphics Library,
   Copyright (c) 2016-2018 Gabor Kiss-Vamosi
 - FatFs R0.13c,
   Copyright (c) 2006-2018, ChaN
   Copyright (c) 2018-2022, CTCaer
 - bcl-1.2.0,
   Copyright (c) 2003-2006, Marcus Geelnard
 - blz,
   Copyright (c) 2018, SciresM
 - elfload,
   Copyright (c) 2014 Owen Shepherd,
   Copyright (c) 2018 M4xw

                         ___
                      .-'   `'.
                     /         \
                     |         ;
                     |         |           ___.--,
            _.._     |0) = (0) |    _.---'`__.-( (_.
     __.--'`_.. '.__.\    '--. \_.-' ,.--'`     `""`
    ( ,.--'`   ',__ /./;   ;, '.__.'`    __
    _`) )  .---.__.' / |   |\   \__..--""  """--.,_
   `---' .'.''-._.-'`_./  /\ '.  \ _.--''````'''--._`-.__.'
         | |  .' _.-' |  |  \  \  '.               `----`
          \ \/ .'     \  \   '. '-._)
           \/ /        \  \    `=.__`'-.
           / /\         `) )    / / `"".`\
     , _.-'.'\ \        / /    ( (     / /
      `--'`   ) )    .-'.'      '.'.  | (
             (/`    ( (`          ) )  '-;   [switchbrew]
```
