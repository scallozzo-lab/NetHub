#include "_dmx512.h"

#define DMX_TX_HIGH()    (GPIOB->BSRR = GPIO_BSRR_BS15)
#define DMX_TX_LOW()     (GPIOB->BSRR = GPIO_BSRR_BR15)

void DMX_GPIO_Init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
    // PB15 = DMX TX
    // Output push-pull, 2 MHz
    GPIOB->CRH &= ~(GPIO_CRH_MODE15 | GPIO_CRH_CNF15);
    GPIOB->CRH |=  (GPIO_CRH_MODE15_1);

    // Estado idle de DMX = HIGH
    DMX_TX_HIGH();
}

static 
inline
void DMX_SendByte(uint8_t data)
{
    uint8_t i;

    // START BIT
    DMX_TX_LOW();
    DelayUs(_DMX_PULSE_4US);

    // DATA - LSB first
    for (i = 0; i < 8; i++)
    {
        if (data & (1 << i))
            DMX_TX_HIGH();
        else
            DMX_TX_LOW();

        DelayUs(_DMX_PULSE_4US);
    }

    // STOP BIT 1
    DMX_TX_HIGH();
    DelayUs(_DMX_PULSE_4US);

    // STOP BIT 2
    DMX_TX_HIGH();
    DelayUs(_DMX_PULSE_4US);
}

void DMX_SendFrame(uint8_t *data, uint16_t length)
{
    uint16_t i;

    __disable_irq();
    /*
     * IDLE
     */
    DMX_TX_HIGH();

    /*
     * BREAK
     * Mínimo DMX: 88 us
     */
    DMX_TX_LOW();
    DelayUs(_DMX_BREAK_100US);

    /*
     * MAB
     * Mínimo: 8 us
     */
    DMX_TX_HIGH();
    DelayUs(_DMX_MAB_12US);    
    /*
     * START CODE
     * 0x00 = DMX normal
     */
    DMX_SendByte(0x00);

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