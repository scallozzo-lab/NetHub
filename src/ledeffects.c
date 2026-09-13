
#include "ledeffects.h"
#include "_dmx512.h"
#include "srtc.h"
#include "_ltproto.h"
#include <string.h>

static uint8_t dmx[_DMX512_LEN] = {0}, dmxold[_DMX512_LEN] = {0};
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
static uint16_t effectDuration = 1000; // 100 x 10ms = 1 segundo
static uint16_t effectDuration_FI = 10000; // 100 x 10ms = 1 segundo
static uint16_t effectDuration_FO = 10000; // 100 x 10ms = 1 segundo


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
        printf("\r\nEVENT [%u]\r\n", i);

        // -----------------------------------------------------
        // Estado y horarios
        // -----------------------------------------------------
        printf("  EN      : %u\r\n",
            p_stnv->CalendarEvent[i].enabled);

        printf("  T1      : %02u:%02u -> %02u:%02u\r\n",
            p_stnv->CalendarEvent[i].start_hour,
            p_stnv->CalendarEvent[i].start_minute,
            p_stnv->CalendarEvent[i].end_hour,
            p_stnv->CalendarEvent[i].end_minute);

        printf("  EN T2   : %u\r\n",
            p_stnv->CalendarEvent[i].enabled_t2);

        printf("  T2      : %02u:%02u -> %02u:%02u\r\n",
            p_stnv->CalendarEvent[i].start_hour_t2,
            p_stnv->CalendarEvent[i].start_minute_t2,
            p_stnv->CalendarEvent[i].end_hour_t2,
            p_stnv->CalendarEvent[i].end_minute_t2);

        // -----------------------------------------------------
        // Configuración
        // -----------------------------------------------------
        printf("  DAYS    : 0x%02X\r\n",
            p_stnv->CalendarEvent[i].days_mask);

        printf("  ACTION  : %u\r\n",
            p_stnv->CalendarEvent[i].action);

        // -----------------------------------------------------
        // RGBW
        // -----------------------------------------------------
        printf("  G1 RGBW : %3u %3u %3u %3u\r\n",
            p_stnv->CalendarEvent[i].r_g1,
            p_stnv->CalendarEvent[i].g_g1,
            p_stnv->CalendarEvent[i].b_g1,
            p_stnv->CalendarEvent[i].w_g1);

        printf("  G2 RGBW : %3u %3u %3u %3u\r\n",
            p_stnv->CalendarEvent[i].r_g2,
            p_stnv->CalendarEvent[i].g_g2,
            p_stnv->CalendarEvent[i].b_g2,
            p_stnv->CalendarEvent[i].w_g2);

        printf("  G3 RGBW : %3u %3u %3u %3u\r\n",
            p_stnv->CalendarEvent[i].r_g3,
            p_stnv->CalendarEvent[i].g_g3,
            p_stnv->CalendarEvent[i].b_g3,
            p_stnv->CalendarEvent[i].w_g3);

        printf("  G4 RGBW : %3u %3u %3u %3u\r\n",
            p_stnv->CalendarEvent[i].r_g4,
            p_stnv->CalendarEvent[i].g_g4,
            p_stnv->CalendarEvent[i].b_g4,
            p_stnv->CalendarEvent[i].w_g4);

        // -----------------------------------------------------
        // Reflectores
        // -----------------------------------------------------
        printf("  REF1    : EN:%u ON:%u\r\n",
            p_stnv->CalendarEvent[i].reflector1_enable,
            p_stnv->CalendarEvent[i].reflector1_on);

        printf("  REF2    : EN:%u ON:%u\r\n",
            p_stnv->CalendarEvent[i].reflector2_enable,
            p_stnv->CalendarEvent[i].reflector2_on);

        // -----------------------------------------------------
        // Dimming
        // -----------------------------------------------------
        printf("  DIMMING : %u\r\n",
            p_stnv->CalendarEvent[i].dimming);
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

/*
void _SetRGBEffect(stCurrentMode *st, uint8_t mode)
{
    if(st)
        memcpy(&_RGBCurrentMode, st, sizeof(_RGBCurrentMode));
    ledEffect = mode;
}
    */

