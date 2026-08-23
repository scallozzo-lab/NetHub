#include "srtc.h"

static rtc_soft_t sRTC = {0};

static uint8_t is_leap_year(uint16_t year)
{
    return ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
}

static uint8_t days_in_month(uint8_t month, uint16_t year)
{
    static const uint8_t days[12] = {
        31,28,31,30,31,30,31,31,30,31,30,31
    };

    if (month == 2 && is_leap_year(year))
        return 29;

    return days[month - 1];
}

rtc_soft_t *_GetRtcPtr(void)
{
    return &sRTC;    
}

void _SetsRTC(rtc_soft_t st)
{
    sRTC = st;
}

void RTC_Soft_Tick(rtc_soft_t *rtc)
{
    rtc->sec++;

    if (rtc->sec >= 60) {
        rtc->sec = 0;
        rtc->min++;

        if (rtc->min >= 60) {
            rtc->min = 0;
            rtc->hour++;

            if (rtc->hour >= 24) {
                rtc->hour = 0;
                rtc->day++;

                if (rtc->day > days_in_month(rtc->month, rtc->year)) {
                    rtc->day = 1;
                    rtc->month++;

                    if (rtc->month > 12) {
                        rtc->month = 1;
                        rtc->year++;
                    }
                }
            }
        }
    }

#ifdef _USE_DEBUG_SRTC
    // DEBUG: imprimir fecha y hora
    printf("FyH: %02d/%02d/%04d %02d:%02d:%02d\r\n",
           rtc->day,
           rtc->month,
           rtc->year,
           rtc->hour,
           rtc->min,
           rtc->sec);
#endif
}

uint8_t RTC_GetWeekDay(const rtc_soft_t *rtc)
{
    static const uint8_t t[] = {
        0, 3, 2, 5, 0, 3,
        5, 1, 4, 6, 2, 4
    };

    uint16_t y = rtc->year;

    if (rtc->month < 3)
        y--;

    return (y + y / 4 - y / 100 + y / 400 +
            t[rtc->month - 1] + rtc->day) % 7;
}

uint32_t RTC_Pack(const rtc_soft_t *rtc)
{
    uint32_t v = 0;

    uint32_t year = rtc->year - 2000;   // 0..63

    v |= (year        & 0x3F) << 26;
    v |= (rtc->month  & 0x0F) << 22;
    v |= (rtc->day    & 0x1F) << 17;
    v |= (rtc->hour   & 0x1F) << 12;
    v |= (rtc->min    & 0x3F) << 6;
    v |= (rtc->sec    & 0x3F);

    return v;
}

void RTC_Unpack(uint32_t v, rtc_soft_t *rtc)
{
    rtc->year  = 2000 + ((v >> 26) & 0x3F);
    rtc->month = (v >> 22) & 0x0F;
    rtc->day   = (v >> 17) & 0x1F;
    rtc->hour  = (v >> 12) & 0x1F;
    rtc->min   = (v >> 6)  & 0x3F;
    rtc->sec   =  v        & 0x3F;
}