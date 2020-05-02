# Nyx - Background - Icons

The background for Nyx, must be a 1280 x 720 32-bit BMP. Alpha blending is taken into account. For that reason, if a solid background is required, that value must be 0xFF. There are sites that can produce the correct bmp.

The icons supported are 192 x 192  32-bit BMP. You can utilize transparency at will and make nice mixes with the button background.

Additionally, using the `icon={sd path}`, the icon will get colorized if the name ends in `_hue.bmp`. That only works nicely if the icon is a white layout with transparency.

The default system icons (`icon_switch.bmp` and `icon_payload.bmp`) can be replaced with white layouts that have transparency. They can also be replaced with normal icons if the following exist: `icon_switch_custom.bmp` or/and `icon_payload_custom.bmp`


## How to configure

The background must go to /bootloader/res/background.bmp

The icons can be utilized either via `[boot entries names]` -> `boot entries names.bmp` or by using `icon={sd path}`, which should point at a bmp.
