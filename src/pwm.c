#include "main.h"
#include "pwm.h"

void _Init_PWM(uint32_t sysclk, uint32_t DutyCH1, uint32_t DutyCH4)
{
    //RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;   // Enable GPIOA clock
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;   // Enable TIM1 clock

    // PA8 (TIM1_CH1)
    GPIOA->CRH &= ~(GPIO_CRH_MODE8 | GPIO_CRH_CNF8);
    GPIOA->CRH |=  (GPIO_CRH_MODE8_1 | GPIO_CRH_MODE8_0);  // Output 50MHz
    GPIOA->CRH |=  (GPIO_CRH_CNF8_1);                      // AF Push-Pull (10)

    // PA11 (TIM1_CH4)
    GPIOA->CRH &= ~(GPIO_CRH_MODE11 | GPIO_CRH_CNF11);
    GPIOA->CRH |=  (GPIO_CRH_MODE11_1 | GPIO_CRH_MODE11_0); // Output 50MHz
    GPIOA->CRH |=  (GPIO_CRH_CNF11_1);                      // AF Push-Pull (10)    

    /* 
    PWM frequency = f_timer / (PSC + 1) / (ARR + 1)
    Example:
    PSC = 71 → timer tick = 1 MHz
    ARR = 999 → 1 kHz PWM period
    */

    TIM1->PSC = sysclk - 1;  // Prescaler (xMHz/x = 1MHz)
    TIM1->ARR = 999;         // Auto-reload value → 1kHz PWM
    
    TIM1->CCR1 = DutyCH1;        // Duty  (250) = 25% 
    TIM1->CCR4 = DutyCH4;        // Duty  (750) = 75%

    // Channel 1 config (OC1M = 110, OC1PE = 1)
    TIM1->CCMR1 &= ~TIM_CCMR1_OC1M;
    TIM1->CCMR1 |=  (6 << TIM_CCMR1_OC1M_Pos);  // PWM mode 1
    TIM1->CCMR1 |=  TIM_CCMR1_OC1PE;            // Preload enable

    // Channel 4 config (OC4M = 110, OC4PE = 1)
    TIM1->CCMR2 &= ~TIM_CCMR2_OC4M;
    TIM1->CCMR2 |=  (6 << TIM_CCMR2_OC4M_Pos);
    TIM1->CCMR2 |=  TIM_CCMR2_OC4PE;

    TIM1->CCER |= TIM_CCER_CC1E;   // Enable CH1 output
    TIM1->CCER |= TIM_CCER_CC4E;   // Enable CH4 output
    TIM1->BDTR |= TIM_BDTR_MOE;    // Main output enable

    TIM1->CR1 |= TIM_CR1_CEN;  // Counter enable
}

void _SetPWM_CH1(uint32_t v)
{
    TIM1->CCR1 = v;     // Duty  (250) = 25% 
} 

void _SetPWM_CH4(uint32_t v)
{
    TIM1->CCR4 = v;     // Duty  (250) = 25% 
} 