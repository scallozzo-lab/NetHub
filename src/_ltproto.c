#include "_ltproto.h"
#include "_sh1106.h"
#include "sx1278.h"
#include "fw_version.h"
#include "fwupdate.h"
#include <string.h>

stLTProtocol LTProtocol = {0};
//stRxLTDatast1 RxLTDatast1[_CANT_MAX_SLV];
//stRxLTDatast2 RxLTDatast2[_CANT_MAX_SLV];
stRxLTData RxLTData[_CANT_MAX_SLV];

uint8_t _GetEnabledLTX(uint8_t s)
{
    return (s < _CANT_MAX_SLV)? LTProtocol.LTScan[s] : 0;   
}

void ProcLCD(uint8_t flag)
{
    
    extern const uint8_t gear32x32[];
    extern const uint8_t altiva64x32[];


    // cada 500ms
    if(flag)
    {

    }
    // Sino procesa
    else
    {       
        // lcd 1
        _SelLCD1();
        // printf ip
        SH1106_Printf(_FONT_8X8, 0,0, "CONN = %02X", _GetSrvComLinkRdy());
        SH1106_Printf(_FONT_8X8, 1,0, "SRV Stg = %02X", _GetSrvComStage());
        SH1106_Printf(_FONT_8X8, 2,0, "Tx/Rx %d,%d", _GetSrvComTxStat(), _GetSrvComRxStat());
        SH1106_Printf(_FONT_8X8, 3,0, "Tx TO %d", _GetSrvComTxTOStat());
        
        SH1106_Printf(_FONT_5X7, 6,0, "Hub %04X%04X", _GetUniqueID(0), _GetUniqueID(1));
        SH1106_Printf(_FONT_8X8, 7,0, "IP%d.%d.%d.%d", 
                                            _GetNetIP()>>24 & 0xff,
                                            (_GetNetIP()>>16) & 0xff,
                                            (_GetNetIP()>>8) & 0xff,
                                            _GetNetIP() & 0xff);
    
        _SelLCD2();
        SH1106_Printf(_FONT_8X8, 0,0, "NETHUB V%c.%c.%c", FW_VERSION_0 +0x30, FW_VERSION_1+0x30, FW_VERSION_REV);
        if(_GetFwUpdateReqFrame())
        {          
            //SH1106_Printf(_FONT_8X8, 7,0, "Actualizar %%d  ", (_GetFwUpdateReqFrame() / _GetFrameTotFrame()) * 100);
        
            int percent = (_GetFwUpdateReqFrame() * 100) / _GetFrameTotFrame();

            SH1106_Printf(_FONT_8X8, 7, 0,
                "Actualizar %d%%  ",
                percent);
        }
        /*
        SH1106_DrawBitmap(32, 10, 
                       gear32x32,
                       32, 32);
        */
    }
}

void _InitLTProtocol(void)
{
    memset(&LTProtocol, 0, sizeof(LTProtocol));
    //memset(RxLTDatast1, 0, sizeof(RxLTDatast1));
    //memset(RxLTDatast2, 0, sizeof(RxLTDatast2));
    memset(RxLTData, 0, sizeof(RxLTData));
    
    if(_GetHubId())
        memcpy(LTProtocol.HubId, _GetHubId(), sizeof(LTProtocol.HubId));

#ifdef _USE_DUMMY_TEST_NOLTSERVICE
    LTProtocol.LTScan[0] = 1;
    LTProtocol.LTScan[10] = 10;
    LTProtocol.LTScan[25] = 90;
#endif    

    ProcLCD(1);
}

uint8_t *_GetHubID(void)
{
    static const uint8_t hubid[] = {0,0,0,0,0,0};
    return (uint8_t*)hubid;
}

