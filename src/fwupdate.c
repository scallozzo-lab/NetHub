#include "fwupdate.h"
#include "_24c256.h"
#include "fw_version.h"
#include <string.h>

static stFwUpdate FwUpdate = {0};

// ---------- CRC32 ----------
static uint32_t crc32_update(const uint8_t *data, size_t len, uint32_t seed)
{
    uint32_t crc = seed; //0xFFFFFFFF 
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++)
            crc = (crc & 1) ? (crc >> 1) ^ 0xEDB88320 : crc >> 1;
    }
    return crc;
}

static uint32_t hex8_to_u32(const uint8_t *s)
{
    uint32_t v = 0;

    for (int i = 0; i < 8; i++)
    {
        uint8_t c = s[i];
        uint8_t n;

        if (c >= '0' && c <= '9')      n = c - '0';
        else if (c >= 'A' && c <= 'F') n = c - 'A' + 10;
        else if (c >= 'a' && c <= 'f') n = c - 'a' + 10;
        else return 0; // error

        v = (v << 4) | n;
    }

    return v;
}

void jump_to_boot(void)
{
    __disable_irq();

    // Reset NVIC
    for(int i=0;i<8;i++){ NVIC->ICER[i]=0xFFFFFFFF; NVIC->ICPR[i]=0xFFFFFFFF; }

    // Reset SysTick
    SysTick->CTRL = 0;
    SCB->ICSR &= ~(SCB_ICSR_PENDSTCLR_Msk | SCB_ICSR_PENDSVCLR_Msk);

    // Reset SCB Faults
    SCB->SHCSR &= ~(SCB_SHCSR_USGFAULTENA_Msk | SCB_SHCSR_BUSFAULTENA_Msk | SCB_SHCSR_MEMFAULTENA_Msk);

    // Reset APB1 peripherals
    RCC->APB1RSTR = 0xFFFFFFFF;
    RCC->APB1RSTR = 0;

    // Reset APB2 peripherals
    RCC->APB2RSTR = 0xFFFFFFFF;
    RCC->APB2RSTR = 0;

    // Resetear relojes a estado por defecto
    RCC->CR |= RCC_CR_HSION;    // HSI ON
    RCC->CFGR = 0x00000000;     // AHB=SYSCLK, APB1=HCLK, PLL OFF
    RCC->CR &= ~(RCC_CR_PLLON | RCC_CR_HSEON);

    // Cargar MSP y VTOR
    uint32_t app_msp   = *(uint32_t*)FLASH_BASE_ADDRESS;
    uint32_t app_reset = *(uint32_t*)(FLASH_BASE_ADDRESS+4);
    __set_MSP(app_msp);
    
    // De esto se encarga systeminit
    //SCB->VTOR = APP_ADDRESS;

    __DSB(); __ISB();
    __enable_irq();

    // Saltar a la prog.principal
    ((void(*)(void))app_reset)();
}

uint32_t CalcCrc32EEProm(uint32_t len)
{
    #define BLOCK_SIZE 1024

    uint8_t buf[BLOCK_SIZE] = {0};
    uint32_t crc = 0xffffffff;
    int first_block = 1;
    uint16_t nvaddr = 0;

    while(len)
    {
        uint32_t lencp = (len > BLOCK_SIZE) ? BLOCK_SIZE : len;

        // Lee y almacena en el buffer buf
        _eeprom_read(_EEPROM_DEV_ADDR, nvaddr, buf, lencp);

        if(first_block)
        {
            first_block = 0;

            if(lencp >= _FLASH_OFFSET_HEADER + sizeof(stfwverid))
            {
                stfwverid *pst = (stfwverid *)&buf[_FLASH_OFFSET_HEADER];

                if(!memcmp(pst->id, _IDFLAG, sizeof(pst->id)))
                {
                    memset(pst->lenstr, '_', sizeof(pst->lenstr));
                    memset(pst->sigstr, '_', sizeof(pst->sigstr));
                }
                else
                    printf("[CalcCrc32EEProm] Warning FW.Header no presente!\n");
            }
        }

        crc = crc32_update(buf, lencp, crc);

        nvaddr += lencp;
        len    -= lencp;
    }

    return ~crc;
}

void _SetFwReqVersion(uint8_t *ver)
{
    FwUpdate.status |= _UPD_STS_REQ_VERSION;
    FwUpdate.newver[0] = ver[0];
    FwUpdate.newver[1] = ver[1];
    FwUpdate.newver[2] = ver[2];
}

