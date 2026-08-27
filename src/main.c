/*------------------------------------- NetHub STM32 ---------------------------------------------
 Plataforma ST STM32 19.3.0
 ----------------------------------------------------------------------------------------------------
 Historial de modificaciones:
 ------------------------------------------------------------------------------------------
 # [MODEM] Corregir que a veces el modem arranca pero nunca se conecta, se debería implementar que reinicie por HW. [Corregido]
 # [MODEM] Cuando conecta a veces la respuesta trae mas data y se interpreta como error y vuelve a iniciar.
 # [MODEM] A veces NETOpen devuelve error 0,4 y es necesario reiniciar por completo.
 # [MODEM] A veces la transmisiones dan timeout y no vuelve a enviar.                                               [Corregido]
 # [MODEM] Corregir que pueda detectar desconexión en standby y volver a iniciar.                                   [Corregido]
 # [MODEM] Verificar: Si no hay servidor, o el modem no conecta debe seguir usando GNSS 
 # [SRTC]  Verificar: Conversión UTC a TimeStamp tiene un desvio de unos 6 minutos
 # [LCD] Mejorar que solo imprima cuando haya cambios.
 # [LCD] Implementar gráficos y segundo display.
 # [STM32] Verificar la lectura de ADC de temperatura, parece estar fuera de rango.
 ------------------------------------------------------------------------------------------
 
 [27/08/2026] SCALLOZZO
    # Se agrega manejo de TX USART1 por DMA.
    # Se mejora el manejo del RTC desde interrupción.
    # Se agrega flag de timeout de comunicación entre Hub<->Srv  
 
 [26/08/2026] SCALLOZZO
    # Se agrega RTC desde GPS con calculo de fecha y hora local  
 
 [26/08/2026] SCALLOZZO
    # Se agrega lectura del GPS/GNSS.  
 
 [25/08/2026] SCALLOZZO
    # Se corrige I2C para EERAM que hacia rollover después de 255 bytes.
    # Se agrega _InitNVEffects
 
 [22/08/2026] SCALLOZZO
    # Se agrega RX USART1 por DMA, para el SIM7670, se habilitada con _USART1_DMA_MODE
 
 [18/08/2026] SCALLOZZO
    # Se agrega bit SSTATUS_STS_FWUPDATE_ENABLE para habilitar actualización remota de Firmware.
 
 [18/08/2026] SCALLOZZO
    # Se agrega protocolo dmx512 y efectos de led básicos.
    Nota: El controlador se debe conectar por RS485, en este caso siempre se transmite, así que solo se utiliza PB15 como TX.  
 
 [23/02/2026] SCALLOZZO
    # Se agrega control de timeout de proceso de modem 
    #define _CANTMAX_TXSRV_RETRY_TO 10
    #define _CANTMAX_TXSRV_RETRY    20
    #define _CANTMAX_CONN_RETRY     30
    # Se agrega control para inicio en caso de mensaje de re-arranque del sim7670

 [22/02/2026] SCALLOZZO
    # Agregado de updatefw. FALTA VER POR QUÉ CUANDO SALTA A EJECUTAR NETBOOT SE CUELGA (JMP_TO_BOOT)
 
 [09/12/2025] SCALLOZZO
    # Implementación de LCD básico.
 
 [16/11/2025] SCALLOZZO
    # Comunicación con SIM7670: Implementación parcial
 
 [15/10/2025] SCALLOZZO
    # Inicio como versión de HUB (espressif32 Ver 4.4)
 
 [04/03/2025] SCALLOZZO
    # Generación Versión Base Luma32 (espressif32 Ver 4.4)
 
 ----------------------------------------------------------------------------------------*/

#include "main.h"
#include "timer.h"
#include "adc.h"
#include "pwm.h"
#include "usart.h"
#include "systick.h"
#include "_ltproto.h"
#include "_24c256.h"
#include "_sh1106.h"
#include "spimaster.h"
#include "i2cmaster.h"
#include "_wireone.h"
#include "nvstore.h"
#include "fw_version.h"
#include "fwupdate.h"
#include "srtc.h"
#include <stdbool.h>
#include <string.h>

