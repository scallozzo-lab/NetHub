
#include "si2c.h"

// --- Macros SDA ---
#define SDA_LOW()   (SDA_PORT->CRH = (SDA_PORT->CRH & ~(0xF << ((SDA_PIN-8)*4))) | (0x1 << ((SDA_PIN-8)*4)))  // Output 10MHz PushPull
#define SDA_RELEASE() (SDA_PORT->CRH = (SDA_PORT->CRH & ~(0xF << ((SDA_PIN-8)*4))) | (0x4 << ((SDA_PIN-8)*4))) // Input floating
#define SDA_READ()  ((SDA_PORT->IDR >> SDA_PIN) & 1)

// --- Macros SCL ---
#define SCL_LOW()   (SCL_PORT->CRH = (SCL_PORT->CRH & ~(0xF << ((SCL_PIN-8)*4))) | (0x1 << ((SCL_PIN-8)*4)))
#define SCL_RELEASE() (SCL_PORT->CRH = (SCL_PORT->CRH & ~(0xF << ((SCL_PIN-8)*4))) | (0x4 << ((SCL_PIN-8)*4)))
#define SCL_READ()  ((SCL_PORT->IDR >> SCL_PIN) & 1)

#define false 0
#define true 1


void SDA_high() { SDA_RELEASE();}
void SDA_low()  { SDA_LOW(); }
void SCL_high() { SCL_RELEASE(); }
void SCL_low()  { SCL_LOW(); }
int  SDA_read() { return SDA_READ(); }



#ifdef _SI2C_CONFIG_400KHZ     
void delay_i2c() { DelayUs(10); } // 50us para 100khz // 10 para 418khz
#else
void delay_i2c() { DelayUs(50); } // 50us para 100khz // 10 para 418khz
#endif

void SI2C_GPIO_Init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;

    // PB14 SCL – Open Drain
    GPIOB->CRH &= ~(GPIO_CRH_MODE14 | GPIO_CRH_CNF14);
    GPIOB->CRH |=  (GPIO_CRH_MODE14_1 | GPIO_CRH_CNF14_0); // 2MHz + Open drain

    // PB15 SDA – Open Drain
    //GPIOB->CRH &= ~(GPIO_CRH_MODE15 | GPIO_CRH_CNF15);
    //GPIOB->CRH |=  (GPIO_CRH_MODE15_1 | GPIO_CRH_CNF15_0);

    // PB13 SDA – Open Drain
    GPIOB->CRH &= ~(GPIO_CRH_MODE13 | GPIO_CRH_CNF13);
    GPIOB->CRH |=  (GPIO_CRH_MODE13_1 | GPIO_CRH_CNF13_0);

    SDA_RELEASE();
    SCL_RELEASE();
}


// ---- Protocolo I2C soft ----
void I2C_start(void) 
{
    SDA_high();
    delay_i2c();
    SCL_high();
    delay_i2c();
    SDA_low();
    delay_i2c();
    SCL_low();
    delay_i2c();
}

void I2C_stop(void)
{
    SDA_low();
    delay_i2c();
    SCL_high();
    delay_i2c();
    SDA_high();
    delay_i2c();
}

int I2C_writeByte(uint8_t data)
{
    for(int i = 0; i < 8; i++)
    {
        if(data & 0x80)
            SDA_high();
        else
            SDA_low();
        delay_i2c();
        SCL_high();
        delay_i2c();
        SCL_low();
        data <<= 1;
    }

    // Leer ACK
    SDA_high(); // liberar SDA
    delay_i2c();
    SCL_high();
     
    delay_i2c();
   
    int ack = !SDA_read(); // ACK es nivel bajo
    delay_i2c();
    SCL_low();
    return ack;
}

static uint8_t I2C_readByte(int ack)
{
    uint8_t data = 0;
    
    SDA_high(); // liberar SDA (input)
    for(int i = 0; i < 8; i++)
    {
        data <<= 1;
        SCL_high();
        if(SDA_read())
            data |= 1;
        delay_i2c();
        SCL_low();
        delay_i2c();
    }

    // Enviar ACK/NACK
    if(ack)
        SDA_low();
    else
        SDA_high();
    
    delay_i2c();
    SCL_high();
    delay_i2c();
    SCL_low();
    
    delay_i2c();
    
    SDA_high(); // liberar SDA
    
    delay_i2c();
    
    return data;
}


