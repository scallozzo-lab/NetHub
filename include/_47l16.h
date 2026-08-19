#ifndef __47L16_H__
    #define  __47L16_H__

#include "main.h"

#define _EERAM_PAGE_SIZE        0x7ff
#define _EERAM_DEV_ADDR         0x50
#define _EERAM_DEV_CTRL_ADDR    0x18//0x30

typedef enum
{
    STS_REG_EVENT   = 1,
    STS_REG_ASE     = 2,
    STS_REG_BP0     = 4,
    STS_REG_BP1     = 8,
    STS_REG_BP2     = 16,
    STS_REG_nu5     = 32,
    STS_REG_nu6     = 64,
    STS_REG_AM      = 128
}estatusreg;

int _eeram_read(uint8_t dev, uint16_t addr, uint8_t *dat, uint16_t len);
int _eeram_write(uint8_t dev, uint16_t addr, uint8_t *dat, uint16_t len);
uint8_t EERAM_ReadStatus(uint8_t *st);
uint8_t EERAM_EnableAutoStore(void);

#endif