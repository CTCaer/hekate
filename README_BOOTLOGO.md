# hekate - Bootlogo

The bootlogo can be any size with a maximum of 720 x 1280.

When it's smaller than 720 x 1280, it is automatically centered and the background takes the color of the first pixel.

The process is to create a landscape bootlogo and then rotate it 90 degrees counterclockwise.

Lastly, the supported format is 32-bit (ARGB) BMP. Classic 24-bit (RGB) BMPs are not supported for performance reasons.


## How to configure

If a boot entry specifies a custom logo path (`logopath=`), this one will be loaded.

If the above is not found or the format is not correct, it will try to load `bootloader/bootlogo.bmp`.
If this is not found, the default hekate logo will be used.

(`bootloader/bootlogo.bmp` is basically like a global bootlogo.)
