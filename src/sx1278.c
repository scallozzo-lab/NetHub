#include "main.h"
#include "spimaster.h"
#include "sx1278.h"

// --- Pines adicionales ---
#define LORA_RESET_LOW()   (GPIOA->BSRR = (1 << (3+16))) // PA3 LOW
#define LORA_RESET_HIGH()  (GPIOA->BSRR = (1 << 3))      // PA3 HIGH
#define LORA_GPIO0()   ((GPIOC->IDR & (1 << 13)) != 0) // PC13 input

// --- PB4 control ---
#define LORA_POWER_DOWN()   (GPIOB->BSRR = (1 << (4 + 16)))  // PB4 LOW
#define LORA_POWER_UP()     (GPIOB->BSRR = (1 << 4))         // PB4 HIGH
/*
// --- Inicialización GPIO para Reset y GPIO0 ---
void LORA_GPIO_Init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;

    // PA3 Reset = Output Push-Pull
    GPIOA->CRL &= ~(0xF << (4*3));
    GPIOA->CRL |=  (0x3 << (4*3)); // MODE=11 (50MHz), CNF=00

    // PA2 GPIO0 = Input Floating
    GPIOA->CRL &= ~(0xF << (4*2));
    GPIOA->CRL |=  (0x4 << (4*2)); // MODE=00, CNF=01
}

*/

/*
// --- Inicialización GPIO para Reset (PB3) y DIO0 (PC13) ---
void LORA_GPIO_Init(void)
{
    // Enable clocks for GPIOB and GPIOC
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPCEN;

    // --- PB3: Reset = Output Push-Pull ---
    // Clear config
    GPIOB->CRL &= ~(0xF << (4 * 3));
    // MODE = 11 (50MHz), CNF = 00 (Push-Pull)
    GPIOB->CRL |=  (0x3 << (4 * 3));

    // --- PC13: DIO0 = Input Floating ---
    // PC13 is in CRH (pins 8..15)
    GPIOC->CRH &= ~(0xF << (4 * (13 - 8)));
    // MODE = 00 (Input), CNF = 01 (Floating)
    GPIOC->CRH |=  (0x4 << (4 * (13 - 8)));
}
*/

// --- Inicialización GPIO para Reset (PB3), PB4 (Output) y DIO0 (PC13) ---
void LORA_GPIO_Init(void)
{
    // Enable clocks for GPIOB and GPIOC
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPCEN;

    // --- PB3: Reset = Output Push-Pull ---
    // Clear config
    GPIOB->CRL &= ~(0xF << (4 * 3));
    // MODE = 11 (50MHz), CNF = 00 (Push-Pull)
    GPIOB->CRL |=  (0x3 << (4 * 3));

    // --- PB4: Output Push-Pull ---
    // Clear config
    GPIOB->CRL &= ~(0xF << (4 * 4));
    // MODE = 11 (50MHz), CNF = 00 (Push-Pull)
    GPIOB->CRL |=  (0x3 << (4 * 4));

    // --- PC13: DIO0 = Input Floating ---
    // PC13 is in CRH (pins 8..15)
    GPIOC->CRH &= ~(0xF << (4 * (13 - 8)));
    // MODE = 00 (Input), CNF = 01 (Floating)
    GPIOC->CRH |=  (0x4 << (4 * (13 - 8)));
}

// Función para leer un registro del SX1278
uint8_t SX1278_ReadReg(uint8_t addr)
{
    uint8_t val;
    CS_LOW();
    SPI1_TransmitReceive(addr & 0x7F);   // MSB=0 → lectura
    val = SPI1_TransmitReceive(0x00);    // dummy write para clock
    CS_HIGH();
    return val;
}

// Función para escribir un registro del SX1278
void SX1278_WriteReg(uint8_t addr, uint8_t value)
{
    CS_LOW();
    SPI1_TransmitReceive(addr | 0x80);   // MSB=1 → escritura
    SPI1_TransmitReceive(value);
    CS_HIGH();
}


