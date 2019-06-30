# Nyx - Background - Icons

The background for Nyx, must be a 1280 x 720 32-bit BMP. Alpha blending is taken into account. For that reason, if a solid background is required, that value must be 0xFF. There are sites that can produce the correct bmp.

The icons supported are 192 x 192  32-bit BMP. You can utilize transparency at will and make nice mixes with the button background.


## How to configure

The background must go to /bootloader/res/background.bmp

The icons can be utilized either via `[boot entries names]` -> `boot entries names.bmp` or by using `icon={sd path}`, which should point at a bmp.
