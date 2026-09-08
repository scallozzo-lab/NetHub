
#include "ledeffects.h"
#include "_dmx512.h"
#include <string.h>

static uint8_t dmx[4] = {0}, dmxold[4] = {0};
static uint8_t MdxSeq = 0;
static stCurrentMode _RGBCurrentMode = {0};



typedef enum
{
    LED_EFFECT_OFF = 0,
    LED_EFFECT_FIXED,
    LED_EFFECT_FADE_IN,
    LED_EFFECT_FADE_OUT,
    LED_EFFECT_FADE_IN_OUT
} LED_EFFECT_t;


static LED_EFFECT_t ledEffect = LED_EFFECT_FIXED;//LED_EFFECT_FADE_IN_OUT;

static uint16_t effectCounter = 0;
static uint16_t effectDuration = 100; // 100 x 10ms = 1 segundo


void _SetRGBCurrentMode(uint8_t *st)
{
    memcpy(&_RGBCurrentMode, st, sizeof(_RGBCurrentMode));
}

uint8_t _GetMDXSeq(void)
{
    return MdxSeq;
}

void _SetMDXSeq(uint8_t s)
{
    MdxSeq = s;
}

void _ProcLEDEffect(void)
{
    uint8_t value;

    switch (ledEffect)
    {
        case LED_EFFECT_OFF:

            dmx[0] = 0;
            dmx[1] = 0;
            dmx[2] = 0;
            dmx[3] = 0;
            break;

        case LED_EFFECT_FIXED:
            dmx[0] = _RGBCurrentMode.rgbg1_r;
            dmx[1] = _RGBCurrentMode.rgbg1_g;
            dmx[2] = _RGBCurrentMode.rgbg1_b;
            dmx[3] = 0;
        break;


        case LED_EFFECT_FADE_IN:

            if (effectCounter < effectDuration)
                effectCounter++;

            value = (uint8_t)((effectCounter * 255UL) / effectDuration);

            dmx[0] = value;
            dmx[1] = value;
            dmx[2] = value;

            break;


        case LED_EFFECT_FADE_OUT:

            if (effectCounter < effectDuration)
                effectCounter++;

            value = 255 -
                    (uint8_t)((effectCounter * 255UL) / effectDuration);

            dmx[0] = value;
            dmx[1] = value;
            dmx[2] = value;

            break;

   
    case LED_EFFECT_FADE_IN_OUT:

        effectCounter++;

        if (effectCounter >= effectDuration)
            effectCounter = 0;

        if (effectCounter <= effectDuration / 2)
        {
            value = (uint8_t)(
                (effectCounter * 2UL * 255UL) /
                effectDuration
            );
        }
        else
        {
            value = (uint8_t)(
                ((effectDuration - effectCounter) * 2UL * 255UL) /
                effectDuration
            );
        }

        dmx[0] = value;
        dmx[1] = value;
        dmx[2] = value;
        dmx[3] = value;

        break;
    }
 
    // Si el contenido cambió lo envía a la controladora
    if(memcmp(dmx, dmxold, 3))
    {
        memcpy(dmxold, dmx, sizeof(dmxold));
        DMX_SendFrame(0, dmx, sizeof(dmx));
    }
}
