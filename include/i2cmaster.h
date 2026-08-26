#ifndef __I2CMASTER_H__
    #define __I2CMASTER_H__

#include "main.h"

#define SDA1_PORT GPIOB
#define SDA1_PIN  7
#define SCL1_PORT GPIOB
#define SCL1_PIN  6


#define _DEBUG_SI2C1


#define _SI2C1_TIMEBASEUS        20
#define _SI2C1_TIMESLOT          10
#define _SI2C1_TIMESLOTREAD      50

//#define _SI2C1_CONFIG_400KHZ     
#define _SI2C1_CONFIG_100KHZ
     

void SI2C1_GPIO_Init(void);
int I2C1_readRegister(uint8_t devAddr, uint8_t regAddr, uint8_t readsize, uint8_t *pdest);
int I2C1_writeRegister(uint8_t devAddr, uint8_t regAddr, uint8_t data);
int I2C1_readRegister16(uint8_t devAddr, uint16_t regAddr, uint16_t readsize, uint8_t *pdest);
int I2C1_writeRegister16(uint8_t devAddr, uint16_t regAddr, uint8_t *data, uint16_t len);

#endif