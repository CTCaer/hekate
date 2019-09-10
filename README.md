# hekate - CTCaer mod

![Image of Hekate](https://user-images.githubusercontent.com/3665130/60391760-bc1e8c00-9afe-11e9-8b7a-b065873081b2.png)


Custom Nintendo Switch bootloader, firmware patcher, and more.


## Bootloader folders and files

| Folder/File              | Description                                                           |
| ------------------------ | --------------------------------------------------------------------- |
| bootloader               | Main folder.                                                          |
|  \|__ bootlogo.bmp       | It is used when custom is on and no logopath found. Can be skipped.   |
|  \|__ hekate_ipl.ini     | Main bootloader configuration and boot entries.                       |
|  \|__ patches.ini        | Add external patches. If not found `patches_template.ini` will be loaded. Can be skipped. |
|  \|__ update.bin         | If newer, it is loaded at boot. For modchips. Can be skipped.         |
| bootloader/ini/          | For individual inis. 'More configs...' menu. Autoboot is supported.   |
| bootloader/res/          | Nyx user resources. Icons and more.                                   |
|  \|__ background.bmp     | Nyx - custom background.                                              |
|  \|__ icon_switch.bmp    | Nyx - Default icon for CFWs.                                          |
|  \|__ icon_payload.bmp   | Nyx - Default icon for Payloads.                                      |
|  \|__ icon_lakka.bmp     | Nyx - Default icon for Lakka.                                         |
| bootloader/sys/          | For system modules.                                                   |
|  \|__ emummc.kipm        | emuMMC KIP1 module. Important!                                        |
|  \|__ libsys_lp0.bso     | LP0 (sleep mode) module. Important!                                   |
|  \|__ libsys_minerva.bso | Minerva Training Cell. Used for DRAM Frequency training. Important!   |
|  \|__ nyx.bin            | Nyx - Our GUI. Important!                                             |
|  \|__ res.pak            | Nyx resources package. Important!                                     |
| bootloader/screenshots/  | Folder where Nyx screenshots are saved                                |
| bootloader/payloads/     | For payloads. 'Payloads...' menu. Autoboot only supported by including them into an ini. All CFW bootloaders, tools, Linux payloads are supported. |
| bootloader/libtools/     | Future reserved                                                       |

**Note**: Sept files for booting 7.0.0 and up are expected at /sept folder at root of sd card.


## Bootloader configuration

The bootloader can be configured via 'bootloader/hekate_ipl.ini' (if it is present on the SD card). Each ini section represents a boot entry, except for the special section 'config' that controls the global configuration.


There are four possible type of entries. "**[ ]**": Boot entry, "**{ }**": Caption, "**#**": Comment, "*newline*": .ini cosmetic newline.


### Global Configuration keys/values when boot entry is **config**:

| Config option      | Description                                                |
| ------------------ | ---------------------------------------------------------- |
| autoboot=0         | 0: Disable, #: Boot entry number to auto boot.             |
| autoboot_list=0    | 0: Read `autoboot` boot entry from hekate_ipl.ini, 1: Read from ini folder (ini files are ASCII ordered). |
| bootwait=3         | 0: Disable (It also disables bootlogo. Having **VOL-** pressed since injection goes to menu.), #: Time to wait for **VOL-** to enter menu. |
| verification=2     | 0: Disable Backup/Restore verification, 1: Sparse (block based, fast and not 100% reliable), 2: Full (sha256 based, slow and 100% reliable). |
| autohosoff=1       | 0: Disable, 1: If woke up from HOS via an RTC alarm, shows logo, then powers off completely, 2: No logo, immediately powers off.|
| autonogc=1         | 0: Disable, 1: Automatically applies nogc patch if unburnt fuses found and a >= 4.0.0 HOS is booted. |
| backlight=100      | Screen backlight level. 0-255.                             |


### Possible boot entry key/value combinations:

| Config option          | Description                                                |
| ---------------------- | ---------------------------------------------------------- |
| logopath={SD path}     | If global customlogo is 1 and logopath empty, bootlogo.bmp will be used. If logopath exists, it will load the specified bitmap. |
| warmboot={SD path}     | Replaces the warmboot binary                               |
| secmon={SD path}       | Replaces the security monitor binary                       |
| kernel={SD path}       | Replaces the kernel binary                                 |
| kip1={SD path}         | Replaces/Adds kernel initial process. Multiple can be set. |
| kip1={SD folder}/*     | Loads every .kip/.kip1 inside a folder. Compatible with single kip1 keys. |
| fss0={SD path}         | Takes a fusee-secondary binary and extracts all needed parts from it. |
| kip1patch=patchname    | Enables a kip1 patch. Specify with multiple lines and/or as CSV. Current available patches nosigchk. |
| fullsvcperm=1          | Disables SVC verification (full services permission)       |
| debugmode=1            | Enables Debug mode. Obsolete when used with exosphere as secmon. |
| atmosphere=1           | Enables Atmosphère patching.                               |
| nouserexceptions=1     | Disables usermode exception handlers when paired with Exosphère. |
| emummc_force_disable=1 | Disabled emuMMC if it's enabled.                           |
| stock=1                | Disables unneeded kernel patching when running stock or semi-stock. `If emuMMC is enabled, emummc_force_disabled=1` is required to run completely stock. |
| payload={SD path}      | Payload launching. Tools, Linux, CFW bootloaders, etc.     |
| icon={SD path}         | Force Nyx to use the icon defined here. If this is not found, it will check for a bmp named as the boot entry ([Test 2] -> `bootloader/res/Test 2.bmp`). Otherwise default will be used. |

**Note1**: When using the wildcard (`/*`) with `kip1` you can still use the normal `kip1` after that to load extra single kips.

**Note2**: When using FSS0 it parses exosphere, warmboot and all core kips. You can override the first 2 by using `secmon`/`warmboot` after defining `fss0`.
You can define `kip1` to load an extra kip or many via the wildcard (`/*`) usage.

**Warning**: Never define core kips when using `fss0` and make sure that the folder (when using `/*`), does not include them. This is in case the kips are incompatible between them.


### Payload storage:

Hekate now has a new storage in the binary that helps it configure it outside of BPMP enviroment:

| Offset / Name        | Description                                                       |
| -------------------- | ----------------------------------------------------------------- |
| '0x94' boot_cfg      | bit0: Force AutoBoot, bit1: Show launch log, bit2: sept run.      |
| '0x95' autoboot      | If `Force AutoBoot`: 0: Force go to menu, else boot that entry.   |
| '0x96' autoboot_list | If `Force AutoBoot` and `autoboot` then it boots from ini folder. |
| '0x97' extra_cfg     | bit7: Force Nyx to run `Dump pkg1/2.                              |
| '0x98' rsvd[128]     | Reserved.                                                         |


You can find a template [Here](./res/hekate_ipl_template.ini)

If the main .ini is not found, it is created on the first hekate boot.


```
hekate     (C) 2018 naehrwert, st4rk
CTCaer mod (C) 2018 CTCaer.

Thanks to: derrek, nedwill, plutoo, shuffle2, smea, thexyz, yellows8.
Greetings to: fincs, hexkyz, SciresM, Shiny Quagsire, WinterMute.

Open source and free packages used:
 - FatFs R0.13a, Copyright (C) 2017, ChaN
 - bcl-1.2.0, Copyright (C) 2003-2006, Marcus Geelnard
 - Atmosphère (SE sha256, prc id kernel patches), Copyright (C) 2018, Atmosphère-NX
 - elfload, Copyright (C) 2014 Owen Shepherd, Copyright (C) 2018 M4xw

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
