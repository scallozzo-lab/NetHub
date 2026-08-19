#ifndef __TIMER_H__
    #define  __TIMER_H__

#include "main.h"


void Timer2_Init_us(uint16_t fbasemhz);
int16_t _GetTimer10us(void);
int32_t _GetTimerms(void);



#endif