uint32_t STM32_UUID[3] = {0};
uint32_t TimeRunning = 0;
static uint16_t  Mainstatus = 0;
static bool iwdg_running = false;

int test_spi(void);
int test_lora(void);
void write_optionbytes(void);

uint16_t _GetMainStatus(void)
{
    return Mainstatus;    
}

void IWDG_Enable(uint8_t prescaler, uint16_t reload)
{
    // Habilitar acceso a PR y RLR
    IWDG->KR = 0x5555;

    // Set prescaler
    IWDG->PR = prescaler & 0x7;

    // Set reload value (max 0x0FFF)
    IWDG->RLR = reload & 0x0FFF;

    // Recargar contador
    IWDG->KR = 0xAAAA;

    // Arrancar watchdog
    IWDG->KR = 0xCCCC;

    iwdg_running = true;
}

bool _GetDG_Enabled(void)
{
    return iwdg_running;
}

// Refrescar watchdog (resetear el contador)
static inline void IWDG_Refresh(void)
{
    IWDG->KR = 0xAAAA;   // clave para recargar el contador
}

void print_hex8(const uint8_t *p, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        printf("%02X ", p[i]);
        if ((i & 0x0F) == 0x0F) printf("\n");
    }
    if (n % 16) printf("\n");
}

/* read_option_bytes.c
 * Small helper to dump STM32F1 option bytes.
 * Works on a "bare-metal" project (CMSIS present).
 *
 * Usa: Llama read_option_bytes() desde tu init, y envía el
 * resultado por printf (semihosting) o por UART (si printf está redirigido).
 *
 * Dirección física de Option Bytes: 0x1FFF F800 .. 0x1FFF F80F (STM32F1).
 * (Ver PM0075 para el layout e interpretación). :contentReference[oaicite:1]{index=1}
 */
void read_option_bytes(void)
{
    volatile const uint8_t *opt = (volatile const uint8_t *)0x1FFFF800U;
    /* Option bytes block typical length for F1 is 16 bytes (0x1FFF F800 .. 0x1FFF F80F).
       Some tools show them as 4 words (option byte 1..4) where each byte has its complement
       (nXXX / XXX). Consult PM0075 for la interpretación exacta. */
    const size_t OPT_LEN = 16;

    printf("=== STM32 option bytes raw dump ===\n");
    print_hex8(opt, OPT_LEN);

    /* Also read the FLASH OBR and WRPR registers which expose option status */
    uint32_t obr = FLASH->OBR;   /* Option byte register (readback status) */
    uint32_t wrpr = FLASH->WRPR; /* Write protection registers (may be multiple on some devices) */

    printf("\nFLASH->OBR = 0x%08lX\n", (unsigned long)obr);
    printf("FLASH->WRPR = 0x%08lX\n", (unsigned long)wrpr);

    /* Helpful human-readable hints (common fields):
       - Read-out protection (RDP) level is usually determined from option bytes / OBR.
         RDP correct value when unlocked often appears as 0xA5 in the option area.
       - Data0 / Data1 are two user-data bytes (often at 0x1FFFF804).
       - WRPn bits control write protection for flash pages.
       For full decoding, compare the raw bytes above with the device programming manual. */
    printf("\nHints / quick checks:\n");

    /* Quick heuristic: check the typical RDP byte location(s)
       (different references show slightly different packing; check manual if unsure). */
    uint8_t raw0 = opt[0]; /* first byte at 0x1FFF F800 */
    uint8_t raw1 = opt[1];
    uint8_t raw2 = opt[2];
    uint8_t raw3 = opt[3];

    printf("First 4 option-bytes (0x1FFF F800..F803): %02X %02X %02X %02X\n", raw0, raw1, raw2, raw3);

    /* A common pattern: word at 0x1FFF F800 contains (nUSER, USER, nRDP, RDP) bytes,
       and 0x1FFF F804 contains data bytes (nData1, Data1, nData0, Data0).
       But *always* verify in your device manual (PM0075). */
    printf("Note: Many references show packing like (nUSER, USER, nRDP, RDP) in the first word.\n");
    printf("See STM32F1 Flash programming manual (PM0075) for exact mapping and complements.\n");
    printf("Reference: PM0075 (STM32F10xxx flash memory programming manual).\n");
}


