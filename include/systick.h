#ifndef __SYSTICK_H__
    #define __SYSTICK_H__

#include "main.h"

void SysTick_Init(uint32_t sysclk);
uint32_t _GetTickSys(void);


#endif