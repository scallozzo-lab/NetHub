
#include "usart.h"
#include "srvcom.h"
#include "_ltproto.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static stSrvCom SrvCom = {0};
static uint8_t rxretry = 0;

uint32_t _GetNetIP(void)
{
    return SrvCom.netip;
}

uint8_t _GetSrvComStage(void)
{
    return SrvCom.stage;
}

uint16_t _GetSrvComTxStat(void)
{
    return SrvCom.txstat;
}

uint16_t _GetSrvComTxTOStat(void)
{
    return SrvCom.txtimeout;
}

uint16_t _GetSrvComRxStat(void)
{
    return SrvCom.rxstat;
}

uint32_t inet_aton(const char *ip)
{
    uint32_t result = 0;
    uint8_t part = 0;
    int shift = 24;

    while (*ip) {
        if (*ip == '.') {
            result |= ((uint32_t)part) << shift;
            shift -= 8;
            part = 0;
        } else {
            part = part * 10 + (*ip - '0');
        }
        ip++;
    }

    result |= part;   // último byte
    return result;
}


int _GetSrvComCHFree(void)
{
    if(SrvCom.netip && SrvCom.stage == SRVCOM_STG_IDLE) return 1;
    else return 0;
}

uint8_t _GetSrvComLinkRdy(void)
{
    return SrvCom.status & SRVCOM_STS_LINKRDY;
}    

int _TxServer(uint8_t *buff, uint16_t len)
{
    if(SrvCom.netip && SrvCom.stage == SRVCOM_STG_IDLE)
    {
        if(len <= sizeof(SrvCom.txsrvdata))
        {
            uint16_t crc = crc_ccitt(buff, len - 2);
            memcpy(SrvCom.txsrvdata, buff, len - 2);
            SrvCom.txsrvdata[len - 2] = crc>>8;
            SrvCom.txsrvdata[len - 1] = crc & 0xff;
            SrvCom.txsrvdatalen = len;
            SrvCom.status |= SRVCOM_STS_TXLOADED;
            return 0;
        }
        else
            return 2;

    }
    else return 1;
}


char *remove_spaces(char *str, uint16_t len) {
    static int8_t _out[2048];
    memset(_out, 0, sizeof(_out));
    char *read = str;
    char *write = (char*)_out;
    uint16_t idx = len;

    if(len <= (sizeof(_out) + 1) )
        while (*read && idx--) {
            if (!isspace((unsigned char)*read)) {
                *write++ = *read;
            }
            read++;
        }
    *write = '\0'; // terminate the new string
    
    return (char*)_out;
}


void _InitCom(void)
{
    _SIM7670_POWER_OFF;
    DelayUs(100000lu);
    _SIM7670_POWER_ON;
    DelayUs(1000000lu);
}

int _TxCom(uint8_t *buf, uint16_t len, uint8_t fwak)
{
#ifdef _USE_DEBUG_SRVCOM
    printf("[_TxCom] Tx Len %d [", len);
    for(int x= 0; x < len; x++) printf("%02X",buf[x]);
    printf("] %s\n", buf);
#endif

    memcpy(SrvCom.txlast, buf, len % sizeof(SrvCom.txlast));
    SrvCom.txlen = len;
    //return _IO_uartSAS_tx(buf, len, fwak);
    return USART1_tx(buf,len);

}

int _TxATCom(char *buf)
{
    uint8_t txbuf[256] = {0}, len = strlen((const char*)buf);
    memcpy(txbuf, buf, len % sizeof(txbuf));
    txbuf[len % sizeof(txbuf)] = '\r';
    _TxCom(txbuf, len + 1, 0);
    SrvCom.status |= SRVCOM_STS_ENABLERX;
    return len;
}

inline
uint8_t *_RxCom(uint16_t *rxlen, uint8_t intf)
{
    //return _IO_uartSAS_rx(rxlen, intf);
    return USART1_rx(rxlen, intf);

}

inline
void _ComIntrEnable(void)
{
    //_IO_uartSAS_Enable_RXInt();
    USART1_EnableRXInterrupt();
}

inline 
void _ComClearRxFlag(void)
{
    //_IO_uartSAS_Clear_RXFlag();

}

inline 
void _ComFlushRx(void)
{
    //_IO_uartSAS_FlushRx();
    USART1_FlushRx();
}

