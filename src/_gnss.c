#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "_gnss.h"


/* -------------------------------------------------------------------------- */
/* Global                                                                      */
/* -------------------------------------------------------------------------- */

stGNSS_Position GNSS_Position = {0};

stGNSS_Position *_GetGNSS(void)
{
    return &GNSS_Position;
}

/* -------------------------------------------------------------------------- */
/* Local functions                                                             */
/* -------------------------------------------------------------------------- */
static uint32_t ASCII_ToBCD(const char *str, uint8_t digits)
{
    uint32_t value = 0;

    for (uint8_t i = 0; i < digits; i++)
    {
        if (str[i] < '0' || str[i] > '9')
            break;

        value = (value << 4) | (uint32_t)(str[i] - '0');
    }

    return value;
}

static int32_t GNSS_StringToScaledInt(const char *str, uint32_t decimals)
{
    int32_t integer_part = 0;
    uint32_t decimal_part = 0;
    uint32_t decimal_count = 0;

    bool negative = false;
    bool fraction = false;

    if (str == NULL)
        return 0;

    if (*str == '-')
    {
        negative = true;
        str++;
    }

    while (*str != '\0')
    {
        if (*str == '.')
        {
            fraction = true;
            str++;
            continue;
        }

        if ((*str < '0') || (*str > '9'))
            break;

        if (!fraction)
        {
            integer_part =
                (integer_part * 10) + (*str - '0');
        }
        else
        {
            if (decimal_count < decimals)
            {
                decimal_part =
                    (decimal_part * 10) + (*str - '0');

                decimal_count++;
            }
        }

        str++;
    }

    /*
     * Complete missing decimal positions.
     *
     * 9.4  -> 9.40
     * 5.45 -> 5.450
     */
    while (decimal_count < decimals)
    {
        decimal_part *= 10;
        decimal_count++;
    }

    /*
     * Convert directly to requested scale.
     *
     * decimals = 7:
     *
     * 34.5596542
     * -> 345596542
     *
     * decimals = 2:
     *
     * 9.4
     * -> 940
     *
     * decimals = 3:
     *
     * 0.213
     * -> 213
     */
    int32_t multiplier = 1;

    for (uint32_t i = 0; i < decimals; i++)
        multiplier *= 10;

    int32_t value =
        (integer_part * multiplier) +
        decimal_part;

    if (negative)
        value = -value;

    return value;
}


/* -------------------------------------------------------------------------- */
/* GNSS parser                                                                 */
/* -------------------------------------------------------------------------- */

