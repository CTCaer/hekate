# hekate

![Image of Hekate](https://upload.wikimedia.org/wikipedia/commons/f/fc/H%C3%A9cate_-_Mallarm%C3%A9.png)

Nintendo Switch bootloader, firmware patcher, and more.

## ipl config

The ipl can be configured via 'hekate_ipl.ini' (if it is present on the SD card). Each ini section represents a boot entry, except for the special section 'config' that controls the global configuration.

Possible key/value combinations:

 - warmboot={SD path}
 - secmon={SD path}
 - kernel={SD path}
 - kip1={SD path}
