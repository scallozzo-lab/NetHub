#include "main.h"
#include "i2cmaster.h"

/*
// --- Macros SDA ---
#define SDA1_LOW()   (SDA1_PORT->CRH = (SDA1_PORT->CRH & ~(0xF << ((SDA1_PIN-8)*4))) | (0x1 << ((SDA1_PIN-8)*4)))  // Output 10MHz PushPull
#define SDA1_RELEASE() (SDA1_PORT->CRH = (SDA1_PORT->CRH & ~(0xF << ((SDA1_PIN-8)*4))) | (0x4 << ((SDA1_PIN-8)*4))) // Input floating
#define SDA1_READ()  ((SDA1_PORT->IDR >> SDA1_PIN) & 1)

// --- Macros SCL ---
#define SCL1_LOW()   (SCL1_PORT->CRH = (SCL1_PORT->CRH & ~(0xF << ((SCL1_PIN-8)*4))) | (0x1 << ((SCL1_PIN-8)*4)))
#define SCL1_RELEASE() (SCL1_PORT->CRH = (SCL1_PORT->CRH & ~(0xF << ((SCL1_PIN-8)*4))) | (0x4 << ((SCL1_PIN-8)*4)))
#define SCL1_READ()  ((SCL1_PORT->IDR >> SCL1_PIN) & 1)
*/

// --- SDA ---
#define SDA1_LOW()      (SDA1_PORT->BRR  = (1 << SDA1_PIN))
#define SDA1_RELEASE()  (SDA1_PORT->BSRR = (1 << SDA1_PIN))
#define SDA1_READ()     ((SDA1_PORT->IDR >> SDA1_PIN) & 1)

// --- SCL ---
#define SCL1_LOW()      (SCL1_PORT->BRR  = (1 << SCL1_PIN))
#define SCL1_RELEASE()  (SCL1_PORT->BSRR = (1 << SCL1_PIN))
#define SCL1_READ()     ((SCL1_PORT->IDR >> SCL1_PIN) & 1)

#define false 0
#define true 1

static void SDA_high() { SDA1_RELEASE();}
static void SDA_low()  { SDA1_LOW(); }
static void SCL_high() { SCL1_RELEASE(); }
static void SCL_low()  { SCL1_LOW(); }
static int  SDA_read() { return SDA1_READ(); }

#ifdef _SI2C1_CONFIG_400KHZ     
static void delay_i2c() { DelayUs(10); } // 50us para 100khz // 10 para 418khz
#else
static void delay_i2c() { DelayUs(50); } // 50us para 100khz // 10 para 418khz
#endif


void SI2C1_GPIO_Init(void)
{
    /* Enable clocks */
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN | RCC_APB2ENR_AFIOEN;
    (void)RCC->APB2ENR;

    /* Fully disable hardware I2C1 */
    RCC->APB1ENR  &= ~RCC_APB1ENR_I2C1EN;
    RCC->APB1RSTR |=  RCC_APB1RSTR_I2C1RST;
    RCC->APB1RSTR &= ~RCC_APB1RSTR_I2C1RST;

    /* Force default I2C1 mapping (PB6/PB7) */
    AFIO->MAPR &= ~AFIO_MAPR_I2C1_REMAP;

    /* PB6 SCL - Open Drain, 2 MHz */
    GPIOB->CRL &= ~(GPIO_CRL_MODE6 | GPIO_CRL_CNF6);
    GPIOB->CRL |=  (GPIO_CRL_MODE6_1 | GPIO_CRL_CNF6_0);

    /* PB7 SDA - Open Drain, 2 MHz */
    GPIOB->CRL &= ~(GPIO_CRL_MODE7 | GPIO_CRL_CNF7);
    GPIOB->CRL |=  (GPIO_CRL_MODE7_1 | GPIO_CRL_CNF7_0);

    SDA1_RELEASE();
    SCL1_RELEASE();
}

// ---- Protocolo I2C soft ----
void I2C1_start(void) 
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

void I2C1_stop(void)
{
    SDA_low();
    delay_i2c();
    SCL_high();
    delay_i2c();
    SDA_high();
    delay_i2c();
}

int I2C1_writeByte(uint8_t data)
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

uint8_t I2C1_readByte(int ack)
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


