#ifndef __MAIN_H__
    #define __MAIN_H__

#include "stm32f1xx.h"
#include <stdio.h>
#include <sys/unistd.h>
#include "stdint.h"
#include <stdbool.h>

/*--------------------------- Configuraciones ------------------*/
#define _NETHUBMODE
#define _USE_SIMCOM_NOECHO
//#define _USE_DUMMY_TEST_DEVICE        // Solo para probar, crea un config id de prueba
//#define _USE_DUMMY_TEST_NOLTSERVICE     // Solo para probar genera lista sin estar conectado a ltserver
//#define _USE_TX_STATUS_CMD_EMPTYDEV     // Transmite el comando de status de dispositivos aunque esté vacio
#define _USE_DMX512                      // Agrega manejo del protocolo para leds 
//#define _USE_DUMMY_TEST_DMX512
/*--------------------------------------------------------------*/

#define _USE_DEBUG_SRVCOM
#define _USE_DEBUG_TXRX
//#define _USE_DEBUG_TX_SRVCOM            
   
//#define _USE_DEBUG_TXRXSLV
//#define _USE_DEBUG_RXTIMEOUT
//#define _USE_DEBUG_TXDONE
#define _USE_DEBUG_NVSTORE
//#define _DEBUG_RF
#define _USE_DEBUG_FWUPDATE

//#define _CLKMODE_EXTERNAL_8MHZ
#define _CLKMODE_EXTERNAL_72MHZ

#define _USART_1    1
#define _USART_2    2


#ifdef _NETHUBMODE
    #define _UART_DEBUG _USART_2       
    #define _USE_USART1_MODEM
    //#define _USE_SYSTICK
    #define _USE_SSPI               // usar softspi debido a que los tracks mosi y miso están invertidos en el pcb
    #define _USE_SI2C
    #define _USE_SH1106
    #define FW_TYPE 'H'
    #define _CANT_MAX_SLV   100
    //#define _LORA_MODE_500
#else
    #define _UART_DEBUG _USART_1
    #define FW_TYPE 'L'
    #define _USE_WIREONE
#endif

int32_t _GetTimerms(void);
uint16_t _GetMainStatus(void);
uint16_t crc_ccitt(const uint8_t *data, size_t len);
void ms_delay(int ms);
void ReadUniqueID(void);
uint32_t _GetUniqueID(uint8_t idx);
uint8_t *_GetUniqueIDPtr(void);
void DelayUs(uint32_t us);
uint8_t _GetBit(const uint8_t data[13], uint16_t bit_index);
uint32_t _Get_PCReg(void);
bool _GetDG_Enabled(void);


typedef enum {
    BIT0  = (1U << 0),
    BIT1  = (1U << 1),
    BIT2  = (1U << 2),
    BIT3  = (1U << 3),
    BIT4  = (1U << 4),
    BIT5  = (1U << 5),
    BIT6  = (1U << 6),
    BIT7  = (1U << 7),
    BIT8  = (1U << 8),
    BIT9  = (1U << 9),
    BIT10 = (1U << 10),
    BIT11 = (1U << 11),
    BIT12 = (1U << 12),
    BIT13 = (1U << 13),
    BIT14 = (1U << 14),
    BIT15 = (1U << 15),
    BIT16 = (1U << 16),
    BIT17 = (1U << 17),
    BIT18 = (1U << 18),
    BIT19 = (1U << 19),
    BIT20 = (1U << 20),
    BIT21 = (1U << 21),
    BIT22 = (1U << 22),
    BIT23 = (1U << 23),
    BIT24 = (1U << 24),
    BIT25 = (1U << 25),
    BIT26 = (1U << 26),
    BIT27 = (1U << 27),
    BIT28 = (1U << 28),
    BIT29 = (1U << 29),
    BIT30 = (1U << 30),
    BIT31 = (1U << 31)
} BitMask_t;

typedef enum
{
    MAIN_STS_UNCONFIG_DEV           = BIT0,
    MAIN_STS_EERAM_FAILURE          = BIT1,
    MAIN_STS_IMU_FAILURE            = BIT2,
    MAIN_STS_LDR_FAILURE            = BIT3,
    MAIN_STS_PS_FAILURE             = BIT4,
    MAIN_STS_ONEWIRE_OPENED         = BIT5,
    MAIN_STS_EXTLED_FAILURE         = BIT6,
    MAIN_STS_DPL_DISCONNECTED       = BIT7,
    MAIN_STS_DPL_FAILURE            = BIT8,
    MAIN_STS_PIR_FAILURE            = BIT9,
    MAIN_STS_VSENSE_DISCONNECTED    = BIT10,
    MAIN_STS_IT1_OUTOFRANGE         = BIT11,
    MAIN_STS_IT2_OUTOFRANGE         = BIT12,
    MAIN_STS_res13                  = BIT13,
    MAIN_STS_res14                  = BIT14,
    MAIN_STS_LORA_FAILURE           = BIT15,
}emainstatus;



#endif
