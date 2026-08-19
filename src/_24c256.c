#include "main.h"
#include "si2c.h"
#include "_24c256.h"

void I2C_start(void);
void I2C_stop(void);
int I2C_writeByte(uint8_t data);


int _eeprom_read(uint8_t dev, uint16_t addr, uint8_t *dat, uint16_t len)
{
    return I2C_readRegister16(dev, addr ,len, dat);
}

int _eeprom_write(uint8_t dev, uint16_t addr, uint8_t *dat, uint16_t len)
{
    if(len <= _EEPROM_PAGE_SIZE)
        return I2C_writeRegister16(dev, addr, dat, len);
    return 0;
}

void I2C_waitEEPROM(uint8_t devAddr)
{
    int ack = 0;

    do {
        I2C_start();
        ack = I2C_writeByte((devAddr << 1) | 0);  // Intento de dirección en modo write
        I2C_stop();
    } while(!ack);  // Mientras NO haya ACK, está escribiendo
}


#define TEST_BLOCK   _EEPROM_PAGE_SIZE   // 64 bytes
#define TEST_ADDR1   0x0000
#define TEST_ADDR2   0x0100

//extern void delay_ms(int ms);

static int test_one_eeprom(uint8_t dev, uint16_t base_addr, uint8_t pattern)
{
    uint8_t tx[TEST_BLOCK];
    uint8_t rx[TEST_BLOCK];

    // leer
    if(!_eeprom_read(dev, base_addr, rx, TEST_BLOCK))
    {
        printf("EEPROM__ dev 0x%02X read failed\n", dev);
        return 0;
    }
    else
    {
        printf("EEPROM__ dev 0x%02X read OK\n", dev);  
        for(int x=0;x<TEST_BLOCK;x++) printf("%02X",rx[x]);
    }


    // generar patrón
    for(int i=0;i<TEST_BLOCK;i++)
        tx[i] = pattern + i;

    // escribir
    if(!_eeprom_write(dev, base_addr, tx, TEST_BLOCK))
    {
        printf("EEPROM dev 0x%02X write failed\n", dev);
        return 0;
    }
    else
    {
        printf("EEPROM__ dev 0x%02X write OK\n", dev);  
    }


    DelayUs(10000); // write cycle

    // limpiar buffer lectura
    memset(rx, 0, sizeof(rx));

    // leer
    if(!_eeprom_read(dev, base_addr, rx, TEST_BLOCK))
    {
        printf("EEPROM dev 0x%02X read failed\n", dev);
        return 0;
    }

    // comparar
    if(memcmp(tx, rx, TEST_BLOCK) != 0)
    {
        printf("EEPROM dev 0x%02X compare ERROR\n", dev);
        return 0;
    }

    printf("EEPROM dev 0x%02X OK\n", dev);
    return 1;
}

void _testeeprom(void)
{
    while(1)
    {
        test_one_eeprom(_EEPROM_DEV_ADDR, 0, 0x5a);
        test_one_eeprom(_EEPROM_DEV2_ADDR_, 0, 0);
    }
}
