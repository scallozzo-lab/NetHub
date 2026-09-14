
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
            memset(dmx, 0,sizeof(dmx));
            break;

        case LED_EFFECT_FIXED:
            dmx[0] = _RGBCurrentMode.rgbg1_r;
            dmx[1] = _RGBCurrentMode.rgbg1_g;
            dmx[2] = _RGBCurrentMode.rgbg1_b;
            dmx[3] = _RGBCurrentMode.rgbg1_w;
        
            dmx[4] = _RGBCurrentMode.rgbg2_r;
            dmx[5] = _RGBCurrentMode.rgbg2_g;
            dmx[6] = _RGBCurrentMode.rgbg2_b;
            dmx[7] = _RGBCurrentMode.rgbg2_w;
        
            dmx[8] = _RGBCurrentMode.rgbg3_r;
            dmx[9] = _RGBCurrentMode.rgbg3_g;
            dmx[10] = _RGBCurrentMode.rgbg3_b;
            dmx[11] = _RGBCurrentMode.rgbg3_w;
        
            dmx[12] = _RGBCurrentMode.rgbg4_r;
            dmx[13] = _RGBCurrentMode.rgbg4_g;
            dmx[14] = _RGBCurrentMode.rgbg4_b;
            dmx[15] = _RGBCurrentMode.rgbg4_w;
        
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

            dmx[3] = (uint8_t)(
                ((uint32_t)_RGBCurrentMode.rgbg1_w * fade) / 255UL
            );

             dmx[4] = (uint8_t)(
                ((uint32_t)_RGBCurrentMode.rgbg2_r * fade) / 255UL
            );

            dmx[5] = (uint8_t)(
                ((uint32_t)_RGBCurrentMode.rgbg2_g * fade) / 255UL
            );

            dmx[6] = (uint8_t)(
                ((uint32_t)_RGBCurrentMode.rgbg2_b * fade) / 255UL
            );

            dmx[7] = (uint8_t)(
                ((uint32_t)_RGBCurrentMode.rgbg2_w * fade) / 255UL
            );

            dmx[8] = (uint8_t)(
                ((uint32_t)_RGBCurrentMode.rgbg3_r * fade) / 255UL
            );

            dmx[9] = (uint8_t)(
                ((uint32_t)_RGBCurrentMode.rgbg3_g * fade) / 255UL
            );

            dmx[10] = (uint8_t)(
                ((uint32_t)_RGBCurrentMode.rgbg3_b * fade) / 255UL
            );

            dmx[11] = (uint8_t)(
                ((uint32_t)_RGBCurrentMode.rgbg3_w * fade) / 255UL
            );

            dmx[12] = (uint8_t)(
                ((uint32_t)_RGBCurrentMode.rgbg4_r * fade) / 255UL
            );

            dmx[13] = (uint8_t)(
                ((uint32_t)_RGBCurrentMode.rgbg4_g * fade) / 255UL
            );

            dmx[14] = (uint8_t)(
                ((uint32_t)_RGBCurrentMode.rgbg4_b * fade) / 255UL
            );

            dmx[15] = (uint8_t)(
                ((uint32_t)_RGBCurrentMode.rgbg4_w * fade) / 255UL
            );


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

            dmx[3] = (uint8_t)(
                ((uint32_t)_RGBCurrentMode.rgbg1_w * fade) / 255UL
            );

             dmx[4] = (uint8_t)(
                ((uint32_t)_RGBCurrentMode.rgbg2_r * fade) / 255UL
            );

            dmx[5] = (uint8_t)(
                ((uint32_t)_RGBCurrentMode.rgbg2_g * fade) / 255UL
            );

            dmx[6] = (uint8_t)(
                ((uint32_t)_RGBCurrentMode.rgbg2_b * fade) / 255UL
            );

            dmx[7] = (uint8_t)(
                ((uint32_t)_RGBCurrentMode.rgbg2_w * fade) / 255UL
            );

            dmx[8] = (uint8_t)(
                ((uint32_t)_RGBCurrentMode.rgbg3_r * fade) / 255UL
            );

            dmx[9] = (uint8_t)(
                ((uint32_t)_RGBCurrentMode.rgbg3_g * fade) / 255UL
            );

            dmx[10] = (uint8_t)(
                ((uint32_t)_RGBCurrentMode.rgbg3_b * fade) / 255UL
            );

            dmx[11] = (uint8_t)(
                ((uint32_t)_RGBCurrentMode.rgbg3_w * fade) / 255UL
            );

            dmx[12] = (uint8_t)(
                ((uint32_t)_RGBCurrentMode.rgbg4_r * fade) / 255UL
            );

            dmx[13] = (uint8_t)(
                ((uint32_t)_RGBCurrentMode.rgbg4_g * fade) / 255UL
            );

            dmx[14] = (uint8_t)(
                ((uint32_t)_RGBCurrentMode.rgbg4_b * fade) / 255UL
            );

            dmx[15] = (uint8_t)(
                ((uint32_t)_RGBCurrentMode.rgbg4_w * fade) / 255UL
            );


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

        dmx[3] = (uint8_t)(
            ((uint32_t)_RGBCurrentMode.rgbg1_w * fade) / 255UL
        );

        dmx[4] = (uint8_t)(
            ((uint32_t)_RGBCurrentMode.rgbg2_r * fade) / 255UL
        );

        dmx[5] = (uint8_t)(
            ((uint32_t)_RGBCurrentMode.rgbg2_g * fade) / 255UL
        );

        dmx[6] = (uint8_t)(
            ((uint32_t)_RGBCurrentMode.rgbg2_b * fade) / 255UL
        );

        dmx[7] = (uint8_t)(
            ((uint32_t)_RGBCurrentMode.rgbg2_w * fade) / 255UL
        );

        dmx[8] = (uint8_t)(
            ((uint32_t)_RGBCurrentMode.rgbg3_r * fade) / 255UL
        );

        dmx[9] = (uint8_t)(
            ((uint32_t)_RGBCurrentMode.rgbg3_g * fade) / 255UL
        );

        dmx[10] = (uint8_t)(
            ((uint32_t)_RGBCurrentMode.rgbg3_b * fade) / 255UL
        );

        dmx[11] = (uint8_t)(
            ((uint32_t)_RGBCurrentMode.rgbg3_w * fade) / 255UL
        );

        dmx[12] = (uint8_t)(
            ((uint32_t)_RGBCurrentMode.rgbg4_r * fade) / 255UL
        );

        dmx[13] = (uint8_t)(
            ((uint32_t)_RGBCurrentMode.rgbg4_g * fade) / 255UL
        );

        dmx[14] = (uint8_t)(
            ((uint32_t)_RGBCurrentMode.rgbg4_b * fade) / 255UL
        );

        dmx[15] = (uint8_t)(
            ((uint32_t)_RGBCurrentMode.rgbg4_w * fade) / 255UL
        );

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