void _SetFramesInfo(uint16_t tframes, uint16_t fsize)
{
    FwUpdate.totframes = tframes;
    FwUpdate.framesize = fsize;
} 

uint16_t _GetFrameTotFrame(void)
{
    return FwUpdate.totframes;
}

uint16_t _GetFrameFrameSize(void)
{
    return FwUpdate.framesize;
}

void _SetFwReqEnd(void)
{
    FwUpdate.status |= _UPD_STS_REQ_VER_STORED;
    FwUpdate.status &= ~_UPD_STS_REQ_VERSION;
}

uint8_t _GetFwUpdateStatus(void)
{
    return FwUpdate.stg;   
}

uint16_t _GetFwUpdateReqFrame(void)
{
    if(FwUpdate.stg == _FW_STG_GET_FRAME)
        return FwUpdate.offset + 1;
    else return 0;
}

void _SetFwUpdateReqFrame(uint16_t f)
{
    if(FwUpdate.stg == _FW_STG_GET_FRAME)
        FwUpdate.offset = f;
}

uint8_t *_GetFwUpdateVer(void)
{
    return (uint8_t*)FwUpdate.newver;
}

uint16_t _GetFwUpdateTxTimer(void)
{
    return FwUpdate.txtimer;
}

void _SetFwUpdateTxTimer(uint16_t t)
{
    FwUpdate.txtimer = t;
}

int _StoreRxFrameEEPROM(uint16_t framepos, uint8_t *buf, uint16_t len)
{
    return _eeprom_write(_EEPROM_DEV_ADDR, framepos * len , buf, len);
}

void _FwUpdateCtrl(void)
{
    if(FwUpdate.txtimer && FwUpdate.stg) FwUpdate.txtimer--;

    switch(FwUpdate.stg)
    {
        case _FW_STG_INIT:
        if(FwUpdate.status & _UPD_STS_REQ_VERSION) 
        {
            FwUpdate.offset = 0;
            FwUpdate.stg = _FW_STG_GET_FRAME;  
        }
        break;

        case _FW_STG_GET_FRAME:
        if(FwUpdate.status & _UPD_STS_REQ_VER_STORED) _FwUpdateInit();
        break;
    }
}

void _FwUpdateInit(void)
{

    uint8_t tmpdata[32];
    
    // Lee los primeros 32 bytes de eeprom 
    _eeprom_read(_EEPROM_DEV_ADDR, _FLASH_OFFSET_HEADER, tmpdata, sizeof(tmpdata));

    stfwverid *pst = (stfwverid *)tmpdata;
   
    if(!memcmp(pst->id, _IDFLAG, sizeof(pst->id)))
    {
        uint32_t eepcrc = hex8_to_u32(pst->sigstr);
        uint32_t eeplen = hex8_to_u32(pst->lenstr);
        uint32_t eepcrccalc = CalcCrc32EEProm(eeplen - 4);

#ifdef _USE_DEBUG_FWUPDATE
        printf("[_FwUpdateInit] Fw Encontrado: len (%ld)\n", eeplen);
        if(eepcrccalc == eepcrc)
            printf("Versión en EEPROM OK (%c.%c.%c) CRC %08lX\n", pst->version[0], pst->version[1], pst->rev, eepcrc);
        else 
            printf("Error de CRC %08lX-%08lX\n", eepcrccalc, eepcrc);
#endif

        if(eepcrccalc == eepcrc)
        {
            // Si las versiones son diferentes, puede re-arrancar para dar lugar a NetBoot
            if(pst->version[0] != (FW_VERSION_0 + 0x30) || pst->version[1] != (FW_VERSION_1 + 0x30) || pst->rev != FW_VERSION_REV )
            {
                printf("[_FwUpdateInit] Nueva Versión en EPROM: Reinicio (%s)\n", (_GetDG_Enabled())? "IWDG" : "JMP");
                uint32_t r = _Get_PCReg();
                printf("PCReg = %08lX\n", r);
                
                if(r >= FLASH_BASE_APP)
                {
                    if(_GetDG_Enabled()) while(1);
                    else jump_to_boot();                
                }
                else
                {
                    printf("[_FwUpdateInit] WAR: NetHUB en zona boot 0x%08lX\n", r);
                }
            }            
        }
    }

#ifdef _USE_DEBUG_FWUPDATE
    else
    { 
        printf("[_FwUpdateInit] Header FW No Encontrador!\n");
    }
#endif
            
}