void _ProcRxLT(uint8_t *xbuff, uint16_t *len)
{   
    for(int idx = 0; idx < _CANTMAXSLOTS; idx++)
    {
        if(len[idx])
        {
            uint8_t *rxbuff = xbuff + (idx * _MAXSRVRXBUFFER);
            
            // Sel cmd
            switch (rxbuff[3] & 0x7f)
            {
                case LT_CMD_HUB_STATUS:
                {
                    stRxLTHubStatus *RxLTHubStatus = (stRxLTHubStatus *)rxbuff;
                    if(LTProtocol.seqId == RxLTHubStatus->Seq)
                    {
#ifdef _USE_DEBUG_TXRX
                        printf("[_ProcRxLT] Rx Cmd HUBStatus...\n");
                        printf("flag = %02X\n", RxLTHubStatus->flag);
                        printf("len = (%d)\n", RxLTHubStatus->len);
                        printf("cmd = %02X\n", RxLTHubStatus->Cmd);
                        printf("Seq = %02X\n", RxLTHubStatus->Seq);
                        printf("SStatus = %02X\n", RxLTHubStatus->SStatus);
                        printf("SRequest = %02X\n", RxLTHubStatus->SRequest);
                        printf("SectorID = (%d)\n", RxLTHubStatus->SectorID);
                        printf("DevAttached = (%d)\n", RxLTHubStatus->DevAttached);
                        printf("DevDisabled = (%d)\n", RxLTHubStatus->DevDisabled);
                        printf("DevbitList->[");
                        for(int i=0;i<sizeof(RxLTHubStatus->DevbitList);i++) 
                            printf("%02X",RxLTHubStatus->DevbitList[i]);
                        printf("]\n");
                        printf("HubVer = %02X.%02X.%02X\n", 
                                                            RxLTHubStatus->HubVer[0],
                                                            RxLTHubStatus->HubVer[1],
                                                            RxLTHubStatus->HubVer[2]);
                        printf("LTVer = %02X.%02X.%02X\n", 
                                                            RxLTHubStatus->LTVer[0],
                                                            RxLTHubStatus->LTVer[1],
                                                            RxLTHubStatus->LTVer[2]);
#endif    
                        // Si la versión informada por el servidor es diferente a la actual dispara le proceso para actualizar EEPROM x frame
                        if((RxLTHubStatus->HubVer[0] != (FW_VERSION_0 + 0x30)) || (RxLTHubStatus->HubVer[1] != (FW_VERSION_1 + 0x30) ))
                        {    
                            // Si está habilitado desde servidor actualizar FW:
                            if(RxLTHubStatus->SStatus & SSTATUS_STS_FWUPDATE_ENABLE)
                            {
                                // Si no está procesando, dispara pedido:
                                if(_GetFwUpdateStatus() == 0)
                                {    
#ifdef _USE_DEBUG_FWUPDATE
                                printf("[_ProcRxLT] Fwupdate Iniciado Nueva Ver:%02X.%02X\n", RxLTHubStatus->HubVer[0], RxLTHubStatus->HubVer[1]);
#endif
                                    _SetFwReqVersion(RxLTHubStatus->HubVer);
                                    // Comienza el pedido de forma inmediata:
                                    _SetFwUpdateTxTimer(0);
                                }
                                else printf("en proceso\n");
                            }
#ifdef _USE_DEBUG_FWUPDATE
                            else printf("[_ProcRxLT] Fwupdate (DESHABILITADO DESDE SRV) Ver:%02X.%02X\n", RxLTHubStatus->HubVer[0], RxLTHubStatus->HubVer[1]);
#endif
                        }
                        // Nueva secuencia
                        LTProtocol.seqId++;
                    }
                    // Si cambió la cantidad de dispositivos habilitados, procesa la nueva lista:
                    if(LTProtocol.LTDevAttached != RxLTHubStatus->DevAttached || LTProtocol.LTDevDisabled != RxLTHubStatus->DevDisabled)
                    {
                        for(int idx = 0;idx < _CANT_MAX_SLV; idx++)
                        {    
                            if(_GetBit((const uint8_t *)RxLTHubStatus->DevbitList,idx))
                                LTProtocol.LTScan[idx] = idx + 1;    
                            else 
                                LTProtocol.LTScan[idx] = 0;    
                        }
#ifdef _USE_DEBUG_TXRX
                        printf("[_ProcRxLT] Nueva lista:\n");
                        for(int idx = 0;idx < _CANT_MAX_SLV; idx++)
                            printf("|Pos:%d - id:%d| ", idx + 1, LTProtocol.LTScan[idx]);
#endif
                        LTProtocol.LTDevAttached = RxLTHubStatus->DevAttached;
                        LTProtocol.LTDevDisabled = RxLTHubStatus->DevDisabled;
                    
                        // Se recibió la configuración de los terminales.
                        LTProtocol.status |= LT_STS_CONFIG_SLV_OK;
                    } 
                }
                break;

                /*                
                case LT_CMD_STATUS:
                break;
                */
 
            case LT_CMD_FW_FRAME:
            {
                stRxLTFwFrame *RxLTFwFrame = (stRxLTFwFrame *)rxbuff;

#ifdef _USE_DEBUG_TXRX
                printf("[_ProcRxLT] Rx Cmd LTFwFrame\n");

                printf(" flag : 0x%02X\n", RxLTFwFrame->flag);
                printf(" len  : %u\n",   RxLTFwFrame->len);
                printf(" Cmd  : 0x%02X\n", RxLTFwFrame->Cmd);

                printf(" HubID : ");
                for(int i = 0; i < 6; i++)
                    printf("%02X", RxLTFwFrame->HubID[i]);
                printf("\n");

                printf(" Version : %c.%c.%c\n",
                    RxLTFwFrame->ver[0],
                    RxLTFwFrame->ver[1],
                    RxLTFwFrame->ver[2]);

                printf(" FrameSize : %u\n", RxLTFwFrame->framesize);
                printf(" FrameNr   : %u\n", RxLTFwFrame->framenr);
                printf(" TotFrames : %u\n", RxLTFwFrame->totframes);
                
                printf(" CRC       : 0x%04X\n", RxLTFwFrame->Crc);

                printf(" Data (first 16 bytes): ");
                uint16_t dump = RxLTFwFrame->framesize;
                if(dump > 16) dump = 16;

                for(int i = 0; i < dump; i++)
                    printf("%02X ", RxLTFwFrame->buffer[i]);
                printf("\n");
#endif
                uint16_t reqframe = _GetFwUpdateReqFrame();
                
                if(reqframe)
                {
                    if(reqframe - 1 == RxLTFwFrame->framenr)
                    {    
#ifdef _USE_DEBUG_TXRX
                        printf("[_ProcRxLT] _StoreRxFrameEEPROM fn:%d, fsize:%d\n", RxLTFwFrame->framenr, RxLTFwFrame->framesize);
#endif  
                        _SetFramesInfo(RxLTFwFrame->totframes, RxLTFwFrame->framesize);

                        if(_StoreRxFrameEEPROM(RxLTFwFrame->framenr, RxLTFwFrame->buffer, RxLTFwFrame->framesize))     
                        {    
                            if((reqframe -1) >= RxLTFwFrame->totframes) _SetFwReqEnd();
                            else _SetFwUpdateReqFrame(reqframe);
                        }
                        else
                            printf("[_ProcRxLT] _StoreRxFrameEEPROM Failed\n");
                    }
                } 
            }
            break;

                default:
                printf("[_ProcRxLT] Rx Cmd Desconocido %02X\n", rxbuff[3]);
            } 
        }
    }    
}

