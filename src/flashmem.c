#include "main.h"
#include "flashmem.h"
 

uint8_t _Flash_Read8(uint32_t addr)
{
    return *(volatile uint8_t*)addr;
}

void _Flash_ReadBlock(uint32_t addr, uint8_t *buf, uint32_t len)
{
    while (len--)
    {
        *buf++ = *(volatile uint8_t*)addr++;
    }
}

static void _Flash_Unlock(void)
{
    if (FLASH->CR & FLASH_CR_LOCK)
    {
        FLASH->KEYR = 0x45670123;
        FLASH->KEYR = 0xCDEF89AB;
    }
}

static void _Flash_Lock(void)
{
    FLASH->CR |= FLASH_CR_LOCK;
}

void _Flash_ErasePage(uint32_t pageAddress)
{
    while (FLASH->SR & FLASH_SR_BSY);

    _Flash_Unlock();

    FLASH->CR |= FLASH_CR_PER;
    FLASH->AR = pageAddress;
    FLASH->CR |= FLASH_CR_STRT;

    while (FLASH->SR & FLASH_SR_BSY);

    FLASH->CR &= ~FLASH_CR_PER;
    _Flash_Lock();
}

void _Flash_WriteHalfWord(uint32_t addr, uint16_t data)
{
    while (FLASH->SR & FLASH_SR_BSY);

    _Flash_Unlock();

    FLASH->CR |= FLASH_CR_PG;

    *(volatile uint16_t*)addr = data;  // WRITE!

    while (FLASH->SR & FLASH_SR_BSY);

    FLASH->CR &= ~FLASH_CR_PG;
    _Flash_Lock();
}

void _Flash_WriteBlock(uint32_t addr, const uint8_t *data, uint32_t len)
{
    if (len % 2) len++; // ensure even number of bytes

    for (uint32_t i = 0; i < len; i += 2)
    {
        uint16_t hw = data[i] | (data[i+1] << 8);
        _Flash_WriteHalfWord(addr + i, hw);
    }
}

void _Flash_ErasePages(uint32_t startPageAddr, uint32_t endPageAddr)
{
    // Alinear
    if (startPageAddr % 1024 != 0) return;
    if (endPageAddr   % 1024 != 0) return;

    _Flash_Unlock();

    for (uint32_t addr = startPageAddr; addr <= endPageAddr; addr += 1024)
    {
        while (FLASH->SR & FLASH_SR_BSY); // Esperar si está ocupada

        FLASH->CR |= FLASH_CR_PER;  // Page erase
        FLASH->AR = addr;           // Dirección de página
        FLASH->CR |= FLASH_CR_STRT; // Ejecutar

        while (FLASH->SR & FLASH_SR_BSY); // Esperar fin

        FLASH->CR &= ~FLASH_CR_PER; // Salir del modo erase
    }

    _Flash_Lock();
}

uint32_t _Flash_PageAddress(uint32_t pageNumber)
{
    return 0x08000000 + pageNumber * 1024;
}


#define FLASH_TEST_ADDR  0x08010000U   // start of second 64 KB

uint8_t Flash_Test128KB(void)
{
    uint32_t test_addr = FLASH_TEST_ADDR;

    // 1. Read old value (if this area does not exist, reading returns 0xFFFF)
    uint16_t original = *(volatile uint16_t*)test_addr;

    // 2. Erase page
    _Flash_ErasePage(test_addr);

    // 3. Write test pattern
    uint16_t pattern = 0xA55A;
    _Flash_WriteHalfWord(test_addr, pattern);

    // 4. Read back
    uint16_t readback = *(volatile uint16_t*)test_addr;

    // 5. Restore original data
    _Flash_ErasePage(test_addr);
    _Flash_WriteHalfWord(test_addr, original);

    // If write succeeded → we have 128 KB
    if (readback == pattern)
        return 1;   // YES → 128 KB present

    return 0;       // NO → only 64 KB
}


