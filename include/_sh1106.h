#ifndef __SH1106_H__
    #define __SH1106_H__

#include "main.h"

//#define SH1106_I2C_ADDR     0x78
//#define SH1106_I2C_ADDR_II  0x7A

#define SH1106_I2C_ADDR     0x78
#define SH1106_I2C_ADDR_II  0x7A



typedef enum
{
    _FONT_5X7,
    _FONT_8X8,
}efonttype;

void _SelLCD1(void);
void _SelLCD2(void);
void SH1106_init(void);
void SH1106_print(uint8_t page, uint8_t col, const char *str);
void SH1106_print8x8(uint8_t page, uint8_t col, const char *str);
void SH1106_Printf(uint8_t font, uint8_t x, uint8_t page, const char *fmt, ...);


#endif