inline 
uint16_t _ComReadRxIdx(void)
{
    //return _IO_uartSAS_readRx();
    return USART1_ReadRx();
}

inline
uint8_t _ComGetRxOuttime(void)
{
    //return _IO_uartSAS_getRxTimeout();
}

void _InitSrvCom(uint8_t flag)
{
    if(flag)
        _InitCom();
    memset(&SrvCom, 0, sizeof(SrvCom));
    _ComFlushRx();
    rxretry = 3;
}

uint16_t memsearch(uint8_t *src, char *ss, uint16_t len)
{
    uint16_t memout = 0;
    
    while(*src)
    {
        memout++;
        if(!memcmp(src, ss, len)) return memout; 
        else src++;
    }

    return 0;
}

uint16_t CheckModemRx(uint8_t *rx, uint16_t len, uint8_t **rxcmd, uint8_t **rxdata)
{
    uint16_t uret = 0;
    uint8_t *prx = 0;
    uint16_t offset = 0;

    if(rx[0] == '\r' && rx[1] == '\n')
    {
        prx = (uint8_t*)remove_spaces((char*)rx, len);

#ifdef _USE_DEBUG_SRVCOM
        printf("Rx->");
        for(int x=0;x<strlen((const char*)prx);x++)
        {    
            if(prx[x] == 0x0D) printf("<CR>");
            else if(prx[x] == 0x0A) printf("<LF>");
            else printf("%c", prx[x]);
        }
#endif
        if(!memcmp(prx, _AT_READY_1, 10 ))
            uret |= SIMCOM_RESTART;
        if(!memcmp(prx, "+CPIN:READY",11) )
            uret |= SIMCOM_SIM_PRESENT;
        if(!memcmp(prx, "+CREG:",6) )
            uret |= SIMCOM_REG_STATUS;
        if((offset = memsearch(prx, "+NETOPEN:0",10)) )
              uret |= SIMCOM_NETOPENED;
        if((offset = memsearch(prx, "+CIPSEND:0,",11)) )
            uret |= SIMCOM_TXDONE;
        if(!memcmp(prx, "+IPERROR:Networkisalreadyopened",31) )
            uret |= SIMCOM_NETALREADYOPENED;
        if(!memcmp(prx, "+CIPOPEN:0,",11) )
            uret |= SIMCOM_NETUDPOPENED;
        if(!memcmp(prx, "+CIPCLOSE:0,",12) )
            uret |= SIMCOM_NETUDPCLOSED;
        if(!memcmp(prx, "+CGPADDR:",9) )
            uret |= SIMCOM_IPADDR;
        if((offset = memsearch(prx, "+CIPRXGET:",10)) )
            uret |= SIMCOM_RX_PKQ;
        if(!memcmp(prx, "OK",2))
            uret |= SIMCOM_ANSWER_OK;
        if((offset = memsearch(prx, ">",1)))
            uret |= SIMCOM_TXSYNCRO;
        if(!memcmp(prx, "+IPERROR:Nodata",15))
            uret |= SIMCOM_RX_NODATA;
    }
    else 
        printf("[CheckModemRx] Error No <CR><LF> [%02X-%02X]\n", rx[0], rx[1]);

    if(rxcmd && prx) 
    {
        *rxcmd = prx;
        *rxdata = rx + offset;
    }
    return uret;
}

