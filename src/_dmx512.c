#include "_dmx512.h"

#define DMX_TX_HIGH()    (GPIOB->BSRR = GPIO_BSRR_BS15)      //bs15   
#define DMX_TX_LOW()     (GPIOB->BSRR = GPIO_BSRR_BR15)      //br15   


static inline void DMX_DWT_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

/*
void xDelay(uint32_t us)
{
    uint32_t start = SysTick->VAL;
    uint32_t ticks = us * 7200;//(get_sysclk_freq / 1000000UL);

    while ((int32_t)(start - SysTick->VAL) < ticks)
    {
    }
}
    */

void xDelay(uint32_t us)
{
    for (volatile uint32_t x = 0; x < us; x++)
    {
        __asm volatile ("nop");
    }
}

void DMX_GPIO_Init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
    // PB15 = DMX TX
    // Output push-pull, 2 MHz
    GPIOB->CRH &= ~(GPIO_CRH_MODE15 | GPIO_CRH_CNF15);
    GPIOB->CRH |=  (GPIO_CRH_MODE15_1);

    // Estado idle de DMX = HIGH
    DMX_TX_HIGH();


 DMX_DWT_Init();

}

/*
static 
inline
void DMX_SendByte(uint8_t data)
{
    uint8_t i;

    // START BIT
    DMX_TX_LOW();
    xDelay(_DMX_PULSE_4US);

    // DATA - LSB first
    for (i = 0; i < 8; i++)
    {
        if (data & (1 << i))
            DMX_TX_HIGH();
        else
            DMX_TX_LOW();

        xDelay(_DMX_PULSE_4US);
    }

    // STOP BIT 1
    DMX_TX_HIGH();
    xDelay(_DMX_PULSE_4US);

    // STOP BIT 2
    DMX_TX_HIGH();
    xDelay(_DMX_PULSE_4US);
}

*/

/*
static inline void DMX_SendByte(uint8_t data)
{
    uint32_t next = DWT->CYCCNT;

    // START BIT
    DMX_TX_LOW();
    next += 288;

    // DATA bits
    for (uint8_t i = 0; i < 8; i++)
    {
        while ((int32_t)(DWT->CYCCNT - next) < 0)
        {
        }

        if (data & (1U << i))
            DMX_TX_HIGH();
        else
            DMX_TX_LOW();

        next += 288;
    }

    // STOP 1
    while ((int32_t)(DWT->CYCCNT - next) < 0)
    {
    }

    DMX_TX_HIGH();
    next += 288;

    // STOP 2
    while ((int32_t)(DWT->CYCCNT - next) < 0)
    {
    }

    DMX_TX_HIGH();

    while ((int32_t)(DWT->CYCCNT - (next + 288)) < 0)
    {
    }
}
*/



static inline void DMX_SendByte(uint8_t data)
{
    uint32_t next = DWT->CYCCNT;

    // START
    DMX_TX_LOW();
    next += 288;

    // DATA
    for (uint8_t i = 0; i < 8; i++)
    {
        while ((int32_t)(DWT->CYCCNT - next) < 0)
        {
        }

        if (data & (1U << i))
            DMX_TX_HIGH();
        else
            DMX_TX_LOW();

        next += 288;
    }

    // STOP 1
    while ((int32_t)(DWT->CYCCNT - next) < 0)
    {
    }

    DMX_TX_HIGH();
    next += 288;

    // STOP 2
    while ((int32_t)(DWT->CYCCNT - next) < 0)
    {
    }

    DMX_TX_HIGH();

    while ((int32_t)(DWT->CYCCNT - (next + 288)) < 0)
    {
    }
}


void DMX_SendFrame(uint8_t scode, uint8_t *data, uint16_t length)
{
    uint16_t i;

    __disable_irq();
    /*
     * IDLE
     */
    DMX_TX_HIGH();

    /*
     * BREAK
     * Mínimo DMX: 88 usDelayUs
     */
    DMX_TX_LOW();
    xDelay(_DMX_BREAK_100US);

    /*DelayUs
     * MAB
     * Mínimo: 8 us
     */
    DMX_TX_HIGH();
    xDelay(_DMX_MAB_12US);    
    /*
     * START CODE
     * 0x00 = DMX normal
     */
    DMX_SendByte(scode);

    /*
     * CHANNEL DATA
     */
    for (i = 0; i < length; i++)
    {
        DMX_SendByte(data[i]);
    }

    /*
     * Dejamos el bus en IDLE
     */
    DMX_TX_HIGH();
    
    __enable_irq();

}