void _SetRGBEffect(stCurrentMode *st, uint8_t mode)
{
    if(st)
        memcpy(&_RGBCurrentMode, st, sizeof(_RGBCurrentMode));

    if (ledEffect != mode)
    {
        ledEffect = mode;
        effectCounter = 0;
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
    //uint8_t value;
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
            dmx[3] = _RGBCurrentMode.rgbg1_w;
        break;

        case LED_EFFECT_FADE_IN:
        {
            uint8_t fade;

            /*
                * effectCounter:
                *      0 -------------------- effectDuration
                *
                * fade:
                *      0 -------------------- 255
                */
            if (effectCounter < effectDuration_FI)
            {
                effectCounter++;
            }

            fade = (uint8_t)(
                ((uint32_t)effectCounter * 255UL) /
                effectDuration_FI
            );

            /*
                * Fade desde negro hasta el RGB configurado.
                *
                * Ejemplo RGB = (255, 100, 20)
                *
                * fade =   0 -> (  0,  0,  0)
                * fade = 128 -> (128, 50, 10)
                * fade = 255 -> (255,100, 20)
                */
            dmx[0] = (uint8_t)(
                ((uint32_t)_RGBCurrentMode.rgbg1_r * fade) / 255UL
            );

            dmx[1] = (uint8_t)(
                ((uint32_t)_RGBCurrentMode.rgbg1_g * fade) / 255UL
            );

            dmx[2] = (uint8_t)(
                ((uint32_t)_RGBCurrentMode.rgbg1_b * fade) / 255UL
            );

            dmx[3] = 0;

            break;
        }

        case LED_EFFECT_FADE_OUT:
        {
            uint8_t fade;

            /*
            * 255 -> 0
            *
            * Al comenzar:
            * effectCounter = 0
            * fade = 255
            *
            * Al terminar:
            * effectCounter = effectDuration
            * fade = 0
            */
            fade = 255U - (uint8_t)(
                ((uint32_t)effectCounter * 255UL) /
                effectDuration_FO
            );

            /*
            * Grupo 1 RGB:
            * color configurado -> negro
            */
            dmx[0] = (uint8_t)(
                ((uint32_t)_RGBCurrentMode.rgbg1_r * fade) / 255UL
            );

            dmx[1] = (uint8_t)(
                ((uint32_t)_RGBCurrentMode.rgbg1_g * fade) / 255UL
            );

            dmx[2] = (uint8_t)(
                ((uint32_t)_RGBCurrentMode.rgbg1_b * fade) / 255UL
            );

            dmx[3] = 0;

            if (effectCounter < effectDuration_FO)
            {
                effectCounter++;
            }

            break;
        }
   
/*
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
 */
    case LED_EFFECT_FADE_IN_OUT:
    {
        uint8_t fade;

        /*
        * effectCounter recorre:
        *
        * 0 ---------------- effectDuration ---------------- 2*effectDuration
        *
        *        FADE IN                         FADE OUT
        *
        * fade:
        * 0 ---------> 255                 255 ---------> 0
        */

        if (effectCounter < effectDuration)
        {
            /*
            * FADE IN
            * 0 -> 255
            */
            fade = (uint8_t)(
                ((uint32_t)effectCounter * 255UL) /
                effectDuration
            );
        }
        else
        {
            /*
            * FADE OUT
            * 255 -> 0
            */
            uint16_t outCounter =
                effectCounter - effectDuration;

            fade = 255U - (uint8_t)(
                ((uint32_t)outCounter * 255UL) /
                effectDuration
            );
        }

        /*
        * Aplicamos el fade sobre el RGB programado.
        */
        dmx[0] = (uint8_t)(
            ((uint32_t)_RGBCurrentMode.rgbg1_r * fade) / 255UL
        );

        dmx[1] = (uint8_t)(
            ((uint32_t)_RGBCurrentMode.rgbg1_g * fade) / 255UL
        );

        dmx[2] = (uint8_t)(
            ((uint32_t)_RGBCurrentMode.rgbg1_b * fade) / 255UL
        );

        dmx[3] = 0;

        /*
        * Avanzamos el efecto.
        * Cuando termina el FADE OUT volvemos a empezar.
        */
        effectCounter++;

        if (effectCounter >= (effectDuration * 2U))
        {
            effectCounter = 0;
        }

        break;
    }

    }

    // Si el contenido cambió o pasaron mas de N segundos, lo envía a la controladora
    if(memcmp(dmx, dmxold, sizeof(dmx)) || (framecount++ >= 1000) )
    {
        memcpy(dmxold, dmx, sizeof(dmxold));
        DMX_SendFrame(0, dmx, sizeof(dmx));
        framecount = 0;
    }
}


void _ProcModeAuto(rtc_soft_t *rtc)
{
    static int8_t lastevent = -1;
    stEffects *p_stnv = _GetNVEffects();

    if (!rtc || !p_stnv)
        return;

    if(!(_GetHubStatus() & HUB_STS_DTIME_SYNCRO_OK))
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
            * Verificar si el evento corresponde al día actual.
            * rtc->day: 1 = LU ... 7 = DO
            * days_mask: BIT0 = LU ... BIT6 = DO
            */
            uint8_t day_bit = (1U << (RTC_GetWeekDay(rtc) - 1));

            //printf("day = %d, bit %02X\n", RTC_GetWeekDay(rtc), day_bit);

            if ((ev->days_mask & day_bit) == 0)
            {
                continue;
            }
            
            uint16_t start_min =
                ((uint16_t)ev->start_hour * 60U) +
                ev->start_minute;

            uint16_t end_min =
                ((uint16_t)ev->end_hour * 60U) +
                ev->end_minute;

            if ((now_min >= start_min) &&
                (now_min <= end_min))
            {
                event_active = 1;

                // Aquí cargamos la estructura con los efectos
                stCurrentMode st;
                st.mode = _RGB_MODE_AUTO;
                st.rgbg1_enable = true;
                st.rgbg1_r = ev->r_g1;
                st.rgbg1_g = ev->g_g1;
                st.rgbg1_b = ev->b_g1;
                
                st.rgbg2_enable = true;
                st.rgbg2_r = ev->r_g2;
                st.rgbg2_g = ev->g_g2;
                st.rgbg2_b = ev->b_g2;
              
                st.rgbg3_enable = true;
                st.rgbg3_r = ev->r_g3;
                st.rgbg3_g = ev->g_g3;
                st.rgbg3_b = ev->b_g3;
                
                _SetRGBEffect(&st, ev->action);
                lastevent = ev->action;

                printf("**inicio del efecto now_min %d, start_min %d, end_min %d\n", now_min, start_min, end_min);
                printf("Efecto %X\n", ev->action);
                break;
            }
        }

        if (!event_active)
        {
            if (lastevent >= 0)
            {
                stCalendarEvent *ev =
                    &p_stnv->CalendarEvent[lastevent];
                /*
                * Si el evento que acaba de terminar era FADE_OUT,
                * iniciamos ahora el apagado progresivo.
                */
                if (ev->action == LED_EFFECT_FADE_OUT)
                {
                    _SetRGBEffect(NULL, LED_EFFECT_FADE_OUT);
                }
                else
                {
                    _SetRGBEffect(NULL, LED_EFFECT_OFF);
                }

                lastevent = -1;
            }
        }
    }
}