uint8_t ProcRxBuf(uint8_t *prx, uint16_t rlen, uint8_t *destbuf, uint16_t *destlen)
{
    /*
    [_ProcSrvCom] Rx Len 160 [0D0A2B43495052584745543A20322C302C3132382C300D0A686F6C61203035000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000D0A0D0A4F4B0D0A]
    Rx->+CIPRXGET:2,0,128,0hola05
    rxcmd bits= 0800 (stg=0B)
    ******* received ok
    ***********rx OK
    */
   /*
    [_ProcSrvCom] Rx Len 178 [0D0A2B43495052584745543A20312C300D0A0D0A2B43495052584745543A20322C302C3132382C300D0A686F6C61203043000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000D0A0D0A4F4B0D0A]
    Rx->+CIPRXGET:1,0+CIPRXGET:2,0,128,0hola0C
*/
    #define _HEADER_LEN 3
    #define _CRC_LEN 2

    uint16_t rxpos = memsearch(prx, "+CIPRXGET: 2",12);
    int rxdatalen = 0;
    uint8_t *rxdata = 0; 
    uint8_t cantproc = 0;
    char tmptxt[32] = {0};

    if(rxpos)
    {
        rxpos--;
        memcpy(tmptxt, &prx[rxpos], sizeof(tmptxt));
        char *token = strtok(tmptxt, ",");
        
        if(token)
        {    
#ifdef _USE_DEBUG_SRVCOM
            printf("CIP %s\n", token); // +CIPRXGET:2
#endif        
        }
        token = strtok(NULL, ",");
        if(token)
        {    
#ifdef _USE_DEBUG_SRVCOM
            printf("Socket %s\n", token); // 0
#endif        
        }
        token = strtok(NULL, ",");
        if(token)
        {    
#ifdef _USE_DEBUG_SRVCOM
            printf("Rx Len %s\n", token); // 128
#endif            
            rxdatalen = atoi(token);
        }
        token = strtok(NULL, ",");
        if(token)
        {
#ifdef _USE_DEBUG_SRVCOM
            printf("Rx Restante %s\n", token); // data
#endif            
            uint8_t tmp = 32;
            // busca fin de línea (donde comienzan los datos binarios)
            while((prx[rxpos++] != 0x0A) && tmp) tmp--;
            rxdata = (uint8_t*)&prx[rxpos];
        }
        // Procesa el o los paquetes recibidos
        if(rxdata)
        {
            //uint16_t destoffset = 0; 
            while(rxdatalen)
            {
                if(*rxdata == _LT_FLAG)
                {
                    uint16_t len = (rxdata[2] << 8) | rxdata[1];

                    if(len <= _MAXSRVRXBUFFER)
                    {    
                        uint16_t rxcrc = rxdata[len - _CRC_LEN] | (rxdata[len - _CRC_LEN + 1]<<8);
                        uint16_t crc = crc_ccitt(rxdata, len - _CRC_LEN);
                        if(crc == rxcrc)
                        {    
                            memcpy(&destbuf[cantproc*_MAXSRVRXBUFFER], rxdata, len);
                            *destlen++ = len;
                            rxdata += len;
                            rxdatalen -= len;
                            cantproc++;
 #ifdef _USE_DEBUG_SRVCOM                           
                            printf("procesado %d\n", cantproc);
 #endif                       
                            if(cantproc >= _CANTMAXSLOTS)
                            {
                                printf("[ProcRxBuf] Maximo Paquetes Alcanzado %X\n", _CANTMAXSLOTS);
                                break;
                            }
                        }
                        else
                        {
                            printf("[ProcRxBuf] Error CRC rxcrc:%04X - crc:%04X\n", rxcrc, crc);  
                            break;
                        }
                    }
                    else
                    {    
                        printf("[ProcRxBuf] Error Buffer excede max.(%d)>(%d)\n", len, _MAXSRVRXBUFFER);
                        break;
                    }
                }
                else 
                {    
                    printf("[ProcRxBuf] Error no es una paquete LTProto %02X\n", *rxdata);    
                    break;
                }
            }
        }
        else 
            printf("[ProcRxBuf] Error Decode (rxdata = 0)\n");
    }
        
    // Devuelve la cantidad de paquetes procesados
    return cantproc;
}