// --- Inicialización mínima SX1278 LoRa ---
void SX1278_LoRaInit(void)
{
    LORA_POWER_UP();
    for (volatile int i=0; i<3130000; i++); // ~pequeño delay
    // Reset del módulo
    LORA_RESET_LOW();
    for (volatile int i=0; i<10000; i++); // ~pequeño delay
    LORA_RESET_HIGH();
    for (volatile int i=0; i<10000; i++); // esperar boot
    
    while(SX1278_ReadReg(REG_VERSION) != 0x12);
      

    SX1278_WriteReg(REG_OP_MODE, LONG_RANGE_MODE); // RegOpMode: LoRa, sleep
    // Set frequency 433 Mhz
    SX1278_WriteReg(REG_FRF_MSB, 0x6C);
    SX1278_WriteReg(REG_FRF_MID, 0x40);
    SX1278_WriteReg(REG_FRF_LSB, 0x00);

    // Set FIFO base addresses
    SX1278_WriteReg(REG_FIFO_TX_BASE_ADDR, 0x00);
    SX1278_WriteReg(REG_FIFO_RX_BASE_ADDR, 0x80); // optional separate RX

    // 4. Set TX power
    SX1278_WriteReg(REG_PA_CONFIG, 0x8F);

    // 5. Modem config    
#ifdef _LORA_MODE_500                   
    /* ---- aprox. 17ms to transmit a luma32 packet ----- */
    SX1278_WriteReg(REG_MODEM_CONFIG1, 0x92); // BW=500kHz, CR=4/5, explicit header
    SX1278_WriteReg(REG_MODEM_CONFIG2, 0x74); // SF=7, CRC on
#else
     /* ---- aprox. 66ms to transmit a luma32 packet ----- */
    SX1278_WriteReg(REG_MODEM_CONFIG1, 0x72); // BW=125kHz, CR=4/5, explicit header
    SX1278_WriteReg(REG_MODEM_CONFIG2, 0x74); // SF=7, CRC on
#endif

    // 6. Preamble
    SX1278_WriteReg(REG_PREAMBLE_MSB, 0x00);
    SX1278_WriteReg(REG_PREAMBLE_LSB, 0x08);
    // 7. Sync word
    SX1278_WriteReg(REG_SYNC_WORD, 0x34);
    // 8. IRQ & DIO mapping
    SX1278_WriteReg(REG_DIO_MAPPING1, 0x00); // DIO0 → RxDone
    SX1278_WriteReg(REG_IRQ_FLAGS, 0xFF);    // clear all IRQs
    // 9. Set FIFO pointer to TX base
    SX1278_WriteReg(REG_FIFO_ADDR_PTR, 0x00);

    SX1278_WriteReg(REG_OP_MODE, LONG_RANGE_MODE + 1); // RegOpMode: LoRa, standby  
}

void SetLoraRxMode(void)
{
    SX1278_WriteReg(REG_OP_MODE, LONG_RANGE_MODE + MODE_RX_CONTINUOUS); // RegOpMode: LoRa, rx continuous   
}


void SetLoraTxMode(void)
{
    SX1278_WriteReg(REG_OP_MODE, LONG_RANGE_MODE + MODE_TX); // RegOpMode: LoRa, tx mode   
}

void ClearLoraIRQFlags(uint8_t f)
{
    SX1278_WriteReg(REG_IRQ_FLAGS, f);    
}

// --- Enviar N bytes ---
void SX1278_SendBytes(uint8_t *data, uint8_t len, uint8_t flag)
{
    // FIFO pointer al inicio
    SX1278_WriteReg(0x0D, 0x00);       // RegFifoTxBaseAddr

    // Escribir todos los bytes en FIFO
    for (uint8_t i = 0; i < len; i++)
    {
        SX1278_WriteReg(0x00, data[i]); // FIFO
    }

    // payload length
    SX1278_WriteReg(0x22, len);        // RegPayloadLength

    if(flag & (_SX1278_SETTXMODE + _SX1278_WAIT4TXDONE))
    {
        // modo TX
        SX1278_WriteReg(0x01, 0x83);       // LoRa, TX
        // esperar TX done
        if(flag & _SX1278_WAIT4TXDONE)
        {    
            while ((SX1278_ReadReg(0x12) & 0x08) == 0); // TXDone
            SX1278_WriteReg(0x12, 0x08);                 // limpiar flag
        }
    }
}

