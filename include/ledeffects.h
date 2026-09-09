#ifndef __LEDEFFECTS_H__
    #define __LEDEFFECTS_H__


#include "main.h"
#include "nvstore.h"
#include "srtc.h"

typedef struct __attribute__((packed))
{
    uint8_t mode;
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
    
}stCurrentMode;

typedef enum
{
    _RGB_MODE_OFF = 0,
    _RGB_MODE_MANUAL,
    _RGB_MODE_AUTO,
    _RGB_MODE_FIXED,
    _RGB_MODE_FADE_IN,
    _RGB_MODE_FADE_OUT,
    _RGB_MODE_FADEIN_FADEOUT,
}ergbmode;  



void _SetRGBCurrentMode(uint8_t *st);
uint8_t _GetMDXSeq(void);
void _SetMDXSeq(uint8_t s);
void _SetRGBMode(uint8_t m);
void _SetCalendarEvent(stCalendarEvent *pst);
void _ProcModeAuto(rtc_soft_t *rtc);


#endif
