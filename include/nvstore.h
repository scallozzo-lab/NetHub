#ifndef __NVSTORE_H__
    #define __NVSTORE_H__

#include "main.h"

typedef enum
{
    EERAM_POS_NVSTORE_CP1       = 0x000,
    EERAM_POS_NVSTORE_CP2       = 0x020,
    
    EERAM_POS_EVENTS_CP1        = 0x040,
    EERAM_POS_EVENTS_CP2        = 0x058,

    EERAM_POS_nu                = 0x06F,
    EERAM_POS_nu_end            = 0x7BF,
    
    EERAM_POS_MODEMCFG          = 0x7C0,
    EERAM_POS_MODEMCFG_end      = 0x7DF,
    
    EERAM_POS_CONFIGID          = 0x7E0,
    EERAM_POS_CONFIGID_end      = 0x7FF
}eeeramlayout;

typedef enum
{
    EERAM_ERR_MEM_FAILURE   = 1,
    EERAM_ERR_UNCONFIG_DEV  = 2,
   
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


#endif