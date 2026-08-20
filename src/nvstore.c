/*----------------------------------- NV STORE --------------------------------
// Mapa de memoria:
000-7FF (2048 Bytes)


----------------------------------------
(000-01F) NVStore Copia 1 (32 Bytes)
(020-03F) NVStore Copia 2 (32 Bytes)
----------------------------------------
(040-057) NVEvents Copia 1 (21 Bytes)
(058-06D) NVEvents Copia 2 (21 Bytes)

(06F-7BF) nu 
----------------------------------------
(7C0-7DF) - Modem Config (31 Bytes Protegida)
1 bytes St-ID (0xA3) NetHub Modem Cfg
28 bytes a definir
2 Crc
----------------------------------------
(7E0-7FF) Dev-ID (31 Bytes Protegida)
1  bytes St-ID (0xA0) NetHub / 0xA1 LTx
12 bytes dev-serial
6  bytes HubId/DevId   
10 bytes libres  
2 Crc 
------------------------------------------------------------------------------*/
#include "nvstore.h"
#include "_47l16.h"

static stConfigid Configid = {0};
static stNVStore NVStore = {0};
static stNVEvents NVEvents = {0};
#ifdef _NETHUBMODE
    static stModemCfg ModemCfg = {0};
#endif

int _InitNVStore(void)
{
    uint8_t status; 
    uint8_t ack = EERAM_ReadStatus(&status);
    if(ack)
    {    
#ifdef _USE_DEBUG_NVSTORE
        printf("[_InitNVStore] EERAM_ReadStatus OK %02X\n", status);
#endif 
        if(!(status & STS_REG_ASE)) 
        {
            ack = EERAM_EnableAutoStore();
            if(ack) 
            {    
#ifdef _USE_DEBUG_NVSTORE          
                printf("[_InitNVStore] EERAM_EnableAutoStore OK %02X\n", status);  
#endif
            }
            else
            {
                printf("[_InitNVStore] EERAM_EnableAutoStore Error\n");
                return EERAM_ERR_MEM_FAILURE;
            }
        }
#ifdef _USE_DEBUG_NVSTORE          
        else printf("[_InitNVStore] EERAM_ReadStatus OK (Already Set) %02X\n", status);  
#endif
    }
    else
    {
        printf("[_InitNVStore] EERAM_ReadStatus Error\n");
        return EERAM_ERR_MEM_FAILURE;
    }

    if(_eeram_read(_EERAM_DEV_ADDR, EERAM_POS_CONFIGID, (uint8_t*)&Configid, sizeof(Configid)))
    {
       if(Configid.stid == 0xA0 || Configid.stid == 0xA1)
       {
            uint16_t crc = crc_ccitt((uint8_t*)&Configid, sizeof(Configid) - 2);
            if(crc != Configid.crc)
            {
                printf("[_InitNVStore] NoConfig Id (CRC Error)\n");
                return EERAM_ERR_UNCONFIG_DEV;
            }
            else
            {
#ifdef _USE_DEBUG_NVSTORE          
                printf("[_InitNVStore] Config ID OK\n");
                printf("stid %02X\n",Configid.stid);
                printf("dev_serial->[");
                for(int idx = 0;idx < sizeof(Configid.dev_serial); idx++)
                    printf("%02X",Configid.dev_serial[idx]);
                printf("]\n");
                printf("dev_id->[");
                for(int idx = 0;idx < sizeof(Configid.dev_id); idx++)
                    printf("%02X",Configid.dev_id[idx]);
                printf("]\n");  
#endif
            }
       }
       else
       {
            printf("[_InitNVStore] NoConfig Id\n");
            return EERAM_ERR_UNCONFIG_DEV;
       }  
    }
    else
    {
        printf("[_InitNVStore] _eeram_read Error\n");
        return EERAM_ERR_MEM_FAILURE;
    }

    return 0;
}

int _NVCreateConfig(uint8_t *id)
{
    if(id)
    {
        if((_GetMainStatus() & MAIN_STS_UNCONFIG_DEV))
        {
            stConfigid Configid = {0};

#ifdef _NETHUBMODE
            Configid.stid = 0xA0;
#else
            Configid.stid = 0xA1;
#endif
            ReadUniqueID();
            memcpy((uint8_t*)Configid.dev_serial, _GetUniqueIDPtr(), sizeof(Configid.dev_serial));
            memcpy((uint8_t*)Configid.dev_id, id, sizeof(Configid.dev_id));
            Configid.crc = crc_ccitt((uint8_t*)&Configid, sizeof(Configid) - 2);
            int r = _eeram_write(_EERAM_DEV_ADDR, EERAM_POS_CONFIGID, (uint8_t*)&Configid, sizeof(Configid));
            printf("r = %02X\n" , r);
            return 0;
        }   
        else
        {
            printf("[_NVCreateConfig] Error Config Already exist\n");
            return 2;    
        }    
    }
    return 1;
}

uint8_t *_GetHubId(void)
{
    if(Configid.stid == 0xA0)
        return Configid.dev_id;   
    else return 0;
}