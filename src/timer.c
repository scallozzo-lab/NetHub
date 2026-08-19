#include "timer.h"



#include "stm32f1xx.h"

static
volatile uint32_t _MainTimerms = 0;

void _UpdateTimer(void)
{
    _MainTimerms++;
}

// Callback de usuario
void (*Timer2_Callback)(void) = 0;

// Inicializa TIM2 con base de fbasemhz y habilita interrupción
void _Timer2_Init_us(uint16_t fbasemhz, void (*callback)(void))
{
    Timer2_Callback = callback;

    // Habilitar reloj TIM2
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    // Reset TIM2
    TIM2->CR1 = 0;

    // Prescaler: 100us por tick aprox
    TIM2->PSC = (fbasemhz - 1) * 100; 

    // Auto-reload máximo 16 bits
    TIM2->ARR = 10;

    // Forzar actualización de prescaler/ARR
    TIM2->EGR = TIM_EGR_UG;

    // Habilitar interrupción por overflow
    TIM2->DIER |= TIM_DIER_UIE;

    // Arrancar timer
    TIM2->CR1 |= TIM_CR1_CEN;

    // NVIC
    NVIC_EnableIRQ(TIM2_IRQn);
    NVIC_SetPriority(TIM2_IRQn, 1);
}

// Cambiar periodo dinámicamente (opcional)
void Timer2_SetPeriod(uint16_t period)
{
    TIM2->ARR = period - 1;
    TIM2->EGR = TIM_EGR_UG;
}

// Handler de interrupción
void TIM2_IRQHandler(void)
{
    if (TIM2->SR & TIM_SR_UIF) // overflow
    {
        TIM2->SR &= ~TIM_SR_UIF; // limpiar flag
        if (Timer2_Callback) Timer2_Callback();
    }
}


// Leer contador actual
inline
int16_t _GetTimer10us(void)
{
    return (int16_t)TIM2->CNT;
}

int32_t _GetTimerms(void)
{
    return _MainTimerms;
}

void Timer2_Init_us(uint16_t fbasemhz)
{
    _Timer2_Init_us(fbasemhz, _UpdateTimer);
}
