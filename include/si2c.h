#ifndef __SI2C_H__
    #define __SI2C_H__

#include "main.h"

#define SDA_PORT GPIOB
#define SDA_PIN  14//14
#define SCL_PORT GPIOB
#define SCL_PIN  13



//#define _DEBUG_SI2C


#define _SI2C_TIMEBASEUS        20
#define _SI2C_TIMESLOT          10
#define _SI2C_TIMESLOTREAD      50

#define _SI2C_CONFIG_400KHZ     
//#define _SI2C_CONFIG_100KHZ
     

void SI2C_GPIO_Init(void);
int I2C_readRegister(uint8_t devAddr, uint8_t regAddr, uint8_t readsize, uint8_t *pdest);
int I2C_writeRegister(uint8_t devAddr, uint8_t regAddr, uint8_t data);
int I2C_readRegister16(uint8_t devAddr, uint16_t regAddr, uint16_t readsize, uint8_t *pdest);
int I2C_writeRegister16(uint8_t devAddr, uint16_t regAddr, uint8_t *data, uint16_t len);


#endif