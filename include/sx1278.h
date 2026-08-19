#ifndef __SX1278_H__
    #define __SX1278_H__

#define _SX1278_SETTXMODE       1
#define _SX1278_WAIT4TXDONE     2


typedef enum {
    REG_FIFO                     = 0x00,
    REG_OP_MODE                  = 0x01,

    // Common settings
    REG_FRF_MSB                  = 0x06,
    REG_FRF_MID                  = 0x07,
    REG_FRF_LSB                  = 0x08,

    // Transmit settings
    REG_PA_CONFIG                = 0x09,
    REG_PA_RAMP                  = 0x0A,
    REG_OCP                      = 0x0B,

    // Receive settings
    REG_LNA                      = 0x0C,

    // LoRa registers
    REG_FIFO_ADDR_PTR            = 0x0D,
    REG_FIFO_TX_BASE_ADDR        = 0x0E,
    REG_FIFO_RX_BASE_ADDR        = 0x0F,
    REG_FIFO_RX_CURRENT_ADDR     = 0x10,
    REG_IRQ_FLAGS_MASK           = 0x11,
    REG_IRQ_FLAGS                = 0x12,
    REG_RX_NB_BYTES              = 0x13,
    REG_RX_HEADER_CNT_VALUE_MSB  = 0x14,
    REG_RX_HEADER_CNT_VALUE_LSB  = 0x15,
    REG_RX_PACKET_CNT_VALUE_MSB  = 0x16,
    REG_RX_PACKET_CNT_VALUE_LSB  = 0x17,
    REG_MODEM_STAT               = 0x18,
    REG_PKT_SNR_VALUE            = 0x19,
    REG_PKT_RSSI_VALUE           = 0x1A,
    REG_RSSI_VALUE               = 0x1B,
    REG_HOP_CHANNEL              = 0x1C,
    REG_MODEM_CONFIG1            = 0x1D,
    REG_MODEM_CONFIG2            = 0x1E,
    REG_SYMB_TIMEOUT_LSB         = 0x1F,
    REG_PREAMBLE_MSB             = 0x20,
    REG_PREAMBLE_LSB             = 0x21,
    REG_PAYLOAD_LENGTH           = 0x22,
    REG_MAX_PAYLOAD_LENGTH       = 0x23,
    REG_HOP_PERIOD               = 0x24,
    REG_FIFO_RX_BYTE_ADDR        = 0x25,
    REG_MODEM_CONFIG3            = 0x26,

    // Frequency hopping
    REG_FEI_MSB                  = 0x28,
    REG_FEI_MID                  = 0x29,
    REG_FEI_LSB                  = 0x2A,

    // RSSI wideband
    REG_RSSI_WIDEBAND            = 0x2C,

    // Detection settings
    REG_DETECT_OPTIMIZE          = 0x31,
    REG_INVERTIQ                 = 0x33,
    REG_DETECTION_THRESHOLD      = 0x37,
    REG_SYNC_WORD                = 0x39,
    REG_INVERTIQ2                = 0x3B,

    // DIO mapping
    REG_DIO_MAPPING1             = 0x40,
    REG_DIO_MAPPING2             = 0x41,

    // Version and temperature
    REG_VERSION                  = 0x42,
    REG_PLL_HOP                  = 0x44,
    REG_TCXO                     = 0x4B,
    REG_PA_DAC                   = 0x4D,

    // AGC / Test
    REG_FORMER_TEMP              = 0x5B,
    REG_AGC_REF                  = 0x61,
    REG_AGC_THRESH1              = 0x62,
    REG_AGC_THRESH2              = 0x63,
    REG_AGC_THRESH3              = 0x64,

    // Test registers
    REG_TEST2F                   = 0x2F,
    REG_TEST30                   = 0x30,
    REG_TEST36                   = 0x36,
    REG_TEST3A                   = 0x3A,
    REG_TEST48                   = 0x48,
    REG_TEST5A                   = 0x5A,
    REG_TEST6F                   = 0x6F,
    REG_TEST70                   = 0x70,

    // End of register map
} sx1278_register_t;


// -----------------------------------------------------------------------------
// LoRa Mode DIO Mappings
// -----------------------------------------------------------------------------
typedef enum {
    // DIO0 (bits 7:6)
    LORA_DIO0_RXDONE_TXDONE          = (0b00 << 6),
    LORA_DIO0_RXTIMEOUT              = (0b01 << 6),
    LORA_DIO0_FHSS_CHANGE_CHANNEL    = (0b10 << 6),
    LORA_DIO0_CAD_DONE               = (0b11 << 6),

    // DIO1 (bits 5:4)
    LORA_DIO1_RXTIMEOUT_FHSS_CHANGE  = (0b00 << 4),
    LORA_DIO1_FHSS_CHANGE_CHANNEL    = (0b01 << 4),
    LORA_DIO1_CAD_DETECTED           = (0b10 << 4),

    // DIO2 (bits 3:2)
    LORA_DIO2_FHSS_CHANGE_CHANNEL    = (0b00 << 2),
    LORA_DIO2_CAD_DETECTED           = (0b01 << 2),

    // DIO3 (bits 1:0)
    LORA_DIO3_CAD_DONE               = (0b00),
    LORA_DIO3_FHSS_CHANGE_CHANNEL    = (0b01 << 0),

    // DIO4 (RegDioMapping2 bits 7:6)
    LORA_DIO4_CAD_DETECTED           = (0b00 << 6),
    LORA_DIO4_FHSS_CHANGE_CHANNEL2   = (0b01 << 6),

    // DIO5 (RegDioMapping2 bits 5:4)
    LORA_DIO5_MODE_READY             = (0b00 << 4)
} sx1278_lora_dio_mapping_t;

