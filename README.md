# hekate

![Image of Hekate](https://upload.wikimedia.org/wikipedia/commons/f/fc/H%C3%A9cate_-_Mallarm%C3%A9.png)

Nintendo Switch bootloader, firmware patcher, and more.


## ipl config

The ipl can be configured via 'hekate_ipl.ini' (if it is present on the SD card). Each ini section represents a boot entry, except for the special section 'config' that controls the global configuration.

### Possible key/value combinations:

| Config option      | Description                                                |
| ------------------ | ---------------------------------------------------------- |
| warmboot={SD path} | Replaces the warmboot binary                               |
| secmon={SD path}   | Replaces the security monitor binary                       |
| kernel={SD path}   | Replaces the kernel binary                                 |
| kip1={SD path}     | Replaces/Adds kernel initial process. Multiple can be set. |
| fullsvcperm=1      | Disables SVC verification                                  |
| debugmode=1        | Enables Debug mode                                         |



```
hekate (C) 2018 naehrwert, st4rk.

Thanks to: derrek, nedwill, plutoo, shuffle2, smea, thexyz, yellows8.
Greetings to: fincs, hexkyz, SciresM, Shiny Quagsire, WinterMute.

Open source and free packages used:
 - FatFs R0.13a, Copyright (C) 2017, ChaN
 - bcl-1.2.0,    Copyright (C) 2003-2006, Marcus Geelnard

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