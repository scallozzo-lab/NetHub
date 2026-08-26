#ifndef __LTPROTO_H__
    #define  __LTPROTO_H__

#include "main.h"
//#include "_IOfncs.h"
#include "srvcom.h"
#include "srtc.h"

//------------------------------------------------- LT - Config -----------------------------------------------------
#define _LTPROTO_TIMERALIVE         300
#define _LTX_TIMESLOT               130//(90)
#define _MAXFRAMEFWUPDATE           128 //1024        // Valor máximo de frame de fw update (el servicio puede elegir uno menor, siendo este el tope)
//------------------------------------------------- LT --------------------------------------------------------------


typedef enum
{
    //----------RC Cmds------------------------//
    RC_CMD_STATUS       = 0x01,
    RC_CMD_VIDEOFILE    = 0x02,
    //----------LT Cmds------------------------//
    LT_CMD_HUB_STATUS   = 0x20,
    LT_CMD_STATUS       = 0x21,
    LT_CMD_FW_FRAME     = 0x22,
    //----------LT Server side Cmds------------//
    LT_CMD_SERVERSIDE   = 0x40,  
    //----------RC Server side-----------------//
    RC_CMD_SERVERSIDE   = 0x80
}erccmds;


#define _LT_FLAG   0xA5


typedef struct
{
    uint8_t status;
    uint16_t timeralive;
    uint16_t seqId;
    uint8_t HubStatus;
    uint8_t HubId[6];           // HubId tomado de la configuración 
    uint8_t LTScan[_CANT_MAX_SLV];
    uint8_t LTCurrent;
    uint8_t LTDevAttached;
    uint8_t LTDevDisabled;
    uint16_t LTSlottime;
    uint16_t LTFrameid;
    uint8_t TxGroup;
}stLTProtocol;

typedef enum
{
    LT_STS_RDY                  = BIT0,
    LT_STS_RX_MODE              = BIT1,
    LT_STS_TX_BUSY              = BIT2,
    LT_STS_CONFIG_SLV_OK        = BIT3,
    LT_STS_SLV_SCAN_COMPLETE    = BIT4,
}eltstatus;

typedef enum
{
    SS_STS_ONLINE               = BIT0,
    SS_STS_ERR_NO_ALTA          = BIT1,
    SS_STS_ERR_EQ_DUPLICADO     = BIT2,
    SSTATUS_STS_res3            = BIT3,
    SSTATUS_STS_res4            = BIT4,
    SSTATUS_STS_res5            = BIT5,
    SSTATUS_STS_res6            = BIT6,
    SSTATUS_STS_FWUPDATE_ENABLE = BIT7    
}esstatus;

typedef enum
{
    HUB_STS_GNSS_RDY            = BIT0,
    HUB_STS_res1                = BIT1,
    HUB_STS_res2                = BIT2,
    HUB_STS_res3                = BIT3,
    HUB_STS_res4                = BIT4,
    HUB_STS_res5                = BIT5,
    HUB_STS_res6                = BIT6,
    HUB_STS_DMX_ENABLED         = BIT7
}ehubstatus_t;

typedef struct
{
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t day;
    uint8_t date;
    uint8_t month;
    uint8_t year;
}stRTC;


typedef struct __attribute__((packed))
{
    // st 2
    uint32_t CDaT;
    uint8_t free[3];
    uint8_t EqStatus; 
    uint8_t EqErrsts;
    uint8_t LowerVoltage;
    uint8_t PeakVoltage;
    uint8_t MainRestarts;
    uint32_t TimeCPURunning;
    uint16_t FwVersion;
}stRxLTDatast2;

typedef struct __attribute__((packed))
{
    uint8_t  currSTS;          // Estado actual 
    uint8_t  currV;            // Tensión actual
    uint16_t currI;            // Corriente actual + cospi
    uint8_t  TopV;             // Tensión máxima detectada
    uint8_t  LowerV;           // Tensión mínima detectada
    uint32_t timerunning;      // Tiempo de funcionamiento de las luminarias (Expresado en segundos)
    uint32_t t_01Wh;           // Para Tx 32bits (01Wh) (para convertir a Wh = 01Wh x 0.1)
    uint32_t r_01Wh;           // Para Tx 32bits (01Wh)
}stRxLTDatast1;


typedef struct _ltproto
{
    uint8_t status;
    stRxLTDatast1 RxLTDatast1;
    stRxLTDatast2 RxLTDatast2;
}stRxLTData;

typedef enum 
{
    LTDATA_STS_ST1OK    = BIT0,
    LTDATA_STS_ST2OK    = BIT1,
}estatusrxdata;

// Estructuras para hub-status
typedef struct __attribute__((packed))
{
    uint8_t flag;
    uint16_t len;
    uint8_t Cmd;
    uint16_t Seq;
    uint8_t HubID[6];
    uint8_t HubStatus;
    uint8_t HubErrsts;
    uint8_t HubEvent;   // <> 0 = Event
    uint32_t TimeRunning;
    
    int32_t latitude_e7;
    int32_t longitude_e7;
    rtc_soft_t rtc;
    
    uint16_t FwVersion;
    uint16_t Crc;
}stTxLTHubStatus;

