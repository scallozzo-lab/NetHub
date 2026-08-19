#include <string.h>
#include "_47l16.h"
#include "i2cmaster.h"

uint8_t I2C1_readByte(int ack);
int I2C1_writeByte(uint8_t data);
void I2C1_start(void);
void I2C1_stop(void); 


int _eeram_read(uint8_t dev, uint16_t addr, uint8_t *dat, uint16_t len)
{
    return I2C1_readRegister16(dev, addr, len, dat);
}

int _eeram_write(uint8_t dev, uint16_t addr, uint8_t *dat, uint16_t len)
{
    if(len <= _EERAM_PAGE_SIZE)
        return I2C1_writeRegister16(dev, addr, dat, len);
    return 0;
}

void I2C1_waitEERAM(uint8_t devAddr)
{
    int ack = 0;

    do {
        I2C1_start();
        ack = I2C1_writeByte((devAddr << 1) | 0);  // Intento de dirección en modo write
        I2C1_stop();
    } while(!ack);  // Mientras NO haya ACK, está escribiendo
}


uint8_t EERAM_EnableAutoStore(void)
{
    uint8_t ack;
    I2C1_start();
    ack = I2C1_writeByte(_EERAM_DEV_CTRL_ADDR<<1);
    if(!ack) goto stop;
    else printf("ack1\n");
    ack = I2C1_writeByte(0);
    if(!ack) goto stop;
    else printf("ack2\n");
    ack = I2C1_writeByte(STS_REG_ASE);
    if(!ack) printf("ack3\n");
stop:    
    I2C1_stop();
    if(ack) DelayUs(5000);
    return ack;
}

uint8_t EERAM_ReadStatus(uint8_t *st)
{
    uint8_t status;
    uint8_t ack;
    I2C1_start();
    ack = I2C1_writeByte((_EERAM_DEV_CTRL_ADDR<<1) | 1);
    status = I2C1_readByte(0);
    I2C1_stop();
    if(ack && st) *st = status;
    return ack;
}

void _test_eeram(void)
{
    uint8_t rdbuffer[16] = {0};
    uint8_t status;
    //EERAM_EnableAutoStore();
    
    uint8_t ack = EERAM_ReadStatus(&status);
    if(ack)
    {    
        printf("eeram read init %02X\n", status);
        if(!(status & STS_REG_ASE)) 
        {
            ack = EERAM_EnableAutoStore();
            if(ack) 
                printf("eeram read init OK %02X\n", status);  
        }
    }
    else 
        printf("eeram read init Error\n");
    
    int ret = _eeram_read(_EERAM_DEV_ADDR,0, rdbuffer, sizeof(rdbuffer));
    printf("eeram read =%02X\n", ret);
    for(int x=0;x<sizeof(rdbuffer);x++) printf("%02X",rdbuffer[x]);
    for(int x=0;x<sizeof(rdbuffer);x++) rdbuffer[x] = x;
    printf("\n");
    //ret = _eeram_write(_EERAM_DEV_ADDR,0, rdbuffer, sizeof(rdbuffer));
    //printf("eeram write =%02X\n", ret);
    memset(rdbuffer,0,sizeof(rdbuffer));
}