/*
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
        printf("modo auto\n");
        
        uint16_t now_min =
            ((uint16_t)rtc->hour * 60U) + rtc->min;

        uint8_t event_active = 0;

        for (int idx = 0; idx < _MAXEFFECTEVENTS; idx++)
        {
            stCalendarEvent *ev = &p_stnv->CalendarEvent[idx];

            if (!ev->enabled)
                continue;

                uint8_t day_bit = (1U << (RTC_GetWeekDay(rtc) - 1));

            printf("day_bit %02X - mask day %02X getweek %d\n", day_bit, ev->days_mask, RTC_GetWeekDay(rtc));
    

            //printf("day = %d, bit %02X\n", RTC_GetWeekDay(rtc), day_bit);

            if ((ev->days_mask & day_bit) == 0)
            {
                continue;
            }
    
            printf("dia valido\n");
    

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
                st.rgbg1_w = ev->w_g1;
                
                st.rgbg2_enable = true;
                st.rgbg2_r = ev->r_g2;
                st.rgbg2_g = ev->g_g2;
                st.rgbg2_b = ev->b_g2;
                st.rgbg2_w = ev->w_g2;
              
                st.rgbg3_enable = true;
                st.rgbg3_r = ev->r_g3;
                st.rgbg3_g = ev->g_g3;
                st.rgbg3_b = ev->b_g3;
                st.rgbg3_w = ev->w_g3;
                
                st.rgbg4_enable = true;
                st.rgbg4_r = ev->r_g4;
                st.rgbg4_g = ev->g_g4;
                st.rgbg4_b = ev->b_g4;
                st.rgbg4_w = ev->w_g4;
           
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
*/

