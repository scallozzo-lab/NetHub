#include "stdint.h"
#include "main.h"

#define CS_LOW()  (GPIOA->BSRR = (1 << (4+16)))  // Reset PA4
#define CS_HIGH() (GPIOA->BSRR = (1 << 4))       // Set PA4

#ifdef _USE_SSPI
    #define SPI1_Init SoftSPI1_Init 
    #define SPI1_TransmitReceive SoftSPI1_TransmitReceive
#endif

void SPI1_Init(void);
uint8_t SPI1_TransmitReceive(uint8_t data);

