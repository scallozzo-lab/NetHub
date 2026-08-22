#ifndef __SRVCOM_H__
    #define __SRVCOM_H__

#include "main.h"
#include "pwm.h"

#define _SIM7670_POWER_ON  _SetPWM_CH1(0lu)
#define _SIM7670_POWER_OFF _SetPWM_CH1(0xfffffffflu)

           

#define _LISTENPORT         5000
#define _SRVCOMUARTBAUD     115200
#define _MAXSRVRXBUFFER     128
#define _CANTMAXSLOTS       8
#define _MAXSRVTXBUFFER     1500

#define _TIMEBASE 10

#define _MAXTIMEOUTRX       (3 * _TIMEBASE)
#define _MAXTIMEUPTIMER     (300 * _TIMEBASE)
#define _TIMEPOLLINGCONN    (300 * _TIMEBASE)
#define _TIMERETRYAT        (500 * _TIMEBASE)
#define _TIMECHECKREG       (10 * _TIMEBASE)
#define _TIMERXTOUTCONNECT  (250 * _TIMEBASE)
#define _TIMETOUTCONNECT    (800 * _TIMEBASE)
#define _TIMEOUTNETOPEN     (1000 * _TIMEBASE)
#define _TIMEOUTCONFSOCKET  (300 * _TIMEBASE)
#define _TIMETOUTIDLE       (200 * _TIMEBASE)
#define _TIMEPOLLINGIDLE    (10 * _TIMEBASE)
#define _TIMETOUTCLOSE      (300 * _TIMEBASE)
#define _TIMECOMMONRETRY    (15 * _TIMEBASE)
#define _TIMERXTOUTSYNCRO   (10 * _TIMEBASE)
#define _TIMERRXSYNCRO      (60 * _TIMEBASE)
#define _TIMERRXTXDONE      (100 * _TIMEBASE)
#define _CANTMAX_TXSRV_RETRY_TO 10
#define _CANTMAX_TXSRV_RETRY    20
#define _CANTMAX_CONN_RETRY     30

//#define _COMINTDISABLE          0
//#define _COMINTCLEARFIFO        2


#define _AT_READY_1     "*ATREADY:1"                                    // UCC de inicio
#define _AT_CPIN        "AT+CPIN?"                                      // Rx-> +CPIN: READY
#define _AT_SETAPN      "AT+CGDCONT=1,\"IP\",\"igprs.claro.com.ar\""    // Rx-> OK
#define _AT_CONNECT     "AT+CGACT=1,1"
#define _AT_CREG        "AT+CREG?"
#define _AT_NETOPEN     "AT+NETOPEN"
#define _AT_UDPOPEN     "AT+CIPOPEN=0,\"UDP\",,,5000"
#define _AT_UDPCLOSE    "AT+CIPCLOSE=0"
#define _AT_CHECKIP     "AT+CGPADDR=1"
#define _AT_TRANSP_MODE "AT+CIPMODE=1"
#define _AT_NOAUTORX    "AT+CIPRXGET=1"
#define _AT_GETRXSTATUS "AT+CIPRXGET=2,0,128" // AT+CIPRXGET=3,0,720
//#define _AT_GETRXSTATUS "AT+CIPRXGET=2,0,1024" // AT+CIPRXGET=3,0,720
#define _AT_GETRXDATA   "AT+CIPRXGET=2,0,128"
#define _AT_DISABLE_URC "AT+CIURC=0"
#ifdef _USE_SIMCOM_NOECHO
    #define _AT_    "ATE0"
#else
    #define _AT_    "AT"
#endif


typedef struct srvcom
{
    uint8_t stage;
    uint8_t status;
    uint8_t txlast[_MAXSRVTXBUFFER];
    uint16_t txlen;
    uint8_t txsrvdata[_MAXSRVTXBUFFER];
    uint16_t txsrvdatalen;
    uint32_t netip;
    uint16_t rxstat;
    uint16_t txstat;
    uint16_t txtimeout;
}stSrvCom;


typedef enum
{
    SRVCOM_STG_POWERUP,
    SRVCOM_STG_POWERUP_W, 
    SRVCOM_STG_INIT,
    SRVCOM_STG_DISABLE_AUTONOT,
    SRVCOM_STG_CONFIG,
    SRVCOM_STG_SETAPN,
    SRVCOM_STG_CHECKREG,
    SRVCOM_STG_CONNECT,
    SRVCOM_STG_INITNET,
    SRVCOM_STG_CONFSOCKET,
    SRVCOM_STG_SETMODE,
    SRVCOM_STG_IDLE,
    SRVCOM_STG_TRANSMIT,
    SRVCOM_STG_RECEIVE,
    SRVCOM_STG_CLOSENET,
    SRVCOM_STG_RX_LOOP,
}eSrvComstages;

typedef enum
{
    SRVCOM_STS_ENABLERX         = BIT0,
    SRVCOM_STS_RXRDY            = BIT1,
    SRVCOM_STS_RXCOMPLETE       = BIT2,
    SRVCOM_STS_NETRDY           = BIT3,
    SRVCOM_STS_TXLOADED         = BIT4,
    SRVCOM_STS_LINKRDY          = BIT5,
    SRVCOM_STS_RDYTORECEIVE     = BIT6,
    SRVCOM_STS_RXREQ_PENDING    = BIT7
    
}eSrvComStatus;

typedef enum
{
    SIMCOM_ERROR_NO_RX      = BIT0,
    SIMCOM_ANSWER_OK        = BIT1,
    SIMCOM_SIM_PRESENT      = BIT2,
    SIMCOM_REG_STATUS       = BIT3,
    SIMCOM_NETOPENED        = BIT4,
    SIMCOM_NETALREADYOPENED = BIT5,
    SIMCOM_NETUDPOPENED     = BIT6,
    SIMCOM_NETUDPCLOSED     = BIT7,
    SIMCOM_IPADDR           = BIT8,
    SIMCOM_TXSYNCRO         = BIT9,
    SIMCOM_TXDONE           = BIT10,
    SIMCOM_RX_PKQ           = BIT11,
    SIMCOM_RX_NODATA        = BIT12,
    SIMCOM_RESTART          = BIT13,
    SIMCOM_res14            = BIT14,
    SIMCOM_res15            = BIT15
}eModemRxCmds;


void _InitSrvCom(uint8_t flag);
void _ProcSrvCom(void);
int _GetSrvComCHFree(void);
uint8_t _GetSrvComLinkRdy(void);
int _TxServer(uint8_t *buff, uint16_t len);
uint32_t _GetNetIP(void);
uint8_t _GetSrvComStage(void);
uint16_t _GetSrvComTxStat(void);
uint16_t _GetSrvComRxStat(void);
uint16_t _GetSrvComTxTOStat(void);



#endif