// --- Recibir N bytes ---
uint8_t SX1278_ReceiveBytes(uint8_t *buffer, uint8_t maxLen)
{
    // modo RX continuous
    //SX1278_WriteReg(0x01, 0x85);       // LoRa, RX continuous

    // esperar RX done
    //while ((SX1278_ReadReg(0x12) & 0x40) == 0); // RxDone

    // leer puntero FIFO
    uint8_t addr = SX1278_ReadReg(0x10); // RegFifoRxCurrentAddr
    SX1278_WriteReg(0x0D, addr);         // RegFifoAddrPtr

    // obtener longitud de payload recibido
    uint8_t payloadLen = SX1278_ReadReg(0x13); // RegRxNbBytes
    if (payloadLen > maxLen) payloadLen = maxLen; // seguridad

    // leer todos los bytes
    for (uint8_t i = 0; i < payloadLen; i++)
    {
        buffer[i] = SX1278_ReadReg(0x00);
    }

    //SX1278_WriteReg(0x12, 0xFF); // limpiar flags

    return payloadLen; // cantidad de bytes leídos
}

uint8_t _GetLoraVersion(void)
{
    return SX1278_ReadReg(REG_VERSION);
}       

uint8_t _GetLoraIrqFlags(void)
{
    return SX1278_ReadReg(REG_IRQ_FLAGS);    
}         

int  _GetLoraRSSI(void)
{
    int rssi_dbm = -164 + SX1278_ReadReg(REG_PKT_RSSI_VALUE);
    return rssi_dbm;
}

int _GetLoraLineRSSI(void)
{
    int snr_dbm = -164 + SX1278_ReadReg(REG_RSSI_VALUE);
    return snr_dbm;   
}

int ReadLoraIRQ(void)
{
    if(LORA_GPIO0())
    {
#ifdef _DEBUG_RF
        printf("[_ReadLoraIRQ] IRQ Signal...\n");            
#endif   
        return 1;
    }
    else return 0;
}

void LedMonitor(void);

int test_lora(void)
{
    static uint8_t flag = 0;
    uint8_t version;

    if(flag == 0)
    {
        printf("SPI1_Init OK\n");    
        SPI1_Init();
        printf("LORA_GPIO_Init OK\n");    
        LORA_GPIO_Init();
        printf("SX1278_LoRaInit OK\n");   
        SX1278_LoRaInit();
        printf("test_lora OK...\n");    
        flag = 1;
    }
    //while(1)
    {
        // Leer el registro Version (0x42)
        version = SX1278_ReadReg(REG_VERSION);
        
        if(version == 0x12)
        {
            static uint8_t txx = 0;
            uint8_t txtest[16]= {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};

            txtest[15] = txx++;

            printf("lora ver:%02X OK\n", version);
            //printf("lora REG_OP_MODE %02X\n", SX1278_ReadReg(REG_OP_MODE));    
            //ReadLoraIRQ();
            
            
            /*
            printf("tx start\n");
            SX1278_SendBytes(txtest,sizeof(txtest),1);
            printf("tx done\n");
            SetLoraRxMode();
            ms_delay(1);
            */
            
            SetLoraRxMode();
            ms_delay(1);
          
            printf("lora REG_OP_MODE %02X\n", SX1278_ReadReg(REG_OP_MODE));    
            
            printf("Lora RegIrqFlags %02X\n", SX1278_ReadReg(0x12));    
            printf("Lora RegPktRssiValue %02X\n", SX1278_ReadReg(0x1A));    
            printf("Lora RegPktSnrValue %02X\n", SX1278_ReadReg(0x19));    
            printf("Lora RegRssiValue %02X\n", SX1278_ReadReg(0x1b));    
            
            if(ReadLoraIRQ() == 1)
            {    
                uint8_t rxbuffer[255];
                uint8_t rxlen = SX1278_ReceiveBytes(rxbuffer, sizeof(rxbuffer));
                printf("something %02x[\n",rxlen);
                
                for(int x=0; x<rxlen;x++)
                    printf("%02X",rxbuffer[x]);
                printf("]\n");    
                ClearLoraIRQFlags(0xff);

            }
            //LedMonitor();
            //ms_delay(100);
            
        }
        else
        {
            printf("Error Lora Not detected ver:%02X OK\n", version);    
            ms_delay(3000);
            
        }
    }   
    return 0;
}