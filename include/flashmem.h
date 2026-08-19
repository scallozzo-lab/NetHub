#ifndef __FLASHMEM_H__
    #define  __FLASHMEM_H__

#include "main.h"

/*
    Región	Dirección inicio	Dirección fin
    Primeros 32 KB	0x08000000	0x08007FFF
    Últimos 32 KB	0x08008000	0x0800FFFF
*/

#define _FLASH_ORG_ADDR     0x08000000lu
#define _FLASH_2ND_PRG_LEN  0x00008000lu 
#define _FLASH_ADDR_2ND_PRG (_FLASH_ORG_ADDR + _FLASH_2ND_PRG_LEN)

uint8_t _Flash_Read8(uint32_t addr);
void _Flash_ReadBlock(uint32_t addr, uint8_t *buf, uint32_t len);
void _Flash_ErasePage(uint32_t pageAddress);
void _Flash_WriteHalfWord(uint32_t addr, uint16_t data);
void _Flash_WriteBlock(uint32_t addr, const uint8_t *data, uint32_t len);
uint8_t Flash_Test128KB(void);

#endif