void _ProcModeAuto(rtc_soft_t *rtc)
{
    static int8_t lastevent = -1;

    stEffects *p_stnv = _GetNVEffects();

    if (!rtc || !p_stnv)
        return;

    if (!(_GetHubStatus() & HUB_STS_DTIME_SYNCRO_OK))
        return;

    // Solo procesamos calendario en modo automático
    if (p_stnv->mode != _RGB_MODE_AUTO)
        return;


    uint8_t weekday = RTC_GetWeekDay(rtc);

    // 0 = fecha inválida
    if (weekday == 0)
        return;

    /*
     * RTC_GetWeekDay:
     * 1..7
     *
     * days_mask:
     * BIT0 .. BIT6
     */
    uint8_t day_bit = (uint8_t)(1U << (weekday - 1U));

    uint16_t now_min =
        ((uint16_t)rtc->hour * 60U) +
        (uint16_t)rtc->min;

    uint8_t event_active = 0;


#ifdef _USE_DEBUG_CALENDAR_DATA
    printf("\r\n[_ProcModeAuto]\r\n");
    printf("Time     : %02u:%02u\r\n",
           rtc->hour,
           rtc->min);

    printf("WeekDay  : %u\r\n", weekday);
    printf("Day Bit  : 0x%02X\r\n", day_bit);
#endif

    for (int idx = 0; idx < _MAXEFFECTEVENTS; idx++)
    {
        stCalendarEvent *ev = &p_stnv->CalendarEvent[idx];

        // Evento deshabilitado
        if (!ev->enabled)
            continue;


        // -------------------------------------------------
        // Día válido
        // -------------------------------------------------
        if ((ev->days_mask & day_bit) == 0)
            continue;


        // -------------------------------------------------
        // Primera franja horaria T1
        // -------------------------------------------------
        uint16_t start_min_t1 =
            ((uint16_t)ev->start_hour * 60U) +
            (uint16_t)ev->start_minute;

        uint16_t end_min_t1 =
            ((uint16_t)ev->end_hour * 60U) +
            (uint16_t)ev->end_minute;


        uint8_t active_t1 = 0;
        uint8_t active_t2 = 0;


        if ((now_min >= start_min_t1) &&
            (now_min <= end_min_t1))
        {
            active_t1 = 1;
        }


        // -------------------------------------------------
        // Segunda franja horaria T2
        // -------------------------------------------------
        if (ev->enabled_t2)
        {
            uint16_t start_min_t2 =
                ((uint16_t)ev->start_hour_t2 * 60U) +
                (uint16_t)ev->start_minute_t2;

            uint16_t end_min_t2 =
                ((uint16_t)ev->end_hour_t2 * 60U) +
                (uint16_t)ev->end_minute_t2;


            if ((now_min >= start_min_t2) &&
                (now_min <= end_min_t2))
            {
                active_t2 = 1;
            }


#ifdef _USE_DEBUG_CALENDAR_DATA

            printf(
                "Event[%d] T1:%02u:%02u-%02u:%02u "
                "T2:%02u:%02u-%02u:%02u "
                "ACT1:%u ACT2:%u\r\n",

                idx,

                ev->start_hour,
                ev->start_minute,
                ev->end_hour,
                ev->end_minute,

                ev->start_hour_t2,
                ev->start_minute_t2,
                ev->end_hour_t2,
                ev->end_minute_t2,

                active_t1,
                active_t2
            );

#endif
        }
#ifdef _USE_DEBUG_CALENDAR_DATA
        else
        {
            printf(
                "Event[%d] T1:%02u:%02u-%02u:%02u "
                "T2:DISABLED "
                "ACT1:%u\r\n",

                idx,

                ev->start_hour,
                ev->start_minute,
                ev->end_hour,
                ev->end_minute,

                active_t1
            );
        }
#endif


        // -------------------------------------------------
        // ¿Estamos dentro de alguna de las dos franjas?
        // -------------------------------------------------
        if (active_t1 || active_t2)
        {
            event_active = 1;

            stCurrentMode st = {0};

            st.mode = _RGB_MODE_AUTO;


            // -------------------------------------------------
            // Grupo 1
            // -------------------------------------------------
            st.rgbg1_enable = true;
            st.rgbg1_r = ev->r_g1;
            st.rgbg1_g = ev->g_g1;
            st.rgbg1_b = ev->b_g1;
            st.rgbg1_w = ev->w_g1;


            // -------------------------------------------------
            // Grupo 2
            // -------------------------------------------------
            st.rgbg2_enable = true;
            st.rgbg2_r = ev->r_g2;
            st.rgbg2_g = ev->g_g2;
            st.rgbg2_b = ev->b_g2;
            st.rgbg2_w = ev->w_g2;


            // -------------------------------------------------
            // Grupo 3
            // -------------------------------------------------
            st.rgbg3_enable = true;
            st.rgbg3_r = ev->r_g3;
            st.rgbg3_g = ev->g_g3;
            st.rgbg3_b = ev->b_g3;
            st.rgbg3_w = ev->w_g3;


            // -------------------------------------------------
            // Grupo 4
            // -------------------------------------------------
            st.rgbg4_enable = true;
            st.rgbg4_r = ev->r_g4;
            st.rgbg4_g = ev->g_g4;
            st.rgbg4_b = ev->b_g4;
            st.rgbg4_w = ev->w_g4;


            // -------------------------------------------------
            // Aplicar efecto
            // -------------------------------------------------
            _SetRGBEffect(&st, ev->action);


            /*
             * IMPORTANTE:
             * Guardamos índice del evento, NO action.
             */
            lastevent = idx;


#ifdef _USE_DEBUG_CALENDAR_DATA

            printf("EVENT ACTIVE [%d]\r\n", idx);

            if (active_t1)
                printf("  Time Slot : T1\r\n");

            if (active_t2)
                printf("  Time Slot : T2\r\n");

            printf("  Action    : %u\r\n", ev->action);
            printf("  Now       : %u min\r\n", now_min);

#endif

            /*
             * El primer evento válido tiene prioridad.
             */
            break;
        }
    }


    // -----------------------------------------------------
    // No hay ningún evento activo
    // -----------------------------------------------------
    if (!event_active)
    {
        if (lastevent >= 0 &&
            lastevent < _MAXEFFECTEVENTS)
        {
            stCalendarEvent *ev =
                &p_stnv->CalendarEvent[lastevent];


#ifdef _USE_DEBUG_CALENDAR_DATA
            printf("EVENT END [%d] Action:%u\r\n",
                   lastevent,
                   ev->action);
#endif

            /*
             * Si el efecto era FADE_OUT, hacemos
             * el apagado progresivo al finalizar.
             */
            if (ev->action == LED_EFFECT_FADE_OUT)
            {
                _SetRGBEffect(NULL,
                              LED_EFFECT_FADE_OUT);
            }
            else
            {
                _SetRGBEffect(NULL,
                              LED_EFFECT_OFF);
            }


            lastevent = -1;
        }
    }
}