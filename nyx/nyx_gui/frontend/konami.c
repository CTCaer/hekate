#include "konami.h"

// pointer to function
static void (*konami_code_handler)(void) = NULL;

void check_konami_code(jc_gamepad_rpt_t *jc_pad){

    static u32 konami_code[] = {
        JC_BTNS_UP,
        JC_BTNS_UP,
        JC_BTNS_DOWN,
        JC_BTNS_DOWN,
        JC_BTNS_LEFT,
        JC_BTNS_RIGHT,
        JC_BTNS_LEFT,
        JC_BTNS_RIGHT,
        JC_BTNS_B,
        JC_BTNS_A
    };

    static u8 konami_code_idx = 0;

    u32 key_pressed = (jc_pad->buttons & konami_code[konami_code_idx]);
    if (key_pressed && (key_pressed == konami_code[konami_code_idx] || key_pressed == konami_code[(konami_code_idx = 0)]))
    {
        konami_code_idx++;
        if (konami_code_idx == 10)
        {
            konami_code_idx = 0;
            if (konami_code_handler)
            {
                (*konami_code_handler)();
            }
        }
    }
}

void register_konami_code_handler(// pointer to function
void (*handler)(void)) {
    konami_code_handler = handler;
}