int I2C_readRegister(uint8_t devAddr, uint8_t regAddr, uint8_t readsize, uint8_t *pdest)
{
    uint8_t data;
    int ack;

    I2C_start();
    ack = I2C_writeByte((devAddr << 1) | 0); // write address
#ifdef _DEBUG_SI2C
    printf("[I2C_readRegister] ACK1: 0x%02X\n", ack);
#endif    
    if(!ack) return false;
    
    DelayUs(_SI2C_TIMESLOT);   // Espera 10 microsegundos

    ack = I2C_writeByte(regAddr);            // Registro
#ifdef _DEBUG_SI2C
    printf("[I2C_readRegister] ACK2: 0x%02X\n", ack);
#endif
    if(!ack) return false;
    
    I2C_stop();
    DelayUs(_SI2C_TIMESLOT);   // Espera 10 microsegundos
    I2C_start();
    
    ack = I2C_writeByte((devAddr << 1) | 1); // read address
#ifdef _DEBUG_SI2C
    printf("[I2C_readRegister] ACK3: 0x%02X\n", ack);
#endif
    if(!ack) return false;
    
    DelayUs(_SI2C_TIMESLOTREAD);   // Espera 50 microsegundos

    while(readsize--)
    {
        data = I2C_readByte((int)readsize);  // Leer un byte, NACK
        if(pdest) *pdest++ = data;
    }
    
    I2C_stop();

    return ack;
}


int I2C_readRegister16(uint8_t devAddr, uint16_t regAddr, uint16_t readsize, uint8_t *pdest)
{
    uint8_t data;
    int ack;

    I2C_start();
    ack = I2C_writeByte((devAddr << 1) | 0);   // Write mode
    if(!ack) return false;

    DelayUs(_SI2C_TIMESLOT);

    // Send high address byte
    ack = I2C_writeByte((regAddr >> 8) & 0xFF);
    if(!ack) return false;

    // Send low address byte
    ack = I2C_writeByte(regAddr & 0xFF);
    if(!ack) return false;

    I2C_stop();
    DelayUs(_SI2C_TIMESLOT);

    // Repeated start
    I2C_start();

    ack = I2C_writeByte((devAddr << 1) | 1);   // Read mode
    if(!ack) return false;

    DelayUs(_SI2C_TIMESLOTREAD);

    while(readsize--)
    {
        // Send NACK only for last byte
        data = I2C_readByte(readsize != 0);
        if(pdest) *pdest++ = data;
    }

    I2C_stop();

    return true;
}


int I2C_writeRegister(uint8_t devAddr, uint8_t regAddr, uint8_t data)
{
    int ack;

    I2C_start();
    ack = I2C_writeByte((devAddr << 1) | 0); // write address
#ifdef _DEBUG_SI2C
    printf("[I2C_writeRegister] ACK1: 0x%02X\n", ack);
#endif
    if(!ack) return false;
    
    DelayUs(_SI2C_TIMESLOT);   // Espera 100 microsegundos

    ack = I2C_writeByte(regAddr);            // Registro
#ifdef _DEBUG_SI2C
    printf("[I2C_writeRegister] ACK2: 0x%02X\n", ack);
#endif
    if(!ack) return false;

    ack = I2C_writeByte(data);            // Registro
#ifdef _DEBUG_SI2C
    printf("[I2C_writeRegister] ACK2: 0x%02X\n", ack);
#endif
    if(!ack) return false;
   
    I2C_stop();

    DelayUs(_SI2C_TIMESLOTREAD);   // Espera 50 microsegundos
   
    return ack;
}

int I2C_writeRegister16(uint8_t devAddr, uint16_t regAddr, uint8_t *data, uint16_t len)
{
    int ack;

    I2C_start();
    ack = I2C_writeByte((devAddr << 1) | 0);   // write
    if(!ack) return false;

    DelayUs(_SI2C_TIMESLOT);

    // High address byte
    ack = I2C_writeByte((regAddr >> 8) & 0xFF);
    if(!ack) return false;

    // Low address byte
    ack = I2C_writeByte(regAddr & 0xFF);
    if(!ack) return false;

    if(data)
    {
        while(len--)
        {
            // Data
            ack = I2C_writeByte(*data++);
            if(!ack) return false;
        }
    }
    
    I2C_stop();

    return true;
}
