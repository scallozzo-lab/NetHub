#include "main.h"
#include "spimaster.h"
#include "stm32f1xx.h"


// Correct CRL shifts for pins 0..7: bits = 4 * pin_number
#define PIN4_SHIFT  (4 * 4)   // 16  (PA4)
#define PIN5_SHIFT  (4 * 5)   // 20  (PA5)
#define PIN6_SHIFT  (4 * 6)   // 24  (PA6)
#define PIN7_SHIFT  (4 * 7)   // 28  (PA7)

#define MODE4_MASK (0x3 << PIN4_SHIFT)
#define CNF4_MASK  (0x3 << (PIN4_SHIFT + 2))

#define MODE5_MASK (0x3 << PIN5_SHIFT)
#define CNF5_MASK  (0x3 << (PIN5_SHIFT + 2))

#define MODE6_MASK (0x3 << PIN6_SHIFT)
#define CNF6_MASK  (0x3 << (PIN6_SHIFT + 2))

#define MODE7_MASK (0x3 << PIN7_SHIFT)
#define CNF7_MASK  (0x3 << (PIN7_SHIFT + 2))



#ifdef _USE_SSPI
/*--------------------- Soft SPI ------------------*/
#define SCK_HIGH()   (GPIOA->BSRR = (1 << 5))
#define SCK_LOW()    (GPIOA->BRR  = (1 << 5))

#define MOSI_HIGH()  (GPIOA->BSRR = (1 << 6))
#define MOSI_LOW()   (GPIOA->BRR  = (1 << 6))

#define READ_MISO()  ((GPIOA->IDR & (1 << 7)) != 0)


void SoftSPI1_Init(void)
{
    // Enable GPIOA clock
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    (void)RCC->APB2ENR;

    // PA5 SCK = Output Push-Pull 50MHz
    GPIOA->CRL &= ~(MODE5_MASK | CNF5_MASK);
    GPIOA->CRL |= (0x3 << PIN5_SHIFT); // MODE=11, CNF=00

    // PA6 MOSI = Output Push-Pull 50MHz  (SWAPPED)
    GPIOA->CRL &= ~(MODE6_MASK | CNF6_MASK);
    GPIOA->CRL |= (0x3 << PIN6_SHIFT); // MODE=11, CNF=00

    // PA7 MISO = Input Floating          (SWAPPED)
    GPIOA->CRL &= ~(MODE7_MASK | CNF7_MASK);
    GPIOA->CRL |= (0x1 << (PIN7_SHIFT + 2)); // CNF=01, MODE=00

    // PA4 CS = Output Push-Pull 50MHz
    GPIOA->CRL &= ~(MODE4_MASK | CNF4_MASK);
    GPIOA->CRL |= (0x3 << PIN4_SHIFT);

    // Idle states (SPI mode 0)
    GPIOA->BRR  = (1 << 5);  // SCK low
    GPIOA->BSRR = (1 << 4);  // CS high
}

static inline void spi_delay(void)
{
    //for (volatile int i = 0; i < 16; i++);  // ajustar según F_CPU
}

uint8_t SoftSPI1_TransmitReceive(uint8_t data)
{
    uint8_t rx = 0;

    for (int i = 7; i >= 0; i--)
    {
        // MOSI setup
        if (data & (1 << i))
            MOSI_HIGH();
        else
            MOSI_LOW();

        spi_delay();

        // Rising edge
        SCK_HIGH();
        spi_delay();

        // Sample MISO
        if (READ_MISO())
            rx |= (1 << i);

        // Falling edge
        SCK_LOW();
        spi_delay();
    }

    return rx;
}
#else

void SPI1_Init(void)
{
    // Habilitar reloj GPIOA y SPI1
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    // small delay to allow peripheral clock to actually turn on (safe practice)
    (void)RCC->APB2ENR;
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

    // PA5 SCK = AF Push-Pull 50MHz
    GPIOA->CRL &= ~(MODE5_MASK | CNF5_MASK);
    GPIOA->CRL |= (0x3 << PIN5_SHIFT) | (0x2 << (PIN5_SHIFT + 2));

    // PA7 MOSI = AF Push-Pull 50MHz
    GPIOA->CRL &= ~(MODE7_MASK | CNF7_MASK);
    GPIOA->CRL |= (0x3 << PIN7_SHIFT) | (0x2 << (PIN7_SHIFT + 2));

    // PA6 MISO = Input Floating (CNF = 01, MODE = 00)
    GPIOA->CRL &= ~(MODE6_MASK | CNF6_MASK);
    GPIOA->CRL |= (0x1 << (PIN6_SHIFT + 2)); // CNF6 = 01

    // PA4 CS = Output Push-Pull 50MHz
    GPIOA->CRL &= ~(MODE4_MASK | CNF4_MASK);
    GPIOA->CRL |= (0x3 << PIN4_SHIFT); // MODE4=11 (50MHz), CNF4=00 Push-Pull
    CS_HIGH();

    // Configure SPI1: Master, fPCLK/16, CPOL=0, CPHA=0, Software NSS
    // Set SSM and SSI before enabling SPE; keep settings explicit
    SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_BR_1;
    SPI1->CR2 = 0; // default (if you want to set interrupts or FRF change here)
    SPI1->CR1 |= SPI_CR1_SPE; // finally enable SPI
}



// Transmitir y recibir un byte
uint8_t SPI1_TransmitReceive(uint8_t data)
{
    // Esperar TXE
    while (!(SPI1->SR & SPI_SR_TXE));
    SPI1->DR = data;
    
    // Esperar RXNE
    while (!(SPI1->SR & SPI_SR_RXNE));
    return SPI1->DR;
}

#endif


int test_spi(void)
{
    static uint8_t init = 0;

    if(init == 0)
        SPI1_Init();

    printf("test_spi = %02X\n",init);

    init = 1;

    uint8_t data_send = 0x55;
    uint8_t data_received;

    while (1)
    {
        CS_LOW();
        data_received = SPI1_TransmitReceive(data_send);
        CS_HIGH();

         printf("received = %02X\n",data_received);

        for (volatile int i=0; i<1000; i++); // delay simple ~500ms
       printf("received = %02X\n",1);

    }
}
