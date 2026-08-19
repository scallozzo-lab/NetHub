#ifndef __24C256_H__
    #define __24C256_H__

#include "main.h"
#include "si2c.h"

#define _EEPROM_PAGE_SIZE   64//128   // 24C256=(64) / 24C512=(128)
#define _EEPROM_DEV_ADDR        0x50
#define _EEPROM_DEV2_ADDR_      0x51


int _eeprom_read(uint8_t dev, uint16_t addr, uint8_t *dat, uint16_t len);
int _eeprom_write(uint8_t dev, uint16_t addr, uint8_t *dat, uint16_t len);
void I2C_waitEEPROM(uint8_t devAddr);

#endif
