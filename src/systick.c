#include "main.h"

static volatile uint32_t Ticks1us = 0;


void SysTick_Init(uint32_t sysclk)
{
    // sysclk_hz = frecuencia del CPU en Hz (por ejemplo 72,000,000)
    uint32_t ticks_per_us = sysclk / 1000000;
    SysTick->LOAD = 7200;//ticks_per_us - 10;   // 10 µs exacto
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |   // CPU clock
                    SysTick_CTRL_TICKINT_Msk   |   // habilitar interrupción
                    SysTick_CTRL_ENABLE_Msk;       // habilitar SysTick
}

uint32_t _GetTickSys(void)
{
    return Ticks1us;    
}

void SysTick_Handler(void)
{
    // Aquí pondrás
    Ticks1us++;
}
