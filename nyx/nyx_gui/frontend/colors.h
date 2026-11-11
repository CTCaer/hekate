#ifndef COLORS_H
#define COLORS_H

#define COLOR_TEXT "CAD3F5" //
#define COLOR_ERROR "ED8796" //FF0000
#define COLOR_ERROR_HEX 0xED8796
#define COLOR_WARN "EED49F" //FFDD00 FFD000 FFBA00
#define COLOR_WARN_HEX 0xEED49F
#define COLOR_ACTION_REQUIRED "F5A97F" //FF8800 FF8000
#define COLOR_ACTION_REQUIRED_HEX 0xF5A97F
#define COLOR_INFO "C6A0F6" //C7EA46 96FF00 D4FF00
#define COLOR_INFO_HEX 0xC6A0F6
#define COLOR_OK "A6DA95" // 00DDFF 00CCFF 00FFCC
#define COLOR_OK_HEX 0xA6DA95

#define COLOR_BASE_HEX 0x363958

#define SCALE_HEX_COLOR(c, f) ( \
    ((unsigned int)(((c) >> 16 & 0xFF) * (f)) << 16) | \
    ((unsigned int)(((c) >> 8  & 0xFF) * (f)) << 8)  | \
    ((unsigned int)(((c)       & 0xFF) * (f)))         \
)

#endif // COLORS_H