void SystemClock_HSE_Direct(void)
{
    // 1. Habilitar HSE
    RCC->CR |= RCC_CR_HSEON;
    while(!(RCC->CR & RCC_CR_HSERDY)); // esperar que HSE esté estable

    // 2. Configurar Flash latency
    FLASH->ACR &= ~FLASH_ACR_LATENCY; // 0 wait states para 8MHz
    FLASH->ACR |= FLASH_ACR_PRFTBE;   // habilitar prefetch

    // 3. Seleccionar HSE como SYSCLK
    RCC->CFGR &= ~RCC_CFGR_SW;
    RCC->CFGR |= RCC_CFGR_SW_HSE;
    while((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSE);

    // 4. Configurar buses
    RCC->CFGR &= ~RCC_CFGR_PPRE1; // APB1 = HCLK
    RCC->CFGR &= ~RCC_CFGR_PPRE2; // APB2 = HCLK
}


void SystemClock_HSE_72MHz(void)
{
    // 1. Habilitar HSE
    RCC->CR |= RCC_CR_HSEON;
    while(!(RCC->CR & RCC_CR_HSERDY)); // esperar que HSE esté estable

    // 2. Configurar Flash prefetch y latencia
    FLASH->ACR |= FLASH_ACR_PRFTBE;
    FLASH->ACR &= ~FLASH_ACR_LATENCY;
    FLASH->ACR |= FLASH_ACR_LATENCY_2; // 2 wait states para 72MHz

    // 3. Configurar PLL
    // PLL source = HSE, multiplicador 9 → 8MHz * 9 = 72MHz
    RCC->CFGR |= RCC_CFGR_PLLSRC;      // HSE como PLL source
    RCC->CFGR &= ~RCC_CFGR_PLLXTPRE;   // HSE sin dividir por 2
    RCC->CFGR &= ~RCC_CFGR_PLLMULL;    
    RCC->CFGR |= RCC_CFGR_PLLMULL9;    // PLL *9

    // 4. Habilitar PLL
    RCC->CR |= RCC_CR_PLLON;
    while(!(RCC->CR & RCC_CR_PLLRDY));

    // 5. Seleccionar PLL como SYSCLK
    RCC->CFGR &= ~RCC_CFGR_SW;
    RCC->CFGR |= RCC_CFGR_SW_PLL;
    while((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);

    // 6. Configurar buses (APB1 = 36MHz, APB2 = 72MHz)
    RCC->CFGR &= ~RCC_CFGR_PPRE1;
    RCC->CFGR |= RCC_CFGR_PPRE1_DIV2; // APB1 = 36MHz
    RCC->CFGR &= ~RCC_CFGR_PPRE2;      // APB2 = 72MHz
}


void ms_delay(int ms)
{
   while (ms-- > 0) {
      volatile int x=500;
      while (x-- > 0)
         __asm("nop");
   }
}

void ReadUniqueID(void)
{
    STM32_UUID[0] = *(uint32_t*)0x1FFFF7E8; // Low
    STM32_UUID[1] = *(uint32_t*)0x1FFFF7EC; // Mid
    STM32_UUID[2] = *(uint32_t*)0x1FFFF7F0; // High
}

void PrintUniqueID(void)
{
    printf("%08lX-%08lX-%08lX\n",
           STM32_UUID[2],
           STM32_UUID[1],
           STM32_UUID[0]);
}

uint32_t _GetUniqueID(uint8_t idx)
{
    return STM32_UUID[idx];    
}

uint8_t *_GetUniqueIDPtr(void)
{
    return (uint8_t*)STM32_UUID;    
}

uint32_t get_sysclk_source(void)
{
    uint32_t src = (RCC->CFGR & RCC_CFGR_SWS) >> RCC_CFGR_SWS_Pos;
    return src; // 0 = HSI, 1 = HSE, 2 = PLL
}

uint32_t get_sysclk_freq(void)
{
    uint32_t src = get_sysclk_source();
    switch(src)
    {
        case 0: return 8000000; // HSI
        case 1: return 8000000; // HSE (depends on crystal)
        case 2: // PLL
        {
            uint32_t pllmul = ((RCC->CFGR & RCC_CFGR_PLLMULL) >> RCC_CFGR_PLLMULL_Pos) + 2;
            uint32_t pllsrc = (RCC->CFGR & RCC_CFGR_PLLSRC);
            if (pllsrc) return 8000000 * pllmul; // HSE * PLL
            else return 8000000 * pllmul / 2;    // HSI/2 * PLL
        }
    }
    return 0;
}

uint32_t _GetTimeRunning(void)
{
    return TimeRunning;    
}

uint16_t _GetFirmwareVer(void)
{
    return FW_VERSION_0<<8 | FW_VERSION_1;
}

uint16_t GetFlashSizeKB(void)
{
    uint16_t *flash_size = (uint16_t*)0x1FFFF7E0;
    return *flash_size;      // returns 64 or 128 usually
}

void _1SecFunctions(void)
{
    //RTC_Soft_Tick(_GetRtcPtr());
    rtc_soft_t *rtc = _GetRtcPtr();
    
    #ifdef _USE_DEBUG_SRTC
    // DEBUG: imprimir fecha y hora
    printf("FyH: %02d/%02d/%04d %02d:%02d:%02d\r\n",
           rtc->day,
           rtc->month,
           rtc->year,
           rtc->hour,
           rtc->min,
           rtc->sec);
#endif

    
    TimeRunning++;
}
   
void _1msFunctions(void)
{
   _ProcSrvCom();
   _ProcLTSlv(); 
}

void _10msFunctions(void)
{
    static uint8_t xDiv = 0, xDiv100ms = 0;
  
    _ProcLTProto();
    _FwUpdateCtrl();

#ifdef _USE_DMX512    
    _ProcLEDEffect();
#endif

    if(xDiv100ms++ >= 50)
    {
        LedMonitor();
        LedMonitorGreen();
        xDiv100ms = 0;
    }

    if(xDiv++ >= 95)
    {
        _1SecFunctions();
        xDiv = 0;
        
        //_test_eeram();
        //test_adc();
    }
}

int main(void)
{
    /* Fuerza la compilación de los atributos de versión */
    const volatile uint8_t *__attribute__((unused)) __nu__ = (*fw_ver_id)? (const volatile uint8_t*) 0 : __nu__; 

#ifdef _CLKMODE_EXTERNAL_8MHZ
    SystemClock_HSE_Direct();
#endif
#ifdef _CLKMODE_EXTERNAL_72MHZ
    SystemClock_HSE_72MHz();
#else
    #pragma message("USING INTERNAL CLOCK SRC 8MHZ")
#endif
    
    IWDG_Enable(255, 1000);

    // WireOne se debe inicializar siempre para poder utilizar los leds con otros propositos.
    _InitWireOne();

    Timer2_Init_us(get_sysclk_freq() / 1000000);

#if(_UART_DEBUG == _USART_2)
    USART2_Init(get_sysclk_freq(), 0);                    // init USART2
#elif(_UART_DEBUG == _USART_1)
    USART1_Init(get_sysclk_freq(), 1);                   // init USART1
#endif

#ifdef _USE_SYSTICK
    SysTick_Init(get_sysclk_freq());
#endif

#ifdef _USE_SI2C
    SI2C_GPIO_Init();
 #ifdef _USE_SH1106
    _SelLCD1();
    SH1106_init();
    _SelLCD2();
    SH1106_init();
#endif
#endif
    // Init I2C (main controller)
    SI2C1_GPIO_Init();

#ifdef _USE_DMX512
    // Init MDX512 (PB15)
    DMX_GPIO_Init();
#endif

#ifdef _UART_DEBUG
    #if FW_TYPE == 'H'     
        printf("**** NETHub Ver %01X.%01X-%c *****\n", FW_VERSION_0, FW_VERSION_1, FW_VERSION_REV);
    #elif FW_TYPE == 'L'     
        printf("**** LTX Ver %01X.%01X-%c *****\n", FW_VERSION_0, FW_VERSION_1, FW_VERSION_REV);
    #endif
#endif

#ifdef _USE_USART1_MODEM
    // PARA SERVER COM
    USART1_Init(get_sysclk_freq(), 1);                   // init USART1
#endif

    //ADC1_Init(ADC_CH_TEMP);
    ADC1_Init_Temperature();
    ADC2_Init(ADC_CH_PB0);  // sensor LDR
    IWDG_Refresh();

    printf("get_sysclk_source = %d\n", get_sysclk_source());
    uint32_t clk = get_sysclk_freq();
    char buf[50];
    sprintf(buf, "SYSCLK = %lu Hz\r\n", clk);
    printf(buf);
    
    // Init PWM for CH1 & CH4 (PWM de Luminarias)
    _Init_PWM(clk/1000000, 0,0);
    _SIM7670_POWER_OFF;
    
    read_option_bytes();
    //write_optionbytes();

    uint8_t user = OB->USER & 0xFF;
    if((user & 0x01) == 0) {
        // IWDG_HW activo
        printf("IWDG_HW activo\n");
    } else {
        
        printf("IWDG_SF activo\n");
        // IWDG_SW
    }

    printf("Unique Serial->");
    ReadUniqueID();
    PrintUniqueID();

    //printf("Flash Size %d - Test %d\n",GetFlashSizeKB(), Flash_Test128KB());

int nvret;
Reinit:
    
    // Inicialización NVStore
    nvret = _InitNVStore();

    if(nvret == 0)
    {

    }
    else if(nvret == EERAM_ERR_MEM_FAILURE)
        Mainstatus |= MAIN_STS_EERAM_FAILURE;
    else if(nvret == EERAM_ERR_UNCONFIG_DEV)
        Mainstatus |= MAIN_STS_UNCONFIG_DEV;     
    
#ifdef _USE_DUMMY_TEST_DEVICE
    {
        uint8_t dummyid[6] = {1,2,3,4,5,6};
        int r = _NVCreateConfig(dummyid);
        if(r == 0)
            goto Reinit;
    }
#endif

#ifdef _USE_DUMMY_TEST_DMX512
    {
        uint8_t dummydata[3] = {0x5A,0x5A,0x5A,0x5A};
        while(1)
            DMX_SendFrame(dummydata, sizeof(dummydata));
    }
#endif

#ifdef _USE_DMX512
    // Si el EERAM se encuentra inicializa y sin errores
    if(!(Mainstatus & EERAM_ERR_MEM_FAILURE))
    {
        int r = _InitNVEffects();
    
        printf("_InitNVEffects = %d\n",r);
        printf("e1 = %X\n", EERAM_POS_EFFECTS_CP1);
        printf("e2 = %X\n", EERAM_POS_EFFECTS_CP2);
        
#ifdef _USE_DEFAULT_NVRAMEFFECTS
        if(r)
        {
            stEffects Effects = {0};
            
            if(_NVEffectsWrite()) printf("Warning...\n");

            for(int idx = 0; idx < _MAXEFFECTEVENTS; idx++)
            {
                Effects.EffectEvent[idx].enabled = true;
                Effects.EffectEvent[idx].weekday = RTC_WEEKDAY_WEDNESDAY;
                Effects.EffectEvent[idx].day = 0; // no es para un dia especifico
                Effects.EffectEvent[idx].month = 0; // no se especifica (usa weekday)
                Effects.EffectEvent[idx].hour = 12;
                Effects.EffectEvent[idx].minute = 30;
                Effects.EffectEvent[idx].second = 0;
                
                Effects.EffectEvent[idx].effect = 123;
                Effects.EffectEvent[idx].red = 255;
                Effects.EffectEvent[idx].blue = 1;
                Effects.EffectEvent[idx].green = 128;
            }
            
            Effects.listlen = _MAXEFFECTEVENTS;
            stEffects * p_st = _GetNVEffects();
            if(p_st) 
            {
                memcpy(p_st, &Effects, sizeof(Effects));
                r = _NVEffectsWrite();
                printf("Write NV Effects r = %d\n", r);
            }
        }        
#endif

    }
#endif


#ifdef _USE_DUMMY_TEST_SRTC
    testrtc();
#endif

    // Ambos canales apagados
    _SetPWM_CH4(0xffffffflu);

    _InitSrvCom(1);
    _InitLTProtocol();
 
    SPI1_Init();    
    LORA_GPIO_Init();
    SX1278_LoRaInit();
    printf("Lora Version = %02X\n", _GetLoraVersion());

    _FwUpdateInit();

    for (;;) 
    {
        static int32_t tim = 0;
        static uint8_t xdiv = 0;
       
        if((_GetTimerms() - tim) >= 1u)
        {
            _1msFunctions();
            if(xdiv++ >= 9)
            {    
                _10msFunctions();
                xdiv = 0;
            }
            tim = _GetTimerms();
        }
        IWDG_Refresh();
    }
    return 0;
}


uint16_t crc_ccitt(const uint8_t *data, size_t len)
{
    uint16_t crc = 0x0000;    // Init value for CRC-CCITT (XModem)
    const uint16_t poly = 0x1021;

    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;

        for (uint8_t bit = 0; bit < 8; bit++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ poly;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}


int _write(int file, char *ptr, int len)
{
    if (file == STDOUT_FILENO || file == STDERR_FILENO) {
        /* bloqueante: transmite todo */
         while(len--)
#if(_UART_DEBUG == _USART_1)            
            USART1_SendChar(*ptr++);
#elif(_UART_DEBUG == _USART_2)
            USART2_SendChar(*ptr++);
#endif   
        return 0;
    }
    return -1;
}

// verificar!!!!
void write_optionbytes(void)
{
    // 1. Desbloquear la Flash
    FLASH->KEYR = 0x45670123;
    FLASH->KEYR = 0xCDEF89AB;

    // 2. Desbloquear Option Bytes
    FLASH->OPTKEYR = 0x45670123;
    FLASH->OPTKEYR = 0xCDEF89AB;

    // 3. Habilitar programación de OB
    FLASH->CR |= FLASH_CR_OPTPG;

    // 4. Escribir el valor deseado
    // Por ejemplo OB->USER: habilitar IWDG_HW
    OB->USER &= ~(1<<0); // depende del bit exacto en la MCU

    // 5. Iniciar programación
    FLASH->CR |= FLASH_CR_STRT;

    // 6. Esperar flag de completado
    while(FLASH->SR & FLASH_SR_BSY);

    // 7. Bloquear Flash/OB
    FLASH->CR &= ~FLASH_CR_OPTPG;
    FLASH->CR |= FLASH_CR_LOCK;
}


void DelayUs(uint32_t us)
{
    __asm volatile (
        "1: \n"
        "   subs %0, %0, #1 \n"   // 1 cycle
        "   nop \n"               // padding to adjust timing
        "   nop \n"
        "   bne 1b \n"            // 3 cycles when branching, 1 when not
        : "+r" (us)
    );
}

uint8_t _GetBit(const uint8_t data[13], uint16_t bit_index)
{
    if (bit_index >= 13 * 8) {
        return 0;   // fuera de rango
    }

    uint16_t byte_index = bit_index / 8;
    uint8_t  bit_pos    = bit_index % 8;

    return (data[byte_index] & (1U << bit_pos)) != 0;
}

uint32_t _Get_PCReg(void)
{
    uint32_t pc;
    __asm volatile ("mov %0, pc" : "=r" (pc));
    return pc;
}