#ifndef __SRTC_H__
    #define __SRTC_H__

#include "main.h"


typedef struct {
    uint8_t sec;    // 0-59
    uint8_t min;    // 0-59
    uint8_t hour;   // 0-23
    uint8_t day;    // 1-31
    uint8_t month;  // 1-12
    uint16_t year;  // ej: 2026
} rtc_soft_t;

typedef enum
{
    RTC_WEEKDAY_SUNDAY = 0,
    RTC_WEEKDAY_MONDAY,
    RTC_WEEKDAY_TUESDAY,
    RTC_WEEKDAY_WEDNESDAY,
    RTC_WEEKDAY_THURSDAY,
    RTC_WEEKDAY_FRIDAY,
    RTC_WEEKDAY_SATURDAY
} rtc_weekday_t;


rtc_soft_t *_GetRtcPtr(void);
void _SetsRTC(rtc_soft_t st);
void RTC_Soft_Tick(rtc_soft_t *rtc);
uint8_t RTC_GetWeekDay(const rtc_soft_t *rtc);
uint32_t RTC_Pack(const rtc_soft_t *rtc);
void RTC_Unpack(uint32_t v, rtc_soft_t *rtc);

#endif
