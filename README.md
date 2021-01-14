# hekate - Nyx

![Image of Hekate](https://user-images.githubusercontent.com/3665130/60391760-bc1e8c00-9afe-11e9-8b7a-b065873081b2.png)


Custom Graphical Nintendo Switch bootloader, firmware patcher, tools, and many more.


## Bootloader folders and files

| Folder/File              | Description                                                           |
| ------------------------ | --------------------------------------------------------------------- |
| bootloader               | Main folder.                                                          |
|  \|__ bootlogo.bmp       | It is used when custom is on and no logopath found. Can be skipped.   |
|  \|__ hekate_ipl.ini     | Main bootloader configuration and boot entries.                       |
|  \|__ patches.ini        | Add external patches. Can be skipped. A template can be found [here](./res/patches_template.ini) |
|  \|__ update.bin         | If newer, it is loaded at boot. For modchips. Auto updated. Can be skipped. |
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
| sept                     | Sept folder. This must always get updated via the Atmosphère release zip. Needed for tools and booting HOS on 7.0.0 and up. Unused for booting HOS if `fss0=` key is defined. |

**Note**: Sept files for booting 7.0.0 and up are expected at /sept folder at root of sd card.


## Bootloader configuration

The bootloader can be configured via 'bootloader/hekate_ipl.ini' (if it is present on the SD card). Each ini section represents a boot entry, except for the special section 'config' that controls the global configuration.


There are four possible type of entries. "**[ ]**": Boot entry, "**{ }**": Caption, "**#**": Comment, "*newline*": .ini cosmetic newline.


You can find a template [Here](./res/hekate_ipl_template.ini)


### Global Configuration keys/values when boot entry is **config**:

| Config option      | Description                                                |
| ------------------ | ---------------------------------------------------------- |
| autoboot=0         | 0: Disable, #: Boot entry number to auto boot.             |
| autoboot_list=0    | 0: Read `autoboot` boot entry from hekate_ipl.ini, 1: Read from ini folder (ini files are ASCII ordered). |
| bootwait=3         | 0: Disable (It also disables bootlogo. Having **VOL-** pressed since injection goes to menu.), #: Time to wait for **VOL-** to enter menu. |
| autohosoff=1       | 0: Disable, 1: If woke up from HOS via an RTC alarm, shows logo, then powers off completely, 2: No logo, immediately powers off.|
| autonogc=1         | 0: Disable, 1: Automatically applies nogc patch if unburnt fuses found and a >= 4.0.0 HOS is booted. |
| bootprotect=0      | 0: Disable, 1: Protect bootloader folder from being corrupted by disallowing reading or editing in HOS. |
| updater2p=0        | 0: Disable, 1: Force updates (if needed) the reboot2payload binary to be hekate. |
| backlight=100      | Screen backlight level. 0-255.                             |


### Nyx Global Configuration keys/values for (nyx.ini):

| Config option      | Description                                                |
| ------------------ | ---------------------------------------------------------- |
| themecolor=167     | Sets Nyx color of text highlights.                         |
| timeoff=100        | Sets time offset in HEX. Must be in HOS epoch format       |
| homescreen=0       | Sets home screen. 0: Home menu, 1: All configs (merges Launch and More configs), 2: Launch, 3: More Configs. |
| verification=1     | 0: Disable Backup/Restore verification, 1: Sparse (block based, fast and mostly reliable), 2: Full (sha256 based, slow and 100% reliable). |
| umsemmcrw=0        | 1: eMMC/emuMMC UMS will be mounted as writable by default. |
| jcdisable=0        | 1: Disables Joycon driver completely.                      |
| newpowersave=1     | 0: Timer based, 1: DRAM frequency based (Better). Use 0 if Nyx hangs. |


### Boot entry key/value combinations:

| Config option          | Description                                                |
| ---------------------- | ---------------------------------------------------------- |
| warmboot={SD path}     | Replaces the warmboot binary                               |
| secmon={SD path}       | Replaces the security monitor binary                       |
| kernel={SD path}       | Replaces the kernel binary                                 |
| kip1={SD path}         | Replaces/Adds kernel initial process. Multiple can be set. |
| kip1={SD folder}/*     | Loads every .kip/.kip1 inside a folder. Compatible with single kip1 keys. |
| fss0={SD path}         | Takes a fusee-secondary binary and `extracts` all needed parts from it. kips, exosphere, warmboot and sept. |
| fss0experimental=1     | Enables loading of experimental content from a FSS0 storage |
| exofatal={SD path}     | Replaces the exosphere fatal binary for Mariko             |
| kip1patch=patchname    | Enables a kip1 patch. Specify with multiple lines and/or as CSV. If not found, an error will show up |
| fullsvcperm=1          | Disables SVC verification (full services permission)       |
| debugmode=1            | Enables Debug mode. Obsolete when used with exosphere as secmon. |
| atmosphere=1           | Enables Atmosphère patching.                               |
| emupath={SD folder}    | Forces emuMMC to use the selected one. (=emuMMC/RAW1, =emuMMC/SD00, etc). emuMMC must be created by hekate because it uses the raw_based/file_based files. |
| emummcforce=1          | Forces the use of emuMMC. If emummc.ini is disabled or not found, then it causes an error. |
| emummc_force_disable=1 | Disables emuMMC, if it's enabled.                           |
| stock=1                | Disables unneeded kernel patching and CFW kips when running stock or semi-stock. `If emuMMC is enabled, emummc_force_disabled=1` is required. emuMMC is not supported on stock. If additional KIPs are needed other than OFW's, you can define them with `kip1` key. No kip should be used that relies on Atmosphère patching, because it will hang. If `NOGC` is needed, use `kip1patch=nogc`. |
| id=idname              | Identifies boot entry for forced boot via id. Max 7 chars. |
| payload={SD path}      | Payload launching. Tools, Linux, CFW bootloaders, etc.     |
| logopath={SD path}     | If no logopath, `bootloader/bootlogo.bmp` will be used if exists. If logopath exists, it will load the specified bitmap. |
| icon={SD path}         | Force Nyx to use the icon defined here. If this is not found, it will check for a bmp named as the boot entry ([Test 2] -> `bootloader/res/Test 2.bmp`). Otherwise default will be used. |


### Boot entry key/value Exosphère combinations:

| Config option          | Description                                                |
| ---------------------- | ---------------------------------------------------------- |
| nouserexceptions=1     | Disables usermode exception handlers when paired with Exosphère. |
| userpmu=1              | Enables user access to PMU when paired with Exosphère.     |
| cal0blank=1            | Overrides Exosphère config `blank_prodinfo_{sys/emu}mmc`. If that key doesn't exist, `exosphere.ini` will be used. |
| cal0writesys=1         | Overrides Exosphère config `allow_writing_to_cal_sysmmc`. If that key doesn't exist, `exosphere.ini` will be used. |


**Note1**: When using the wildcard (`/*`) with `kip1` you can still use the normal `kip1` after that to load extra single kips.

**Note2**: When using FSS0 it parses exosphere, warmboot and all core kips. You can override the first 2 by using `secmon`/`warmboot` after defining `fss0`.
You can define `kip1` to load an extra kip or many via the wildcard (`/*`) usage.

**Warning**: Never define *fss0 core* kips when using `fss0` and make sure that the folder (when using `/*`), does not include them.
This is in case the kips are incompatible between them. If compatible, you can override `fss0` kips with no issues (useful for testing with intermediate kip changes).


### Payload storage:

hekate has a boot storage in the binary that helps it configure it outside of BPMP enviroment:

| Offset / Name           | Description                                                       |
| ----------------------- | ----------------------------------------------------------------- |
| '0x94' boot_cfg         | bit0: `Force AutoBoot`, bit1: `Show launch log`, bit2: `Boot from ID`, bit3: `Boot to emuMMC`, bit7: `sept run`. |
| '0x95' autoboot         | If `Force AutoBoot`: 0: Force go to menu, else boot that entry.   |
| '0x96' autoboot_list    | If `Force AutoBoot` and `autoboot` then it boots from ini folder. |
| '0x97' extra_cfg        | When menu is forced: bit5: `Run UMS`, bit7: `Run Dump pkg1/2`.    |
| '0x98' xt_str[128]      | Depends on the set cfg bits.                                      |
| '0x98' ums[1]           | When `Run UMS` is set, it will launch the selected UMS. 0: SD, 1: eMMC BOOT0, 2: eMMC BOOT1, 3: eMMC GPP, 4: emuMMC BOOT0, 5: emuMMC BOOT1, 6: emuMMC GPP,  |
| '0x98' id[8]            | When `Boot from ID` is set, it will search all inis automatically and find the boot entry with that id and boot it. Must be NULL terminated. |
| '0xA0' emummc_path[120] | When `Boot to emuMMC` is set, it will override the current emuMMC (boot entry or emummc.ini). Must be NULL terminated. |


If the main .ini is not found, it is created on the first hekate boot.


```
hekate  (c) 2018,      naehrwert, st4rk.
        (c) 2018-2020, CTCaer.

Nyx GUI (c) 2019-2020, CTCaer.

Thanks to: derrek, nedwill, plutoo, shuffle2, smea, thexyz, yellows8.
Greetings to: fincs, hexkyz, SciresM, Shiny Quagsire, WinterMute.

Open source and free packages used:
 - FatFs R0.13a, Copyright (c) 2017, ChaN
 - bcl-1.2.0, Copyright (c) 2003-2006, Marcus Geelnard
 - Atmosphère (Exosphere types/panic, prc id kernel patches),
   Copyright (c) 2018-2019, Atmosphère-NX
 - elfload, Copyright (c) 2014 Owen Shepherd, Copyright (c) 2018 M4xw
 - Littlev Graphics Library, Copyright (c) 2016 Gabor Kiss-Vamosi

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
