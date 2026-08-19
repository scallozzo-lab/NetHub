#ifndef __DMX512_H__
    #define __DMX512_H__

#include "main.h"

#define _DMX_BREAK_100US    652
#define _DMX_MAB_12US       74
#define _DMX_PULSE_4US      20

void DMX_GPIO_Init(void);
void DMX_SendFrame(uint8_t *data, uint16_t length);

#endif