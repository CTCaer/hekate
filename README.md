# hekate - CTCaer mod

![Image of Hekate](https://i.imgur.com/O3REoy5.png)


Custom Nintendo Switch bootloader, firmware patcher, and more.


## Bootloader folders and files

| Folder/File          | Description                                                           |
| -------------------- | --------------------------------------------------------------------- |
| bootloader           | Main folder.                                                          |
|  \|__ bootlogo.bmp   | It is used when custom is on and no logopath found. Can be skipped.   |
|  \|__ hekate_ipl.ini | Main bootloader configuration and boot entries.                       |
|  \|__ update.bin     | If newer, it is loaded at boot. For modchips. Can be skipped.         |
| bootloader/ini/      | For individual inis. 'More configs...' menu. Autoboot is supported.   |
| bootloader/sys/      | For system modules.                                                   |
|  \|__ libsys_lp0.bso | LP0 (sleep mode) module. Important!                                   |
| bootloader/payloads/ | For payloads. 'Payloads...' menu. Autoboot only supported by including them into an ini. All CFW bootloaders, tools, Linux payloads are supported. |
| bootloader/libtools/ | Future reserved                                                       |


## Bootloader configuration

The bootloader can be configured via 'bootloader/hekate_ipl.ini' (if it is present on the SD card). Each ini section represents a boot entry, except for the special section 'config' that controls the global configuration.


There are four possible type of entries. "**[ ]**": Boot entry, "**{ }**": Caption, "**#**": Comment, "*newline*": .ini cosmetic newline.


### Configuration keys/values when boot entry is **config**:

| Config option      | Description                                                |
| ------------------ | ---------------------------------------------------------- |
| autoboot=0         | 0: Disable, #: Boot entry number to auto boot.             |
| bootwait=3         | 0: Disable (It also disables bootlogo. Having **VOL-** pressed since injection goes to menu.), #: Time to wait for **VOL-** to enter menu. |
| customlogo=0       | 0: Use default hekate bootlogo, 1: Use bootlogo.bmp.       |
| verification=2     | 0: Disable Backup/Restore verification, 1: Sparse (block based, fast and not 100% reliable), 2: Full (sha256 based, slow and 100% reliable). |
| autohosoff=1       | 0: Disable, 1: If woke up from HOS via an RTC alarm, shows logo, then powers off completely, 2: No logo, immediately powers off.|
| backlight=100      | Screen backlight level. 0-255.                             |


### Possible boot entry key/value combinations:

| Config option      | Description                                                |
| ------------------ | ---------------------------------------------------------- |
| logopath={SD path} | If global customlogo is 1 and logopath empty, bootlogo.bmp will be used. If logopath exists, it will load the specified bitmap. |
| warmboot={SD path} | Replaces the warmboot binary                               |
| secmon={SD path}   | Replaces the security monitor binary                       |
| kernel={SD path}   | Replaces the kernel binary                                 |
| kip1={SD path}     | Replaces/Adds kernel initial process. Multiple can be set. |
| kip1={SD folder}/* | Loads every .kip/.kip1 inside a folder. Compatible with single kip1 keys. |
| kip1patch=patchname| Enables a kip1 patch. Specify with multiple lines and/or as CSV. Implemented patches right now are nosigchk,nogc |
| fullsvcperm=1      | Disables SVC verification (full services permission)       |
| debugmode=1        | Enables Debug mode                                         |
| atmosphere=1       | Enables Atmosphère patching                                |
| payload={SD path}  | Payload launching. Tools, Linux, CFW bootloaders, etc.     |


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
