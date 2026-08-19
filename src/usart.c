#include "main.h"
#include "usart.h"
#include <string.h>


static volatile uint8_t rx_buf[UART_RX_BUF_SIZE];
static volatile uint16_t rx_idx = 0;
static volatile uint8_t overrun_u1  = 0;

/*
void USART1_IRQHandler(void)
{
    if (USART1->SR & USART_SR_RXNE)
        rx_buf[rx_idx++ % UART_RX_BUF_SIZE] = USART1->DR;
}
*/

void USART1_IRQHandler(void)
{
    uint32_t sr = USART1->SR;

    // 1️⃣ Overrun error
    if (sr & USART_SR_ORE)
    {
        volatile uint32_t tmp;
        tmp = USART1->SR;
        tmp = USART1->DR;
        (void)tmp;
        overrun_u1++;
        return; // opcional, pero recomendable
    }

    // 2️⃣ Recepción normal
    if (sr & USART_SR_RXNE)
    {
        rx_buf[rx_idx++ % UART_RX_BUF_SIZE] = USART1->DR;
    }
}


void USART2_IRQHandler(void)
{
    if (USART2->SR & USART_SR_RXNE)
        rx_buf[rx_idx++ % UART_RX_BUF_SIZE] = USART2->DR;
}

// Disable USART1 RX interrupt
void USART1_DisableRXInterrupt(void)
{
    USART1->CR1 &= ~USART_CR1_RXNEIE;   // Disable RX interrupt
}

// Enable USART1 RX interrupt
void USART1_EnableRXInterrupt(void)
{
    USART1->CR1 |= USART_CR1_RXNEIE;    // Enable RX interrupt
}

// Minimal USART1 init
void USART1_Init(uint32_t _sysclkfreq, uint8_t _irq)
{
    // Enable clocks for GPIOA and USART1
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_USART1EN;

    // Configure PA9 as alternate function push-pull (TX)
    GPIOA->CRH &= ~(GPIO_CRH_MODE9 | GPIO_CRH_CNF9);
    GPIOA->CRH |= (GPIO_CRH_MODE9_1 | GPIO_CRH_MODE9_0); // Output 50 MHz
    GPIOA->CRH |= GPIO_CRH_CNF9_1;                        // AF Push-Pull

    // Configure PA10 as input floating (RX)
    GPIOA->CRH &= ~(GPIO_CRH_MODE10 | GPIO_CRH_CNF10);
    GPIOA->CRH |= GPIO_CRH_CNF10_0;

    //#define SYSCLK 8000000UL  // HSI = 8 MHz
    USART1->BRR = _sysclkfreq / 115200;

    // Configure USART1: 115200 baud, 8N1
    //USART1->BRR = 72000000 / 115200; // Assuming 72 MHz clock
    USART1->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;

    if(_irq)
    {
        // Enable RX interrupt
        USART1->CR1 |= USART_CR1_RXNEIE;
        // Enable IRQ in NVIC
        NVIC_EnableIRQ(USART1_IRQn);
    }
}

// Send a single character
void USART1_SendChar(char c)
{
    while (!(USART1->SR & USART_SR_TXE)); // Wait until transmit empty
    USART1->DR = c;
}

int USART1_ReceiveChar(char *c)
{
    if (USART1->SR & USART_SR_RXNE)
    {
        *c = (char)(USART1->DR & 0xFF);
        return 1;           // Received
    }
    return 0;               // No data
}

// Send a string
void USART1_Print(const char *s)
{
    while (*s)
        USART1_SendChar(*s++);
}

// Calculate BRR for oversampling by 16 (USARTDIV = PCLK / (16 * baud))
static uint32_t USART_BRR_FromPCLK(uint32_t pclk, uint32_t baud)
{
    // Use integer math to get mantissa and fraction:
    // usartdiv = pclk / (16 * baud)
    // We compute with scaling to keep fraction: scaled = (25 * pclk) / (4 * baud)
    uint32_t scaled = (25U * pclk) / (4U * baud);   // scaled = usartdiv * 100
    uint32_t mantissa = scaled / 100U;
    uint32_t fraction = ((scaled - mantissa * 100U) * 16U + 50U) / 100U; // rounded

    if (fraction >= 16U) { mantissa += 1U; fraction = 0U; }

    return (mantissa << 4) | (fraction & 0x0F);
}