bool GNSS_ParseInfo(const char *rx, stGNSS_Position *gnss)
{
    const char *p;
    const char *field_start;

    char field[32];

    uint8_t field_num = 0;
    uint8_t len;


    if ((rx == NULL) || (gnss == NULL))
        return false;


    memset(gnss, 0, sizeof(stGNSS_Position));


    /*
     * Search for:
     *
     * +CGNSSINFO:
     */
    p = strstr(rx, "+CGNSSINFO:");

    if (p == NULL)
        return false;


    /*
     * Skip command prefix.
     */
    p += strlen("+CGNSSINFO:");


    field_start = p;


    while (1)
    {
        /*
         * Field separator.
         *
         * IMPORTANT:
         *
         * We deliberately do NOT use strtok().
         *
         * This preserves empty fields:
         *
         * 3,10,,02,01,...
         *       ^^
         */
        if ((*p == ',') ||
            (*p == '\r') ||
            (*p == '\n') ||
            (*p == '\0'))
        {
            len = (uint8_t)(p - field_start);


            if (len >= sizeof(field))
                len = sizeof(field) - 1;


            memcpy(field, field_start, len);

            field[len] = '\0';


            /*
             * Remove trailing "OK" if the modem sends:
             *
             * ...,05OK
             */
            char *ok = strstr(field, "OK");

            if (ok != NULL)
                *ok = '\0';


            /*
             * Debug fields.
             */
            printf("[GNSS] FIELD %u = [%s]\r\n",
                   (unsigned int)field_num,
                   field);


            switch (field_num)
            {
                /* ---------------------------------------------------------- */
                /* 0 - Fix mode                                                */
                /* ---------------------------------------------------------- */

                case 0:

                    if (field[0] != '\0')
                    {
                        gnss->fix_mode =
                            (uint8_t)atoi(field);
                    }

                    break;


                /* ---------------------------------------------------------- */
                /* 1 - Satellites                                              */
                /* ---------------------------------------------------------- */

                case 1:

                    if (field[0] != '\0')
                    {
                        gnss->satellites =
                            (uint8_t)atoi(field);
                    }

                    break;


                /* ---------------------------------------------------------- */
                /* 5 - Latitude                                                 */
                /* ---------------------------------------------------------- */

                case 5:

                    if (field[0] != '\0')
                    {
                        gnss->latitude_e7 =
                            GNSS_StringToScaledInt(field, 7);

                        gnss->latitude_valid = true;
                    }

                    break;


                /* ---------------------------------------------------------- */
                /* 6 - Latitude hemisphere                                       */
                /* ---------------------------------------------------------- */

                case 6:

                    if (field[0] == 'S')
                    {
                        gnss->latitude_e7 =
                            -gnss->latitude_e7;
                    }

                    break;


                /* ---------------------------------------------------------- */
                /* 7 - Longitude                                                */
                /* ---------------------------------------------------------- */

                case 7:

                    if (field[0] != '\0')
                    {
                        gnss->longitude_e7 =
                            GNSS_StringToScaledInt(field, 7);

                        gnss->longitude_valid = true;
                    }

                    break;


                /* ---------------------------------------------------------- */
                /* 8 - Longitude hemisphere                                      */
                /* ---------------------------------------------------------- */

                case 8:

                    if (field[0] == 'W')
                    {
                        gnss->longitude_e7 =
                            -gnss->longitude_e7;
                    }

                    break;


                /* ---------------------------------------------------------- */
                /* 9 - Date                                                     */
                /* ---------------------------------------------------------- */

                case 9:

                    if (field[0] != '\0')
                    {
                        //gnss->date = (uint32_t)strtoul(field, NULL, 10);
                        gnss->date = ASCII_ToBCD(field,6);
                    }

                    break;


                /* ---------------------------------------------------------- */
                /* 10 - Time                                                     */
                /* ---------------------------------------------------------- */

                case 10:

                    if (field[0] != '\0')
                    {
                        /*
                         * Example:
                         *
                         * 025459.46
                         *
                         * Stored as:
                         *
                         * 025459
                         *
                         * The fractional seconds are ignored.
                         */
                        //gnss->time = (uint32_t)strtoul(field, NULL, 10);
                        gnss->time = ASCII_ToBCD(field,6);
                    }

                    break;


                /* ---------------------------------------------------------- */
                /* 11 - Altitude                                                 */
                /* ---------------------------------------------------------- */

                case 11:

                    if (field[0] != '\0')
                    {
                        /*
                         * Stored in centimeters.
                         *
                         * 89.7 m -> 8970
                         */
                        gnss->altitude_cm =
                            GNSS_StringToScaledInt(field, 2);
                    }

                    break;


                /* ---------------------------------------------------------- */
                /* 12 - Speed                                                    */
                /* ---------------------------------------------------------- */

                case 12:

                    if (field[0] != '\0')
                    {
                        /*
                         * Stored in milli-units.
                         *
                         * 3.328 -> 3328
                         */
                        gnss->speed_milli =
                            GNSS_StringToScaledInt(field, 3);
                    }

                    break;


                /* ---------------------------------------------------------- */
                /* 14 - PDOP                                                     */
                /* ---------------------------------------------------------- */

                case 14:

                    if (field[0] != '\0')
                    {
                        gnss->pdop_milli =
                            GNSS_StringToScaledInt(field, 3);
                    }

                    break;


                /* ---------------------------------------------------------- */
                /* 15 - HDOP                                                     */
                /* ---------------------------------------------------------- */

                case 15:

                    if (field[0] != '\0')
                    {
                        gnss->hdop_milli =
                            GNSS_StringToScaledInt(field, 3);
                    }

                    break;


                /* ---------------------------------------------------------- */
                /* 16 - VDOP                                                     */
                /* ---------------------------------------------------------- */

                case 16:

                    if (field[0] != '\0')
                    {
                        gnss->vdop_milli =
                            GNSS_StringToScaledInt(field, 3);
                    }

                    break;


                default:

                    break;
            }


            field_num++;


            /*
             * End of GNSS response.
             */
            if ((*p == '\r') ||
                (*p == '\n') ||
                (*p == '\0'))
            {
                break;
            }


            /*
             * Next field.
             */
            field_start = p + 1;
        }


        p++;
    }


    /*
     * Position is considered valid when:
     *
     * - latitude exists
     * - longitude exists
     * - at least one satellite is reported
     */
    if (gnss->latitude_valid &&
        gnss->longitude_valid &&
        (gnss->satellites > 0))
    {
        return true;
    }


    return false;
}


