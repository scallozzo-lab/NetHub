#include "_wireone.h"

#define LEDPORT GPIOB
#define LED1    12
#define LEDTX   11
#define ENABLE_GPIO_CLOCK (RCC->APB2ENR |= RCC_APB2ENR_IOPBEN)
#define _MODER CRH
#define GPIOMODER GPIO_CRH_MODE12_0
#define GPIOMODER_TX GPIO_CRH_MODE11_0

#define GPIO_CRH_PIN11_MASK   (0xF << 12)   // bits [15:12]
#define GPIO_OUTPUT_PP_2MHZ   0x2  // 0010: CNF=00, MODE=10

void _InitWireOne(void)
{
    ENABLE_GPIO_CLOCK;              // enable the clock to GPIO
    LEDPORT->CRH &= ~GPIO_CRH_PIN11_MASK;
    LEDPORT->CRH |=  (GPIO_OUTPUT_PP_2MHZ << 12);

#ifdef _USE_WIREONE

#else
    // Initializa RX_WIREONE como salida digital
    LEDPORT->CRH |= GPIOMODER;      // set pins to be general purpose output
#endif
}

// OJO que compile el resto solo si esta " #define _USE_WIREONE"
#ifdef _USE_WIREONE


#else
// Solo compila para no está wireone (Para uso general de los leds)
void LedMonitor(void)
{
   LEDPORT->ODR ^= (1<<LED1);  // toggle LED
}   
void LedMonitorGreen(void)
{
   LEDPORT->ODR ^= (1<<LEDTX);  // toggle LED
}   
void SetLedMonitorGreen(uint8_t on)
{
    if (on)
        LEDPORT->ODR |=  (1 << LEDTX);   // ON
    else
        LEDPORT->ODR &= ~(1 << LEDTX);   // OFF
}

#endif
