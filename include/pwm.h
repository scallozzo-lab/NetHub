#ifndef __PWM_H__
    #define  __PWM_H__

void _Init_PWM(uint32_t sysclk, uint32_t DutyCH1, uint32_t DutyCH4);
void _SetPWM_CH1(uint32_t v);
void _SetPWM_CH4(uint32_t v);

#define _SET_OFF_RELE_K1 _SetPWM_CH4(0xffffffflu)
#define _SET_ON_RELE_K1 _SetPWM_CH4(0x0lu)
#define _SET_OFF_RELE_K2 _SetPWM_CH1(0xffffffflu)
#define _SET_ON_RELE_K2 _SetPWM_CH1(0x0lu)
    
    //_SetPWM_CH4(0x0lu);


#endif
