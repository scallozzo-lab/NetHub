#ifndef __NVSTORE_H__
    #define __NVSTORE_H__

#include "main.h"

#ifdef _USE_DMX512
    #define _MAXEFFECTEVENTS    3

    typedef struct __attribute__((packed))
    {
        uint8_t enabled;       // 0 = libre, 1 = activo
        uint8_t day;           // 1-31
        uint8_t month;         // 1-12
        uint8_t weekday;       // 0-6
        uint8_t hour;          // 0-23
        uint8_t minute;        // 0-59
        uint8_t second;        // 0-59
        uint8_t effect;        // Código de efecto

        uint8_t red;           // 0-255
        uint8_t green;         // 0-255
        uint8_t blue;          // 0-255
    } stEffectEvent;

    typedef struct __attribute__((packed))
    {
        uint8_t stid;       // 0xE3
        uint8_t listlen;    // Longitud del la lista de effectos por calendario
        stEffectEvent EffectEvent[_MAXEFFECTEVENTS];
        uint16_t crc;
    }stEffects;

#endif

typedef enum
{
    EERAM_POS_NVSTORE_CP1       = 0x000,
    EERAM_POS_NVSTORE_CP2       = 0x020,
    
    EERAM_POS_EVENTS_CP1        = 0x040,
    EERAM_POS_EVENTS_CP2        = 0x058,

#ifdef _USE_DMX512
    EERAM_POS_EFFECTS_CP1       = 0x06F,
    EERAM_POS_EFFECTS_CP2       = EERAM_POS_EFFECTS_CP1 + sizeof(stEffects),
#endif

    EERAM_POS_nu_end            = 0x7BF,
  
    EERAM_POS_MODEMCFG          = 0x7C0,
    EERAM_POS_MODEMCFG_end      = 0x7DF,
    
    EERAM_POS_CONFIGID          = 0x7E0,
    EERAM_POS_CONFIGID_end      = 0x7FF
}eeeramlayout;

typedef enum
{
    EERAM_ERR_MEM_FAILURE       = 1,
    EERAM_ERR_UNCONFIG_DEV      = 2,
    EERAM_ERR_ST1_CORRUPTED     = 3,
    EERAM_ERR_ST2_CORRUPTED     = 4,
}envstoreerrs;

typedef struct __attribute__((packed))
{
    uint8_t stid;
    uint8_t dev_serial[12];
    uint8_t dev_id[6];
    uint8_t _nu[10];
    uint16_t crc;
}stConfigid;

typedef struct __attribute__((packed))
{
    uint8_t stid;
    uint8_t _nu[28];
    uint16_t crc;
}stModemCfg;

typedef struct __attribute__((packed))
{
    uint8_t  stid;             // 0xC1 / 0xC2
    //--------------- Estructura dentro de los 18 bytes de datos de Luma32 ----------------------------
    uint16_t res;              // libre (!) 
    uint16_t currV;            // Tensión actual
    uint16_t currI;            // Corriente actual + cospi

    uint32_t timerunning;      // Tiempo de funcionamiento de las luminarias (Expresado en segundos)
  
    uint64_t t_wxs;            // Watts x Segundo (Teórico)     // Para Tx 32bits (Wm o Wh)
    uint64_t r_wxs;            // Watts x Segundo (Real)        // Para Tx 32bits (Wm o Wh)
    //------------------------------------------------------------------------------------------------
    uint16_t crc;              // Crc de estructura
}stNVStore;

typedef struct __attribute__((packed))
{
    uint8_t  stid;             // 0xE1 / 0xE2
    //--------------- Estructura dentro de los 18 bytes de datos de Luma32 ----------------------------
    uint8_t  status;
    uint8_t  idx;
    uint8_t  buff[16];
    //------------------------------------------------------------------------------------------------ 
    uint16_t crc;
}stNVEvents;


int _InitNVStore(void);
int _NVCreateConfig(uint8_t *id);
uint8_t *_GetHubId(void);
stEffects *_GetNVEffects(void);
int _InitNVEffects(void);


#endif