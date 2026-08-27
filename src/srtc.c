#include "srtc.h"

static volatile rtc_soft_t sRTC = {0};

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

uint64_t RTC_ToTimestamp(const rtc_soft_t *rtc)
{
    uint64_t days = 0;
    uint16_t year;

    /* Días completos desde 1970 hasta el año anterior */
    for (year = 1970; year < rtc->year; year++)
    {
        days += ((year % 4 == 0 &&
                  (year % 100 != 0 || year % 400 == 0)) ? 366 : 365);
    }

    /* Días de los meses anteriores */
    for (uint8_t month = 1; month < rtc->month; month++)
    {
        days += days_in_month(month,rtc->year);
    }

    /* Días del mes actual */
    days += rtc->day - 1;

    return (days * 86400ULL) +
           ((uint64_t)rtc->hour * 3600ULL) +
           ((uint64_t)rtc->min  * 60ULL) +
           rtc->sec;
}

void Timestamp_ToRTC(uint64_t timestamp, rtc_soft_t *rtc)
{
    uint64_t days;
    uint32_t seconds;
    uint16_t year = 1970;

    days = timestamp / 86400ULL;
    seconds = timestamp % 86400ULL;

    rtc->hour = seconds / 3600;
    seconds %= 3600;

    rtc->min = seconds / 60;
    rtc->sec = seconds % 60;

    while (1)
    {
        uint16_t days_year =
            ((year % 4 == 0 &&
              (year % 100 != 0 || year % 400 == 0)) ? 366 : 365);

        if (days < days_year)
            break;

        days -= days_year;
        year++;
    }

    rtc->year = year;

    rtc->month = 1;

    while (days >= days_in_month(rtc->month, year))
    {
        days -= days_in_month(rtc->month, year);
        rtc->month++;
    }

    rtc->day = days + 1;
}

rtc_soft_t *_GetIntRtcPtr(void)
{
    return &sRTC;    
}

rtc_soft_t *_GetRtcPtr(void)
{
    static rtc_soft_t rtc = {0};
 
    __disable_irq();
    rtc = sRTC;
     __enable_irq();
    return &rtc;    
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

/*
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
*/
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

#ifdef _USE_DUMMY_TEST_SRTC
    void testrtc(void)
    {
        rtc_soft_t rtc;
        rtc.day = 29;
        rtc.month = 2;
        rtc.year = 2026;
        rtc.hour = 10;
        rtc.min = 50;
        rtc.sec = 1;

        printf("GNSS RTC: %02u/%02u/%04u %02u:%02u:%02u\r\n",
                            rtc.day,
                            rtc.month,
                            rtc.year,
                            rtc.hour,
                            rtc.min,
                            rtc.sec);
                                                    
        int64_t tstamp = RTC_ToTimestamp(&rtc); 
        rtc_soft_t newrtc;
        
        printf("[_ProcSrvCom] RTC_ToTimestamp...%llu\n", (unsigned long long)tstamp);
        tstamp += TIMEZONE_ARGENTINA_SECONDS;
        printf("[_ProcSrvCom] UTC -3...%llu\n", (unsigned long long)tstamp);
        Timestamp_ToRTC((uint64_t)tstamp, &newrtc);
        
        printf("ROUNDTRIP  : %02u/%02u/%04u %02u:%02u:%02u\r\n",
            newrtc.day,
            newrtc.month,
            newrtc.year,
            newrtc.hour,
            newrtc.min,
            newrtc.sec);
    }
#endif