/* -------------------------------------------------------------------------- */
/* GNSS debug                                                                  */
/* -------------------------------------------------------------------------- */

void GNSS_DebugPrint(const stGNSS_Position *gnss)
{
    if (gnss == NULL)
    {
        printf("[GNSS] NULL\r\n");
        return;
    }


    printf("\r\n");
    printf("========== GNSS ==========\r\n");


    printf("Fix Mode    : %u\r\n",
           (unsigned int)gnss->fix_mode);


    printf("Satellites  : %u\r\n",
           (unsigned int)gnss->satellites);


    /* ---------------------------------------------------------------------- */
    /* Latitude                                                               */
    /* ---------------------------------------------------------------------- */

    if (gnss->latitude_valid)
    {
        int32_t lat = gnss->latitude_e7;

        if (lat < 0)
            lat = -lat;


        printf("Latitude    : %s%ld.%07ld\r\n",
               (gnss->latitude_e7 < 0) ? "-" : "",
               (long)(lat / 10000000L),
               (long)(lat % 10000000L));
    }
    else
    {
        printf("Latitude    : INVALID\r\n");
    }


    /* ---------------------------------------------------------------------- */
    /* Longitude                                                              */
    /* ---------------------------------------------------------------------- */

    if (gnss->longitude_valid)
    {
        int32_t lon = gnss->longitude_e7;

        if (lon < 0)
            lon = -lon;


        printf("Longitude   : %s%ld.%07ld\r\n",
               (gnss->longitude_e7 < 0) ? "-" : "",
               (long)(lon / 10000000L),
               (long)(lon % 10000000L));
    }
    else
    {
        printf("Longitude   : INVALID\r\n");
    }


    /* ---------------------------------------------------------------------- */
    /* Date / Time                                                            */
    /* ---------------------------------------------------------------------- */

    printf("Date        : %0X\r\n",
           (unsigned long)gnss->date);


    printf("Time        : %0X\r\n",
           (unsigned long)gnss->time);


    /* ---------------------------------------------------------------------- */
    /* Altitude                                                               */
    /* ---------------------------------------------------------------------- */

    {
        int32_t altitude = gnss->altitude_cm;

        if (altitude < 0)
        {
            altitude = -altitude;

            printf("Altitude    : -%ld.%02ld m\r\n",
                   (long)(altitude / 100),
                   (long)(altitude % 100));
        }
        else
        {
            printf("Altitude    : %ld.%02ld m\r\n",
                   (long)(altitude / 100),
                   (long)(altitude % 100));
        }
    }


    /* ---------------------------------------------------------------------- */
    /* Speed                                                                  */
    /* ---------------------------------------------------------------------- */

    {
        int32_t speed = gnss->speed_milli;

        if (speed < 0)
            speed = -speed;

        printf("Speed       : %ld.%03ld\r\n",
               (long)(speed / 1000),
               (long)(speed % 1000));
    }


    /* ---------------------------------------------------------------------- */
    /* PDOP                                                                   */
    /* ---------------------------------------------------------------------- */

    printf("PDOP        : %ld.%03ld\r\n",
           (long)(gnss->pdop_milli / 1000),
           (long)(gnss->pdop_milli % 1000));


    /* ---------------------------------------------------------------------- */
    /* HDOP                                                                   */
    /* ---------------------------------------------------------------------- */

    printf("HDOP        : %ld.%03ld\r\n",
           (long)(gnss->hdop_milli / 1000),
           (long)(gnss->hdop_milli % 1000));


    /* ---------------------------------------------------------------------- */
    /* VDOP                                                                   */
    /* ---------------------------------------------------------------------- */

    printf("VDOP        : %ld.%03ld\r\n",
           (long)(gnss->vdop_milli / 1000),
           (long)(gnss->vdop_milli % 1000));


    printf("===========================\r\n");
}