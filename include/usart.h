#ifndef __USART_H__
    #define  __USART_H__

#include "main.h"

#define UART_RX_BUF_SIZE        2048
#define _MAX_UART_TXBUFFER      1500

#define _USART1_DMA_MODE
#define _USART1_DMA_TX_MODE

#define _COMINTDISABLE          0
#define _COMINTCLEARFIFO        2


void USART1_Init(uint32_t _sysclkfreq, uint8_t _irq);
void USART2_Init(uint32_t _sysclkfreq, uint8_t _irq);
void USART1_SendChar(char c);
void USART1_Print(const char *s);
void USART2_SendChar(char c);
void USART2_Print(const char *s);
uint8_t *USART1_rx(uint16_t *rxlen, uint8_t irqf);
uint16_t USART1_ReadRx(void);
void USART1_FlushRx(void);
int USART1_tx(uint8_t *buf, uint16_t len);
uint8_t USART1_GetOVR(void);

#endif