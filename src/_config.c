#include "_config.h"

void _InitConfigPin(void)
{
    // Habilitar clock GPIOC
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;

    // PC14 input pull-up/pull-down
    GPIOC->CRH &= ~(0xFU << 24);
    GPIOC->CRH |=  (0x8U << 24);  // MODE=00, CNF=10

    // Seleccionar pull-up
    GPIOC->ODR |= (1U << 14);
}

uint8_t _ReadConfigInput(void)
{
    return (GPIOC->IDR & (1U << 14)) ? 1U : 0U;
}