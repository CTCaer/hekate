#ifndef _KONAMI_H_
#define _KONAMI_H_

#include <bdk.h>

void check_konami_code(jc_gamepad_rpt_t *jc_pad);
void register_konami_code_handler(// pointer to function
void (*handler)(void));

#endif /* _KONAMI_H_ */