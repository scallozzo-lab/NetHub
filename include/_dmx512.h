#ifndef __DMX512_H__
    #define __DMX512_H__

#include "main.h"

#define _DMX_BREAK_100US    295//340
#define _DMX_MAB_12US       26//38
#define _DMX_PULSE_4US      11//11

void DMX_GPIO_Init(void);
void DMX_SendFrame(uint8_t scode, uint8_t *data, uint16_t length);

#endif