void _ProcLTProto(void)
{
    static uint8_t xdiv = 0;
    
    if(xdiv++ >= 50)
    {
        ProcLCD(0);
        xdiv = 0;
    }
    // Si existe conexión con el modem
    if(_GetSrvComLinkRdy())
    {   
        // Si hay solicitud de frame de binario nethub:
        uint16_t tmpframe = _GetFwUpdateReqFrame();
        
        if(tmpframe && !_GetFwUpdateTxTimer())
        {
            uint8_t tmptx[1500];
            stTxLTFwFrame *pst = (stTxLTFwFrame *)tmptx;

            pst->flag = _LT_FLAG;
            pst->Cmd = LT_CMD_FW_FRAME;
            pst->len = 0;
            memcpy(pst->HubID, _GetHubID(), sizeof(pst->HubID));
            pst->framenr = tmpframe - 1;
            memcpy(pst->ver, _GetFwUpdateVer(), sizeof(pst->ver));

#ifdef _USE_DEBUG_TXRX
            printf("[_ProcLTProto] Tx->LT_CMD_FW_FRAME framenr (%d), newver %02X.%02X.%02X\n", pst->framenr, 
                                                                                               pst->ver[0],
                                                                                               pst->ver[1],
                                                                                               pst->ver[2]);
#endif
            // Transmite la estructura
            _TxServer((uint8_t*)pst, sizeof(stTxLTFwFrame));
            _SetFwUpdateTxTimer(_FW_UDATE_TWINDOW);
        }
        // Sino si existe configuración de los terminales y se completó el barrido:
        else if((LTProtocol.status & (LT_STS_SLV_SCAN_COMPLETE + LT_STS_CONFIG_SLV_OK)) == (LT_STS_SLV_SCAN_COMPLETE + LT_STS_CONFIG_SLV_OK))
        {
            uint8_t tmptx[1500];
            uint8_t cantslv = 0;
            uint8_t stsize = 0;
            stTxLTStatusG *pst = (stTxLTStatusG *)tmptx;
            uint8_t *pdatst = (&pst->ListEqs + 1);
            
            pst->flag = _LT_FLAG;
            pst->Cmd = LT_CMD_STATUS;
            pst->Seq = LTProtocol.seqId;
            pst->Group = LTProtocol.TxGroup;

            memcpy(pst->HubID, _GetHubID(), sizeof(pst->HubID));
            
            for(uint8_t idx = 0; idx < _CANT_MAX_SLV / 4;idx++)
            {
                stRxLTData *pstslvdat = &RxLTData[idx + (pst->Group * 4)];
         
                // Si la estructura tiene datos, los agrega al envío
                if((pstslvdat->status & (LTDATA_STS_ST1OK + LTDATA_STS_ST2OK)) == (LTDATA_STS_ST1OK + LTDATA_STS_ST2OK))
                {    
                    // agrega la st1 y st2
                    stsize = sizeof(stRxLTData) - sizeof(pstslvdat->status);
                    memcpy(pdatst, (uint8_t *)&pstslvdat->RxLTDatast1, stsize);
                    pdatst += stsize;
                    cantslv++;
                    // reset de los flags 
                    pstslvdat->status &= ~(LTDATA_STS_ST1OK + LTDATA_STS_ST2OK);
                }
            }
            pst->ListEqs = cantslv;
            
            
#ifndef _USE_TX_STATUS_CMD_EMPTYDEV
            if(pst->ListEqs)
#endif       
            {

#ifdef _USE_DEBUG_TXRX
                printf("[_ProcLTProto] Tx->_ProcLTProto block:%d, catslv:%d\n", 0, cantslv);
#endif
                // Transmite la estructura + cantidad de datos de slvs + longitud de crc
                _TxServer((uint8_t*)pst, sizeof(stTxLTStatusG) + (cantslv * stsize) + sizeof(uint16_t));
            }

            LTProtocol.status &= ~LT_STS_SLV_SCAN_COMPLETE;
        }   
        else if(LTProtocol.timeralive) LTProtocol.timeralive--;
        else
        {
            //printf("ready...\n");
            if(_GetSrvComCHFree())
            {
                printf("tx LTHubStatus\n");
        
                stTxLTHubStatus TxLTHubStatus;

                TxLTHubStatus.flag = _LT_FLAG;
                TxLTHubStatus.Cmd = LT_CMD_HUB_STATUS;
                TxLTHubStatus.Seq = LTProtocol.seqId;
                memcpy(TxLTHubStatus.HubID, _GetHubID(), sizeof(TxLTHubStatus.HubID));
                TxLTHubStatus.HubStatus = LTProtocol.HubStatus;
                TxLTHubStatus.HubErrsts = 0;
                TxLTHubStatus.HubEvent = 0;
                TxLTHubStatus.TimeRunning = _GetTimeRunning();
                TxLTHubStatus.FwVersion = _GetFirmwareVer();
                printf("ready to tx\n");
                _TxServer(&TxLTHubStatus, sizeof(TxLTHubStatus));
                printf("tx ok\n");
                
                LTProtocol.timeralive = _LTPROTO_TIMERALIVE;
            }
        }     
    }
}

