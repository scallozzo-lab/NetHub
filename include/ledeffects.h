#ifndef __LEDEFFECTS_H__
    #define __LEDEFFECTS_H__

#include "main.h"


typedef struct
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

uint8_t _GetMDXSeq(void);
void _SetMDXSeq(uint8_t s);


#endif
