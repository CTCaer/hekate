# hekate - Bootlogo

The bootlogo can be any size with a maximum of 720 x 1280. It is automatically centered when it's smaller than 720 x 1280.

When saving a landscape bootlogo, it should be rotated 90o counterclockwise.

Lastly, the supported format is 32-bit BMP. Classic 24-bit BMPs are not supported for performance reasons.


## How to configure

If the custom logo option is enabled, it will try to load /bootlogo.bmp. If this is not found, the default hekate's logo will be used.

If a boot entry specifies a custom logo path, this is one will be loaded. Again if this is not found, bootlogo.bmp will be loaded and if that fails, hekate's built-in will be used.