int I2C1_readRegister(uint8_t devAddr, uint8_t regAddr, uint8_t readsize, uint8_t *pdest)
{
    uint8_t data;
    int ack;

    I2C1_start();
    ack = I2C1_writeByte((devAddr << 1) | 0); // write address
#ifdef _DEBUG_SI2C1
    printf("[I2C1_readRegister] ACK1: 0x%02X\n", ack);
#endif    
    if(!ack) return false;
    
    DelayUs(_SI2C1_TIMESLOT);   // Espera 10 microsegundos

    ack = I2C1_writeByte(regAddr);            // Registro
#ifdef _DEBUG_SI2C1
    printf("[I2C1_readRegister] ACK2: 0x%02X\n", ack);
#endif
    if(!ack) return false;
    
    I2C1_stop();
    DelayUs(_SI2C1_TIMESLOT);   // Espera 10 microsegundos
    I2C1_start();
    
    ack = I2C1_writeByte((devAddr << 1) | 1); // read address
#ifdef _DEBUG_SI2C1
    printf("[I2C1_readRegister] ACK3: 0x%02X\n", ack);
#endif
    if(!ack) return false;
    
    DelayUs(_SI2C1_TIMESLOTREAD);   // Espera 50 microsegundos

    while(readsize--)
    {
        data = I2C1_readByte((int)readsize);  // Leer un byte, NACK
        if(pdest) *pdest++ = data;
    }
    
    I2C1_stop();

    return ack;
}


int I2C1_readRegister16(uint8_t devAddr, uint16_t regAddr, uint8_t readsize, uint8_t *pdest)
{
    uint8_t data;
    int ack;

    I2C1_start();
    ack = I2C1_writeByte((devAddr << 1) | 0);   // Write mode
    if(!ack) return false;

    DelayUs(_SI2C1_TIMESLOT);

    // Send high address byte
    ack = I2C1_writeByte((regAddr >> 8) & 0xFF);
    if(!ack) return false;

    // Send low address byte
    ack = I2C1_writeByte(regAddr & 0xFF);
    if(!ack) return false;

    I2C1_stop();
    DelayUs(_SI2C1_TIMESLOT);

    // Repeated start
    I2C1_start();

    ack = I2C1_writeByte((devAddr << 1) | 1);   // Read mode
    if(!ack) return false;

    DelayUs(_SI2C1_TIMESLOTREAD);

    while(readsize--)
    {
        // Send NACK only for last byte
        data = I2C1_readByte(readsize != 0);
        if(pdest) *pdest++ = data;
    }

    I2C1_stop();

    return true;
}


int I2C1_writeRegister(uint8_t devAddr, uint8_t regAddr, uint8_t data)
{
    int ack;

    I2C1_start();
    ack = I2C1_writeByte((devAddr << 1) | 0); // write address
#ifdef _DEBUG_SI2C1
    printf("[I2C1_writeRegister] ACK1: 0x%02X\n", ack);
#endif
    if(!ack) return false;
    
    DelayUs(_SI2C1_TIMESLOT);   // Espera 100 microsegundos

    ack = I2C1_writeByte(regAddr);            // Registro
#ifdef _DEBUG_SI2C1
    printf("[I2C1_writeRegister] ACK2: 0x%02X\n", ack);
#endif
    if(!ack) return false;

    ack = I2C1_writeByte(data);            // Registro
#ifdef _DEBUG_SI2C1
    printf("[I2C1_writeRegister] ACK2: 0x%02X\n", ack);
#endif
    if(!ack) return false;
   
    I2C_stop();

    DelayUs(_SI2C1_TIMESLOTREAD);   // Espera 50 microsegundos
   
    return ack;
}

int I2C1_writeRegister16(uint8_t devAddr, uint16_t regAddr, uint8_t *data, uint16_t len)
{
    int ack;

    I2C1_start();
    ack = I2C1_writeByte((devAddr << 1) | 0);   // write
    if(!ack) return false;

    DelayUs(_SI2C1_TIMESLOT);

    // High address byte
    ack = I2C1_writeByte((regAddr >> 8) & 0xFF);
    if(!ack) return false;

    // Low address byte
    ack = I2C1_writeByte(regAddr & 0xFF);
    if(!ack) return false;

    if(data)
    {
        while(len--)
        {
            // Data
            ack = I2C1_writeByte(*data++);
            if(!ack) return false;
        }
    }
    
    I2C1_stop();

    return true;
}



void test_gpio_pb6_pb7(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
    (void)RCC->APB2ENR;

    // PB6 push-pull output
    GPIOB->CRL &= ~(GPIO_CRL_MODE6 | GPIO_CRL_CNF6);
    GPIOB->CRL |=  GPIO_CRL_MODE6_1; // 2 MHz, push-pull

    // PB7 push-pull output
    GPIOB->CRL &= ~(GPIO_CRL_MODE7 | GPIO_CRL_CNF7);
    GPIOB->CRL |=  GPIO_CRL_MODE7_1;

    while (1)
    {
        GPIOB->BSRR = (1 << 6) | (1 << 7);
        for (volatile int i = 0; i < 100000; i++);

        GPIOB->BRR  = (1 << 6) | (1 << 7);
        for (volatile int i = 0; i < 100000; i++);
    }
}
