#ifndef __PWM_H__
    #define  __PWM_H__

void _Init_PWM(uint32_t sysclk, uint32_t DutyCH1, uint32_t DutyCH4);
void _SetPWM_CH1(uint32_t v);
void _SetPWM_CH4(uint32_t v);


#endif