// -----------------------------------------------------------------------------
// FSK/OOK Mode DIO Mappings
// -----------------------------------------------------------------------------
typedef enum {
    // DIO0 (bits 7:6)
    FSK_DIO0_PAYLOAD_READY           = (0b00 << 6),
    FSK_DIO0_PACKET_SENT             = (0b01 << 6),
    FSK_DIO0_FIFO_LEVEL              = (0b10 << 6),
    FSK_DIO0_FIFO_FULL               = (0b11 << 6),

    // DIO1 (bits 5:4)
    FSK_DIO1_FIFO_LEVEL              = (0b00 << 4),
    FSK_DIO1_FIFO_EMPTY              = (0b01 << 4),
    FSK_DIO1_FIFO_FULL               = (0b10 << 4),
    FSK_DIO1_FIFO_OVERRUN            = (0b11 << 4),

    // DIO2 (bits 3:2)
    FSK_DIO2_FIFO_EMPTY              = (0b00 << 2),
    FSK_DIO2_FIFO_FULL               = (0b01 << 2),
    FSK_DIO2_SYNC_ADDR               = (0b10 << 2),

    // DIO3 (bits 1:0)
    FSK_DIO3_FIFO_FULL               = (0b00),
    FSK_DIO3_FIFO_EMPTY              = (0b01 << 0),
    FSK_DIO3_FIFO_LEVEL              = (0b10 << 0),
    FSK_DIO3_FIFO_OVERRUN            = (0b11 << 0),

    // DIO4 (RegDioMapping2 bits 7:6)
    FSK_DIO4_PREAMBLE_DETECT         = (0b00 << 6),
    FSK_DIO4_SYNC_ADDR_MATCH         = (0b01 << 6),
    FSK_DIO4_RSSI                    = (0b10 << 6),
    FSK_DIO4_TIMEOUT                 = (0b11 << 6),

    // DIO5 (RegDioMapping2 bits 5:4)
    FSK_DIO5_MODE_READY              = (0b00 << 4),
    FSK_DIO5_CLOCK_OUT               = (0b01 << 4),
    FSK_DIO5_PLL_LOCK                = (0b10 << 4),
    FSK_DIO5_DATA                    = (0b11 << 4)
} sx1278_fsk_dio_mapping_t;

typedef enum
{
    LORA_IRQ_RX_TIMEOUT           = (1 << 7),  // RxTimeout
    LORA_IRQ_RX_DONE              = (1 << 6),  // RxDone
    LORA_IRQ_PAYLOAD_CRC_ERROR    = (1 << 5),  // PayloadCrcError
    LORA_IRQ_VALID_HEADER         = (1 << 4),  // ValidHeader
    LORA_IRQ_TX_DONE              = (1 << 3),  // TxDone
    LORA_IRQ_CAD_DONE             = (1 << 2),  // CadDone
    LORA_IRQ_FHSS_CHANGE_CHANNEL  = (1 << 1),  // FhssChangeChannel
    LORA_IRQ_CAD_DETECTED         = (1 << 0)   // CadDetected
} lora_irq_flags_t;

// Bit masks
#define LONG_RANGE_MODE            0x80  // 1 = LoRa mode, 0 = FSK/OOK
#define ACCESS_SHARED_REG          0x40  // 1 = access FSK registers while in LoRa
#define LOW_FREQUENCY_MODE_ON      0x08  // 1 = LF (<= 525 MHz), 0 = HF (> 525 MHz)

// Operation modes (bits 2:0)
#define MODE_SLEEP                 0x00
#define MODE_STDBY                 0x01
#define MODE_FSTX                  0x02
#define MODE_TX                    0x03
#define MODE_FSRX                  0x04
#define MODE_RX_CONTINUOUS         0x05
#define MODE_RX_SINGLE             0x06
#define MODE_CAD                   0x07

void LORA_GPIO_Init(void);
void SX1278_LoRaInit(void);
void SetLoraRxMode(void);
void ClearLoraIRQFlags(uint8_t f);
void SX1278_SendBytes(uint8_t *data, uint8_t len, uint8_t flag);
uint8_t SX1278_ReceiveBytes(uint8_t *buffer, uint8_t maxLen);
uint8_t _GetLoraVersion(void);
uint8_t _GetLoraIrqFlags(void);
int _GetLoraRSSI(void);
int _GetLoraLineRSSI(void);
int ReadLoraIRQ(void);
#endif