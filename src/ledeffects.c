
#include "ledeffects.h"
#include "_dmx512.h"
#include "srtc.h"
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


static LED_EFFECT_t ledEffect = LED_EFFECT_OFF;//LED_EFFECT_FIXED;//LED_EFFECT_FADE_IN_OUT;

static uint16_t effectCounter = 0;
static uint16_t effectDuration = 100; // 100 x 10ms = 1 segundo


void _SetCalendarEvent(stCalendarEvent *pst)
{
    stEffects *p_stnv = _GetNVEffectsPtr();
     
    if (pst)
    {    
        memcpy(p_stnv->CalendarEvent, pst, sizeof(p_stnv->CalendarEvent));
        _NVEffectsWrite();
    }
#ifdef _USE_DEBUG_CALENDAR_DATA

    printf("\r\n[_SetCalendarEvent] Calendar Events\r\n");

    for (uint8_t i = 0; i < _MAXEFFECTEVENTS; i++)
    {
        printf(
            "[%u] EN:%u "
            "START:%02u:%02u "
            "END:%02u:%02u "
            "DAYS:0x%02X "
            "ACT:%u "
            "G1:(%u,%u,%u) "
            "G2:(%u,%u,%u) "
            "G3:(%u,%u,%u) "
            "DIM:%u\r\n",

            i,
            p_stnv->CalendarEvent[i].enabled,

            p_stnv->CalendarEvent[i].start_hour,
            p_stnv->CalendarEvent[i].start_minute,

            p_stnv->CalendarEvent[i].end_hour,
            p_stnv->CalendarEvent[i].end_minute,

            p_stnv->CalendarEvent[i].days_mask,
            p_stnv->CalendarEvent[i].action,

            p_stnv->CalendarEvent[i].r_g1,
            p_stnv->CalendarEvent[i].g_g1,
            p_stnv->CalendarEvent[i].b_g1,

            p_stnv->CalendarEvent[i].r_g2,
            p_stnv->CalendarEvent[i].g_g2,
            p_stnv->CalendarEvent[i].b_g2,

            p_stnv->CalendarEvent[i].r_g3,
            p_stnv->CalendarEvent[i].g_g3,
            p_stnv->CalendarEvent[i].b_g3,

            p_stnv->CalendarEvent[i].dimming
        );
    }

    printf("\r\n");

#endif
}

void _SetRGBCurrentMode(uint8_t *st)
{
    memcpy(&_RGBCurrentMode, st, sizeof(_RGBCurrentMode));
    if(_RGBCurrentMode.mode == _RGB_MODE_OFF) ledEffect = LED_EFFECT_OFF; 
    // Forzado de modo manual
    else ledEffect = LED_EFFECT_FIXED;

    stEffects *p_stnv = _GetNVEffectsPtr();
   
    if(p_stnv)
    {
        p_stnv->mode = _RGBCurrentMode.mode;
        _NVEffectsWrite();  
    }
}

void _SetRGBMode(uint8_t m)
{
    _RGBCurrentMode.mode = m;
    stEffects *p_stnv = _GetNVEffectsPtr();
   
    if(p_stnv)
    {
        p_stnv->mode = m;
        _NVEffectsWrite();  
    }
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
    static uint16_t framecount = 0;

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
 
    // Si el contenido cambió o pasaron mas de N segundos, lo envía a la controladora
    if(memcmp(dmx, dmxold, 3) || (framecount++ >= 1000) )
    {
        memcpy(dmxold, dmx, sizeof(dmxold));
        DMX_SendFrame(0, dmx, sizeof(dmx));
        framecount = 0;
    }
}


void _ProcModeAuto(rtc_soft_t *rtc)
{
    stEffects *p_stnv = _GetNVEffects();

    if (!rtc || !p_stnv)
        return;
    // Si estamos en modo automático
    if (p_stnv->mode == _RGB_MODE_AUTO)
    {
        uint16_t now_min =
            ((uint16_t)rtc->hour * 60U) + rtc->min;

        uint8_t event_active = 0;

        for (int idx = 0; idx < _MAXEFFECTEVENTS; idx++)
        {
            stCalendarEvent *ev = &p_stnv->CalendarEvent[idx];

            if (!ev->enabled)
                continue;

            /*
             * OJO:
             * si days_mask es realmente una máscara de bits,
             * acá no debería ser rtc->day == days_mask.
             */
            if (rtc->day != ev->days_mask)
                continue;

            uint16_t start_min =
                ((uint16_t)ev->start_hour * 60U) +
                ev->start_minute;

            uint16_t end_min =
                ((uint16_t)ev->end_hour * 60U) +
                ev->end_minute;

            if ((now_min >= start_min) &&
                (now_min < end_min))
            {
                event_active = 1;

                // Aquí cargamos la estructura con los efectos
                // _SetEffect(ev);
                printf("********************** inicio del efecto\n");

                break;
            }
        }

        if (!event_active)
        {
            printf("********************** FIN del efecto\n");
            // Aquí apagamos el efecto
        }
    }
}