// Estructura de respuesta para HubStatus (podría devolver configuración?)
typedef struct __attribute__((packed))
{
    uint8_t flag;
    uint16_t len;
    uint8_t Cmd;
    uint32_t Seq;
    uint8_t SStatus;
    uint8_t SRequest;
    uint8_t SectorID;
    uint8_t DevAttached;
    uint8_t DevDisabled;
    uint8_t DevbitList[13];             // Device listing expresado en bits bit0 = Dev1, bit1 = Dev2, etc.
    uint8_t HubVer[3];
    uint8_t LTVer[3];
    
    uint8_t TxConfig;                   // Tiempo expresado en segundos para la transmisión de hubstatus
    rtc_soft_t rtc;                     // RTC propuesto
    
    uint16_t Crc;
}stRxLTHubStatus;

typedef struct __attribute__((packed))
{
    stRxLTDatast1 LTDatast1;    //(18)
    stRxLTDatast2 LTDatast2;    //(18)
}stGroupStatus;

typedef struct __attribute__((packed))
{
    uint8_t flag;
    uint16_t len;
    uint8_t Cmd;
    uint16_t Seq;
    uint8_t HubID[6];
    uint8_t Group;
    uint8_t ListEqs;    // Cantidad de equipos listados 
/*-----------------Estructura de Equipos -----------------------------------*/
//stGroupStatus GroupStatus[25];
/*--------------------------------------------------------------------------*/    
//    uint16_t Crc;
}stTxLTStatusG;

typedef struct __attribute__((packed))
{
    // terminar!!!
    uint8_t flag;
    uint16_t len;
    uint8_t Cmd;
    uint32_t Seq;
    uint8_t SStatus;
    uint8_t SRequest;
    uint16_t Crc;
}stTxLTStatus;

typedef struct __attribute__((packed))
{
    uint8_t flag;
    uint16_t len;
    uint8_t Cmd;
    uint8_t HubID[6];
    uint16_t framenr;
    uint8_t ver[3];
    uint16_t Crc;    
}stTxLTFwFrame;

typedef struct __attribute__((packed))
{
    uint8_t flag;
    uint16_t len;
    uint8_t Cmd;
    uint8_t HubID[6];
    uint8_t ver[3];
    uint16_t framesize;
    uint16_t framenr;
    uint16_t totframes;
    uint8_t buffer[_MAXFRAMEFWUPDATE];
    uint16_t Crc;    
}stRxLTFwFrame;


/*-------------------------------- Structuras Luma32 ----------------------------------------------------------------*/
#define _LADDR0 0xDE
#define _LADDR1 0xAD
#define _LADDR2 0xBE
#define _LADDR3 0xEF
#define _LADDR4 0x01

#define _MADDR0 0xDE
#define _MADDR1 0xAD
#define _MADDR2 0xBE
#define _MADDR3 0xEF
#define _MADDR4 0x01

#define _MCAST0 0xf0

#define _BCAST0 0xaf
#define _BCAST1 0xff
#define _BCAST2 0xff
#define _BCAST3 0xff
#define _BCAST4 0xf0

#define _LUMACRCSEED   0xCABA

typedef struct __attribute__((packed))
{
    uint8_t DAd[5];
    uint8_t RAd[3];
    uint8_t CSts;               // 4H = Cmd, 4L = status;
    uint8_t frameidh;           // (Para WRITE_FW & READ_FW) frameidh + frameidl = offset binario / otros cmds: frameidh = net hops 
    uint8_t frameidl;           
    uint8_t rid;                // Remote id: usado por el nodo intermediario / Modo multicast : Indica el grupo al que pertenece el mensaje. 
    uint8_t data[18];
    uint16_t crc;
}stIOLuma;


typedef enum
{
    /* ------ 4 bits cmds -----------*/
    LUMACMD_STS             = 0x00,
    LUMACMD_PROPAGATE       = 0x01,
    LUMACMD_SYNCRO          = 0x02,    
    LUMACMD_STS2            = 0x03,
    LUMACMD_res4            = 0x04,
    LUMACMD_res5            = 0x05,
    LUMACMD_res6            = 0x06,
    LUMACMD_res7            = 0x07,
    LUMACMD_res8            = 0x08,
    LUMACMD_res9            = 0x09,
    LUMACMD_res10           = 0x0A,
    LUMACMD_res11           = 0x0B,
    LUMACMD_READ_FW         = 0x0C,
    LUMACMD_WRITE_FW        = 0x0D,
    LUMACMD_INFO            = 0x0E,         // Broadcast
    LUMACMD_UNCONFIG_DEV    = 0x0F          // Broadcast sin dirección de red, solo Serial interno.
}elcmds;

typedef enum 
{
    /* --------- 4 bits cmds request -----------*/
    LUMA_MDSTS_REPEAT_REQ       = BIT4,
    LUMA_MDSTS_ACK_REQ          = BIT5,
    LUMA_MDSTS_MESH_REQ         = BIT6,
    LUMA_MDSTS_USEMCGROUP_REQ   = BIT7          // Con este bit en modo multi-cast utiliza Rid para determinar el grupo al que va dirigido el mensaje.
}emdsts;

typedef enum 
{
    LUMA_DMSTS_REPEAT_REQ           = BIT4,
    LUMA_DMSTS_ACK_REQ              = BIT5,
    LUMA_DMSTS_MESH_REQ             = BIT6,
    LUMA_DMSTS_UNCONFIG_DEV_AROUND  = BIT7
}edmsts;



void _ProcLTProto(void);
void _ProcRxLT(uint8_t *xbuff, uint16_t *len);
void _InitLTProtocol(void);
void _SetHubStatus(uint8_t sts);
void _ResetHubStatus(uint8_t sts);
uint8_t _GetHubStatus(void);



#endif