void USART2_Init(uint32_t sysclk_hz, uint8_t use_irq)
{
    // Enable GPIOA and USART2 clocks
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    // Configure PA2 TX = AF push-pull, 50MHz
    GPIOA->CRL &= ~(GPIO_CRL_MODE2 | GPIO_CRL_CNF2);
    GPIOA->CRL |=  (GPIO_CRL_MODE2_1 | GPIO_CRL_MODE2_0); // 50 MHz
    GPIOA->CRL &= ~GPIO_CRL_CNF2;                         // clear CNF
    GPIOA->CRL |=  (0x2 << (4*2 + 2));                   // CNF = 10 (AF push-pull)
    // Note: GPIO_CRL_CNF2_1 macro equals (0x2 << (4*2 + 2)) depending header

    // Configure PA3 RX = floating input
    GPIOA->CRL &= ~(GPIO_CRL_MODE3 | GPIO_CRL_CNF3);
    GPIOA->CRL |=  (0x1 << (4*3 + 2)); // CNF = 01 (floating input) -> same as GPIO_CRL_CNF3_0

    // Compute APB1 clock. On typical Blue Pill: APB1 = SYSCLK / 2
    uint32_t apb1 = sysclk_hz / 2U;

    // Set BRR with fraction
    USART2->BRR = USART_BRR_FromPCLK(apb1, 115200U);

    // Clear pending flags and reset control regs before enabling
    USART2->SR = 0;
    USART2->CR1 = 0;
    USART2->CR2 = 0;
    USART2->CR3 = 0;

    // Enable TX, RX and UE
    USART2->CR1 = USART_CR1_TE | USART_CR1_RE;
    USART2->CR1 |= USART_CR1_UE;

    // Optionally enable RX interrupt
    if (use_irq)
    {
        USART2->CR1 |= USART_CR1_RXNEIE;
        NVIC_EnableIRQ(USART2_IRQn);
    }
}

// Send a single character
void USART2_SendChar(char c)
{
    while (!(USART2->SR & USART_SR_TXE)); // Wait until transmit empty
    USART2->DR = c;
}

// Send a string
void USART2_Print(const char *s)
{
    while (*s)
        USART2_SendChar(*s++);
}

int USART2_ReceiveChar(char *c)
{
    if (USART2->SR & USART_SR_RXNE)
    {
        *c = (char)(USART2->DR & 0xFF);
        return 1;           // Received
    }
    return 0;               // No data
}

void USART1_FlushRx(void)
{
    rx_idx = 0;
    memset((uint8_t *)rx_buf, 0, sizeof(rx_buf));
}

uint16_t USART1_ReadRx(void)
{
    return rx_idx;
}

uint8_t *USART1_rx(uint16_t *rxlen, uint8_t irqf)
{
    if(rx_idx)
    {
        USART1_DisableRXInterrupt();
        if(rxlen) *rxlen = rx_idx;
        if(irqf & 2)
            rx_idx = 0;
        if((irqf & 1) || !rxlen)
            USART1_EnableRXInterrupt();
        return (uint8_t*)rx_buf;
    }
    else return 0;
}

int USART1_tx(uint8_t *buf, uint16_t len)
{
    for(int x=0;x<len;x++) USART1_SendChar(buf[x]);
    return 1;
}

uint8_t USART1_GetOVR(void)
{
    return overrun_u1;
}

void test_usart(void)
{
    uint16_t rxlen;

    while(1)
    {
        ms_delay(100);
        uint8_t *rx = USART1_rx(&rxlen, 3);
        if(rx)
        {
            printf("rx = len (%d) [", rxlen);
            for(int x=0;x<rxlen;x++) printf("%02X",rx[x]);
            printf("]\n"); 
        }
    }
}