void _ProcSrvCom(void)
{
#ifdef _USE_DEBUG_TXRX
    static uint16_t debug_timer = 0; 
#endif
    static uint8_t localbuf[_CANTMAXSLOTS][_MAXSRVRXBUFFER];
    uint16_t localbuflen[_CANTMAXSLOTS];
    uint16_t rxcmd = 0;
    static uint16_t rxtimeout = 0;
    static uint16_t timeuptimer = 0, timerrx = 0;
    uint8_t *prxcmd_data = 0, *prx_pos = 0;
    uint16_t rxdatalen = 0, rxlen = 0;
    static gralrx = 0;

#ifdef _USE_DEBUG_TXRX
    debug_timer++;
#endif

    if(SrvCom.status & SRVCOM_STS_ENABLERX)
    {    
        uint8_t *prxbuf = 0;

        if((SrvCom.status & SRVCOM_STS_RXCOMPLETE))
        {
            uint8_t rxflags = (SrvCom.status & SRVCOM_STS_RXCOMPLETE)? _COMINTCLEARFIFO : _COMINTDISABLE; 
            // Recibe paquete serie:
            prxbuf = _RxCom(&rxlen, rxflags);
        }
        // Si recibió algo, se copia a buffer externo y se vuelven a habilitar las interrupciones: 
        if(prxbuf && (SrvCom.status & SRVCOM_STS_RXCOMPLETE)) 
        {
            rxtimeout = _MAXTIMEOUTRX;
            _ComClearRxFlag();
            SrvCom.status &= ~(SRVCOM_STS_ENABLERX + SRVCOM_STS_RXCOMPLETE);
#ifdef _USE_DEBUG_SRVCOM
            printf("[_ProcSrvCom] Rx Len %d [", rxlen);
            for(int x= 0; x < rxlen; x++) printf("%02X",prxbuf[x]);
            printf("]\n");
#endif
            // Aca debe analizar las respuestas y cargar en caso de rx UDP el buffer lbuf
#ifdef _USE_SIMCOM_NOECHO
            if(rxlen)
#else
            if(rxlen >= SrvCom.txlen)
#endif            
            {
                uint16_t prxoffset = 0;
                uint16_t rxprecmd = memsearch(prxbuf + prxoffset, (char*)SrvCom.txlast, SrvCom.txlen);
#ifdef _USE_SIMCOM_NOECHO
                rxprecmd = 1;
#endif
                // Sino si no recibió nada, procesa otras respuestas
                if(rxprecmd)
                {
                    rxprecmd -= 1;
#ifdef _USE_SIMCOM_NOECHO
                    rxcmd = CheckModemRx(prxbuf, rxlen, &prxcmd_data, &prx_pos);
#else
                    rxcmd = CheckModemRx(prxbuf + rxprecmd + SrvCom.txlen, rxlen - SrvCom.txlen, &prxcmd_data);
#endif
#ifdef _USE_DEBUG_SRVCOM
                    if(rxcmd) printf("\nrxcmd bits= %04X (stg=%02X)\n", rxcmd, SrvCom.stage);
                    else printf("\nrxcmd = UNKNOWN\n");
#endif
                    // Restart detectado, fuerza inicialización:
                    if(rxcmd & SIMCOM_RESTART)
                    {
                        printf("[_ProcSrvCom] Restart Detectado! Inicializar...\n");
                        _ComFlushRx();
                        _InitSrvCom(0);
                    }
                    else if(rxcmd)
                    {
                        SrvCom.status |= SRVCOM_STS_RXRDY;     
                        rxdatalen = rxlen - SrvCom.txlen;
                        rxretry = 10;
                    }
                }
                // Sólo chequea coherencia si está inicializando:
                else if(!(SrvCom.status & SRVCOM_STS_RDYTORECEIVE)) 
                {
                    printf("[_ProcSrvCom] Rx ERROR! MISMATCH Len %d - %s\n", rxlen, prxbuf);
                    _ComFlushRx();
                    _InitSrvCom(0);
                }
                else
                {
                    printf("[_ProcSrvCom] Warning Rx ERROR! MISMATCH Len %d - %s\n", rxlen, prxbuf);
                }
            }
            else
            { 
                printf("[_ProcSrvCom] Rx ERROR! Len %d\n", rxlen);
                _ComFlushRx();
                _InitSrvCom(0);
            }
            _ComIntrEnable();
        }
        // Sino si al menos recibió algo, comienza timer 
        else if(_ComReadRxIdx())
        {
            uint16_t rxcant = _ComReadRxIdx();
            static uint16_t rx = 0;
            
            if(rxcant != rx) rxtimeout = _MAXTIMEOUTRX;
            else if(rxtimeout) rxtimeout--;
            else
                SrvCom.status |= SRVCOM_STS_RXCOMPLETE;
            rx =  rxcant;
        }
    }
    else
        rxtimeout = _MAXTIMEOUTRX;
    
    switch(SrvCom.stage)
    {
        case SRVCOM_STG_POWERUP:
#ifdef _USE_DEBUG_SRVCOM
            printf("[_ProcSrvCom] Powering Up SIMCOM 7670...\n");
#endif
        timeuptimer = _MAXTIMEUPTIMER;
        SrvCom.stage++;
        break;

        case SRVCOM_STG_POWERUP_W:
        if(timeuptimer) timeuptimer--;
        else 
        {    
            SrvCom.stage = SRVCOM_STG_INIT; 
        }
        break;

        case SRVCOM_STG_INIT:
        if(SrvCom.status & SRVCOM_STS_RXRDY)
        {
            SrvCom.status &= ~SRVCOM_STS_RXRDY;
     
#ifdef _USE_DEBUG_SRVCOM
            printf("[_ProcSrvCom] CommInit OK\n");
#endif
            //SrvCom.stage = SRVCOM_STG_DISABLE_AUTONOT;
            SrvCom.stage = SRVCOM_STG_CONFIG;
       
        }
        else if(--rxretry == 0) 
        {
            _TxATCom(_AT_);
            rxretry = _TIMERETRYAT;
        }
        break;

        case SRVCOM_STG_DISABLE_AUTONOT:
        if(SrvCom.status & SRVCOM_STS_RXRDY)
        {
            SrvCom.status &= ~SRVCOM_STS_RXRDY;
            SrvCom.stage = SRVCOM_STG_CONFIG;
        }
        else if(--rxretry == 0) 
        {
            _TxATCom(_AT_DISABLE_URC);
            rxretry = _TIMERETRYAT;
        }
        break;

        case SRVCOM_STG_CONFIG:

        if(rxcmd & SIMCOM_SIM_PRESENT)
        {
            SrvCom.status &= ~SRVCOM_STS_RXRDY;     
            SrvCom.stage++;
        }        
        else if(--rxretry == 0) _TxATCom(_AT_CPIN);
        break;

        case SRVCOM_STG_SETAPN:

        if(rxcmd & SIMCOM_ANSWER_OK)
        {
            SrvCom.status &= ~SRVCOM_STS_RXRDY;     
            timerrx = 1;
            SrvCom.stage++;
        }        
        else if(--rxretry == 0) _TxATCom(_AT_SETAPN);
        break;

        case SRVCOM_STG_CHECKREG:
        
        if(rxcmd & SIMCOM_REG_STATUS)
        {
            uint8_t constatus = (prxcmd_data)? prxcmd_data[8] : '?';
            SrvCom.status &= ~SRVCOM_STS_RXRDY;     
        
            if(constatus == '1' || constatus == '5')
            {    
                printf("CONECTADO Modo-> %c\n", constatus);
                timerrx = 0;
                rxretry = _TIMECHECKREG;
                SrvCom.stage++;
            }
            else if(gralrx++ >= _CANTMAX_CONN_RETRY)
            {
                printf("MODEM RESTART (_CANTMAX_CONN_RETRY)\n");
                gralrx = 0;
                _InitSrvCom(0);
            }
        }        
        else if(--timerrx == 0) 
        {
            timerrx = _TIMEPOLLINGCONN;
            _TxATCom(_AT_CREG);
        }
        break;

        case SRVCOM_STG_CONNECT:

        if(rxcmd & SIMCOM_ANSWER_OK)
        {
            SrvCom.status &= ~SRVCOM_STS_RXRDY;     
            SrvCom.stage = SRVCOM_STG_INITNET;
            timerrx = 0;
        }        
        else if(timerrx)
        {
            if(--timerrx == 0)
                _InitSrvCom(0);
            else if(timerrx == (_TIMETOUTCONNECT / 2))
                SrvCom.status |= SRVCOM_STS_ENABLERX;
        }
        else if(--rxretry == 0)
        {    
            _TxATCom(_AT_CONNECT);
            timerrx = _TIMETOUTCONNECT;
            rxtimeout = _TIMERXTOUTCONNECT;
        }
        break;

        case SRVCOM_STG_INITNET:
        if(rxcmd & SIMCOM_NETOPENED || rxcmd & SIMCOM_NETALREADYOPENED)
        {
            SrvCom.status &= ~SRVCOM_STS_RXRDY;     
            SrvCom.status |= SRVCOM_STS_NETRDY;
            timerrx = 0;
            SrvCom.stage = SRVCOM_STG_CONFSOCKET;
        }        
        else if(timerrx)
        {
            if(--timerrx == 0)
                _InitSrvCom(0);
        }
        else 
        {    
            timerrx = _TIMEOUTNETOPEN;
            _TxATCom(_AT_NETOPEN);
            rxtimeout = _TIMERXTOUTCONNECT;
        }
        break;

        case SRVCOM_STG_CONFSOCKET:
        
        if(rxcmd & SIMCOM_NETUDPOPENED)
        {
            SrvCom.status &= ~SRVCOM_STS_RXRDY;     
            timerrx = 0;
            
            uint8_t stat = prxcmd_data[11] - 0x30;
            printf("[_ProcSrvCom] SIMCOM_NETUDPOPENED stat:%d\n",stat);
            
            if(stat)
            {
                printf("[_ProcSrvCom] NETOPEN ERR %d\n",stat);
                if(stat == 4)
                    SrvCom.stage = SRVCOM_STG_CLOSENET;
            }
            else
            {    
                SrvCom.status |= SRVCOM_STS_RDYTORECEIVE;
                SrvCom.stage = SRVCOM_STG_SETMODE;
            }
        }        
        else if(timerrx)
        {
            if(--timerrx == 0)
                _InitSrvCom(0);
        }
        else 
        {    
            timerrx = _TIMEOUTCONFSOCKET;
            _TxATCom(_AT_UDPOPEN);
            rxtimeout = _TIMERXTOUTCONNECT;
        }
        break;

        case SRVCOM_STG_SETMODE:
        
        if(rxcmd & SIMCOM_ANSWER_OK)
        {
            timerrx = 0;
            SrvCom.stage++;
            // Listo para enviar y recibir
            SrvCom.status |= SRVCOM_STS_LINKRDY;
        }        
        else if(--rxretry == 0) _TxATCom(_AT_NOAUTORX);
        break;

        case SRVCOM_STG_IDLE:
        {
            static uint16_t exttimer = 0;
            static uint8_t rxpolling = 0;

            if(rxcmd & SIMCOM_RX_NODATA)
            {
#ifdef _USE_DEBUG_SRVCOM
                printf("[_ProcSrvCom] NO Data Rx.\n");
#endif                
                timerrx = 0;
                SrvCom.status &= ~SRVCOM_STS_RXRDY;
                SrvCom.status &= ~SRVCOM_STS_RXREQ_PENDING;          
            }
            // Sino es RX
            else if(rxcmd & SIMCOM_RX_PKQ)
            {
#ifdef _USE_DEBUG_SRVCOM
                printf("[_ProcSrvCom] RX OK...\n");
#endif                
                timerrx = 0;
                gralrx = 0;
                SrvCom.status &= ~SRVCOM_STS_RXRDY;
                SrvCom.status &= ~SRVCOM_STS_RXREQ_PENDING;          
                
                for(int x=0;x<_CANTMAXSLOTS;x++)
                    localbuflen[x] = 0;
                uint8_t cantproc = ProcRxBuf(prx_pos,rxlen, (uint8_t*)localbuf, localbuflen);

#ifdef _USE_DEBUG_TXRX          
                printf("[_ProcSrvCom] t(%d) Cantidad Rx procesados (%d)- Uart OverRun (%d)\n", debug_timer ,cantproc, USART1_GetOVR());
               
                for(uint8_t x=0;x<cantproc;x++)
                {
                    printf("B len(%d) N = (%d) = [",localbuflen[x] ,x);
                    
                    for(int x2=0;x2<localbuflen[x];x2++)
                        printf("%02X",localbuf[x][x2]);
                    SrvCom.rxstat++;
                    printf("]\n");
                }
#endif               
                _ProcRxLT((uint8_t*)localbuf, localbuflen);
            }
            // Si existe paquete para transmitir...
            else if(SrvCom.status & SRVCOM_STS_TXLOADED && !(SrvCom.status & SRVCOM_STS_RXREQ_PENDING) )
            {
                uint8_t tmpstr[64] = {0}; 
                sprintf((char*)tmpstr, "AT+CIPSEND=0,%d,\"186.123.27.7\",%d", SrvCom.txsrvdatalen, 5000);
                _TxATCom((char*)tmpstr);
                SrvCom.status &= ~SRVCOM_STS_TXLOADED;
                SrvCom.stage = SRVCOM_STG_TRANSMIT;
                timerrx = _TIMERRXSYNCRO;
            }
            // Sino continúa pedidos de polling
            else if(rxcmd & SIMCOM_IPADDR)
            {
                SrvCom.status &= ~(SRVCOM_STS_RXRDY);     
                timerrx = 0;
                
                if(SrvCom.netip == 0)
                {
                    int8_t aip[16] = {0};
                    for(int x = 0; x < rxdatalen; x++)
                    {    
                        if(prxcmd_data[11+x] != 'O') aip[x] = prxcmd_data[11+x];
                        else break;
                    }
                    //if(inet_aton((const char*)aip, &SrvCom.netip))
                    //if(inet_aton((const char*)aip, &SrvCom.netip))
                    SrvCom.netip = inet_aton((const char*)aip);
                    if(SrvCom.netip)
                    {
                        printf("[_ProcSrvCom] NewIp (%s) = %04x\n", aip, SrvCom.netip);
                    }
                    else
                        printf("[_ProcSrvCom] Ip: ERROR %s\n", aip);
                } 
            }        
            else if(exttimer) exttimer--;
            else 
            {        
                timerrx = 600;
                
                if(rxpolling++ & 1)
                {    
                    if(SrvCom.netip == 0)
                        _TxATCom(_AT_CHECKIP);
                    else 
                    { 
                        _TxATCom(_AT_GETRXSTATUS);        
                        SrvCom.status |= SRVCOM_STS_RXREQ_PENDING;
                    }
                }
                else 
                {
                    _TxATCom(_AT_GETRXSTATUS);        
                    SrvCom.status |= SRVCOM_STS_RXREQ_PENDING;
                }
                exttimer = _TIMEPOLLINGIDLE;
                rxtimeout = _TIMETOUTIDLE;
            }
        }
        break;

        case SRVCOM_STG_TRANSMIT:
        if((rxcmd & SIMCOM_TXSYNCRO) || !timerrx)
        {
            if(timerrx == 0) 
            {
                if(gralrx++ >= _CANTMAX_TXSRV_RETRY_TO)
                {
                    printf("MODEM RESTART Timeout...\n");
                    gralrx = 0;
                    _InitSrvCom(0);
                }
                else 
                    printf("TX SYNCRO timeout\n");
            }
            else
            {
                if(gralrx++ >= _CANTMAX_TXSRV_RETRY)
                {    
                    printf("MODEM RESTART (_CANTMAX_TXSRV_RETRY)...\n");
                    gralrx = 0;
                    _InitSrvCom(0);
                }
            }
            SrvCom.status |= SRVCOM_STS_ENABLERX;
            rxtimeout = _TIMERXTOUTSYNCRO;
            timerrx = _TIMERRXTXDONE;
            _TxCom(SrvCom.txsrvdata, SrvCom.txsrvdatalen, 0);

#ifdef _USE_DEBUG_TXRX
    #ifdef _USE_DEBUG_TX_SRVCOM            
            printf("[_ProcSrvCom] t(%d) Tx Len (%d) [", debug_timer, SrvCom.txsrvdatalen);          
            for(int x = 0;x < SrvCom.txsrvdatalen; x++) printf("%02X", SrvCom.txsrvdata[x]);
            printf("]\n");
    #endif
#endif
        }
        else if((rxcmd & SIMCOM_TXDONE) || (rxcmd & SIMCOM_ANSWER_OK) || !timerrx)
        {
            if(timerrx == 0) 
            {
                SrvCom.txtimeout++;
                printf("TX Done timeout\n");
            }
            else 
            {
                printf("TX DONE!\n");
                SrvCom.txstat++;
            }
            SrvCom.stage = SRVCOM_STG_IDLE;   
        }
        if(timerrx) timerrx--;
        break;

        case SRVCOM_STG_RECEIVE:
            SrvCom.stage = SRVCOM_STG_IDLE;   
        
        break;

        case SRVCOM_STG_CLOSENET:
        if(rxcmd & SIMCOM_NETUDPCLOSED)
        {
            SrvCom.status &= ~SRVCOM_STS_RXRDY;     
            timerrx = 0; 
            SrvCom.stage = SRVCOM_STG_CONFSOCKET;
        }        
        else if(timerrx)
        {
            if(--timerrx == 0)
                _InitSrvCom(0);
        }
        else 
        {    
            timerrx = _TIMETOUTCLOSE;
            _TxATCom(_AT_UDPCLOSE);
            rxtimeout = _TIMERXTOUTCONNECT;
        }
        break;


        case SRVCOM_STG_RX_LOOP:
        
        //if(exttimer) exttimer--;
        //else
        {


        }

        break;


        default:
        break;
    }

    // Restaura reintentos para los cmd que lo usan...
    if(rxretry == 0) rxretry = _TIMECOMMONRETRY;
}
