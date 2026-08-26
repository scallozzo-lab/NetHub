#ifndef __GNSS_H__
    #define __GNSS_H__

#include "main.h"

typedef struct
{
    uint8_t  fix_mode;
    uint8_t  satellites;

    bool     latitude_valid;
    int32_t  latitude_e7;

    bool     longitude_valid;
    int32_t  longitude_e7;

    uint32_t date;
    uint32_t time;

    int32_t  altitude_cm;
    int32_t  speed_milli;

    int32_t  pdop_milli;
    int32_t  hdop_milli;
    int32_t  vdop_milli;

} stGNSS_Position;

extern stGNSS_Position GNSS_Position;

bool GNSS_ParseInfo(const char *rx, stGNSS_Position *gnss);
void GNSS_DebugPrint(const stGNSS_Position *gnss);


#endif