void _ProcLTSlv(void)
{
    #ifdef _USE_DEBUG_TXRXSLV
    int32_t _tin, _tout;
    static uint16_t _tim1;
    #endif
    uint8_t nextflag = 0;
    static uint8_t st1st2 = 0;

    if(LTProtocol.status & LT_STS_SLV_SCAN_COMPLETE)
    {
        // espera a la tx a srvcomm

    }
          
    // Si está pendiente la tranmisión: 
    else if(LTProtocol.status & LT_STS_TX_BUSY)
    {
        // Si terminó de transmitir...
        if(_GetLoraIrqFlags() & LORA_IRQ_TX_DONE)
        {
            ClearLoraIRQFlags(0xff);
            SetLoraRxMode();
            LTProtocol.LTSlottime = _LTX_TIMESLOT;
            LTProtocol.status &= ~LT_STS_TX_BUSY;
            LTProtocol.status |= LT_STS_RX_MODE;
#ifdef _USE_DEBUG_TXRXSLV
    #ifdef _USE_DEBUG_TXDONE
            printf("[_ProcLTSlv] Tx Done (%dms)\n", _tim1);    
    #endif
            _tim1 = 0;
#endif
        }
#ifdef _USE_DEBUG_TXRXSLV        
        else
            _tim1++;
#endif    
    }
    else if(LTProtocol.status & LT_STS_RX_MODE)
    {
        stRxLTDatast1 *pst1 = (stRxLTDatast1*) &RxLTData[LTProtocol.LTCurrent].RxLTDatast1;
        stRxLTDatast2 *pst2 = (stRxLTDatast2*) &RxLTData[LTProtocol.LTCurrent].RxLTDatast2;
        stRxLTData *pst = &RxLTData[LTProtocol.LTCurrent];
        
        // Si hay interrupción por RX:
        if(ReadLoraIRQ() == 1)
        {    
            uint8_t tmprx[255] = {0};
#ifdef _USE_DEBUG_TXRXSLV        
            printf("[_ProcLTSlv] rx ok rssi = %d\n", _GetLoraRSSI());    
#endif
            uint8_t rx = SX1278_ReceiveBytes(tmprx, 255);
            if(rx)
            {
#ifdef _USE_DEBUG_TXRXSLV        
                printf("RX len (%d) t=%d\n",rx, _LTX_TIMESLOT - LTProtocol.LTSlottime);
                for(int idx = 0; idx < rx; idx++) printf("%02X",tmprx[idx]);
                printf("\n");    
#endif
                if(sizeof(stIOLuma) == rx)
                {
                    stIOLuma *RxIOLuma = (stIOLuma*)tmprx;
                    uint16_t rxcrc = crc_ccitt((uint8_t*)RxIOLuma, sizeof(stIOLuma) - sizeof(uint16_t));
                    if(rxcrc == RxIOLuma->crc)
                    {
#ifdef _USE_DEBUG_TXRXSLV        
                        printf("======== RxIOLuma frame ======\n");
                        printf("DAd->%02X%02X%02X%02X%02X\n", 
                                                            RxIOLuma->DAd[0],
                                                            RxIOLuma->DAd[1],
                                                            RxIOLuma->DAd[2],
                                                            RxIOLuma->DAd[3],
                                                            RxIOLuma->DAd[4]);
                        printf("Rad->%02X%02X%02X\n", 
                                                      RxIOLuma->RAd[0],
                                                      RxIOLuma->RAd[1],
                                                      RxIOLuma->RAd[2]);
                        printf("Csts %02X\n", RxIOLuma->CSts);
                        printf("frameidh %02X\n", RxIOLuma->frameidh);
                        printf("frameidl %02X\n", RxIOLuma->frameidl);
                        printf("rid %02X\n", RxIOLuma->rid);
#endif
                        // Si el terminal corresponde al consultado:
                        if(RxIOLuma->rid == (LTProtocol.LTCurrent + 1))
                        {
                            switch(RxIOLuma->CSts & 0x0f)
                            {
                                case LUMACMD_STS:
#ifdef _USE_DEBUG_TXRXSLV        
                                {
                                    stRxLTDatast1 *psetdebug = (stRxLTDatast1 *)RxIOLuma->data;

                                    printf("--- data struct st1---\n");

                                    printf("currSTS %02X\n", psetdebug->currSTS);
                                    printf("currV (%d)\n", psetdebug->currV);
                                    printf("currI %04x\n", psetdebug->currI);
                                    printf("TopV (%d)\n", psetdebug->TopV);
                                    printf("LowerV (%d)\n", psetdebug->LowerV);
                                    printf("timerunning (%lu)\n", psetdebug->timerunning);
                                    printf("t_01Wh (%lu)\n", psetdebug->t_01Wh);
                                    printf("r_01Wh (%lu)\n", psetdebug->r_01Wh);
                                    printf("---------------------\n");
                                }

#endif                                       
                                memcpy(pst1, RxIOLuma->data, sizeof(stRxLTDatast1));
                                pst->status |= LTDATA_STS_ST1OK;
                                break;
                                
                                case LUMACMD_STS2:
#ifdef _USE_DEBUG_TXRXSLV        
                                {
                                    stRxLTDatast2 *psetdebug = (stRxLTDatast2 *)RxIOLuma->data;
                                    
                                    printf("--- data struct st2---\n");
                                    printf("CDaT %08X\n", psetdebug->CDaT);
                                    printf("free{3} %02X%02X%02X\n", psetdebug->free[0], psetdebug->free[1],psetdebug->free[2]);
                                    printf("EqStatus %02X\n", psetdebug->EqStatus);
                                    printf("EqErrsts %02X\n", psetdebug->EqErrsts);
                                    printf("LowerVoltage (%d)\n", psetdebug->LowerVoltage);
                                    printf("PeakVoltage (%d)\n", psetdebug->PeakVoltage);
                                    printf("TimeCPURunning (%d)\n", psetdebug->TimeCPURunning);
                                    printf("FwVersion %04X\n", psetdebug->FwVersion);
                                }

#endif                                       
                                memcpy(pst2, RxIOLuma->data, sizeof(stRxLTDatast2));
                                pst->status |= LTDATA_STS_ST2OK;
                                break;

                                default:
                                printf("[_ProcLTSlv] Error: RX Cmd desconocido %02X\n", RxIOLuma->CSts);                 
                            }
                        }
                        else 
                            printf("[_ProcLTSlv] Error: Rx ID %d != %d\n",RxIOLuma->rid, LTProtocol.LTCurrent + 1); 
                    }
                    else
                        printf("[_ProcLTSlv] Error: RX CRC Error (%04X-%04X)\n", rxcrc, RxIOLuma->crc);                 
                }
                else
                    printf("[_ProcLTSlv] Warning:RX len out of range (%d)\n", rx);    
            }
            ClearLoraIRQFlags(0xff);
            nextflag = 1;
        }
        else if(LTProtocol.LTSlottime) LTProtocol.LTSlottime--;
        else 
        {
#ifdef _USE_DEBUG_TXRXSLV        
    #ifdef _USE_DEBUG_RXTIMEOUT
            printf("[_ProcLTSlv] Rx Timeout Slv id(%d)\n", LTProtocol.LTCurrent + 1);    
    #endif
#endif            
            nextflag = 1;
            // Return to TX mode
            LTProtocol.status &= ~LT_STS_RX_MODE;
        }
    }
    else if(_GetEnabledLTX(LTProtocol.LTCurrent))
    {
        uint8_t slvaddr = LTProtocol.LTCurrent + 1;

#ifdef _USE_DEBUG_TXRXSLV
    #ifdef _USE_DEBUG_TXDONE
        printf("[_ProcLTSlv] Tx Slv id(%d) - noise floor(%d)\n", slvaddr, _GetLoraLineRSSI());
    #endif
#endif
        stIOLuma IOLuma;
        if(st1st2 & 1)   
            IOLuma.CSts = LUMACMD_STS2;
        else 
            IOLuma.CSts = LUMACMD_STS;
            
#ifdef _USE_DEBUG_TXRXSLV
    if(st1st2 & 1) printf("*** TX2 addr %02X***\n", slvaddr);
    else  printf("*** TX1 addr %02X***\n", slvaddr);
#endif       
       

        memcpy(IOLuma.DAd, &LTProtocol.HubId[1], sizeof(LTProtocol.HubId) - 2);
        IOLuma.DAd[4] = slvaddr;
        memcpy(IOLuma.RAd, "\0x0\0x0\0x0", sizeof(IOLuma.RAd));
        IOLuma.crc = crc_ccitt((uint8_t*)&IOLuma, sizeof(stIOLuma) - sizeof(IOLuma.crc));
#ifdef _USE_DEBUG_TXRXSLV
        _tin = _GetTimerms();
#endif
        SX1278_SendBytes((uint8_t*)&IOLuma,sizeof(IOLuma), _SX1278_SETTXMODE);  
        LTProtocol.status |= LT_STS_TX_BUSY;

#ifdef _USE_DEBUG_TXRXSLV
        _tout = _GetTimerms();
        //printf("[_ProcLTSlv] Tx Fill FIFO time = %dms tin = %d\n", _tout - _tin, _tin);
#endif
    }
    // Sino si no está habilitado, pasa al siguiente:
    else nextflag = 1;
    // Si no hay tx pendiente avanza a siguiente nodo
    if(nextflag)
    {    
        if(++LTProtocol.LTCurrent >= _CANT_MAX_SLV) 
        {
            LTProtocol.LTCurrent = 0; 
            // Toggle de pedido de status
            if(++st1st2 >= 2)
            {
                LTProtocol.status |= LT_STS_SLV_SCAN_COMPLETE;
#ifdef _USE_DEBUG_TXRXSLV
                printf("*************** RONDA COMPLETA ******!!!!!\n");
#endif
                st1st2 = 0;
            }
        }
        LTProtocol.LTFrameid++;
    }
    nextflag = 0;
}    
