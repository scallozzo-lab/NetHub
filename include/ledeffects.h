#ifndef __LEDEFFECTS_H__
    #define __LEDEFFECTS_H__

#include "main.h"


typedef struct __attribute__((packed))
{
    uint8_t  enabled;

    uint8_t  start_hour;
    uint8_t  start_minute;
    uint8_t  end_hour;
    uint8_t  end_minute;

    uint8_t  days_mask;

    uint8_t action;

    uint8_t  r_g1;
    uint8_t  g_g1;
    uint8_t  b_g1;
    uint8_t  r_g2;
    uint8_t  g_g2;
    uint8_t  b_g2;
    uint8_t  r_g3;
    uint8_t  g_g3;
    uint8_t  b_g3;

    uint8_t  dimming;

} stCalendarEvent;

typedef struct __attribute__((packed))
{
    bool rgbg1_enable;
    uint8_t rgbg1_r;
    uint8_t rgbg1_g;
    uint8_t rgbg1_b;

    bool rgbg2_enable;
    uint8_t rgbg2_r;
    uint8_t rgbg2_g;
    uint8_t rgbg2_b;
    
    bool rgbg3_enable;
    uint8_t rgbg3_r;
    uint8_t rgbg3_g;
    uint8_t rgbg3_b;
    
    uint8_t mode;
}stCurrentMode;

void _SetRGBCurrentMode(uint8_t *st);
uint8_t _GetMDXSeq(void);
void _SetMDXSeq(uint8_t s);


#endif
