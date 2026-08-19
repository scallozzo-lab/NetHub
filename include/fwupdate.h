#ifndef __FWUPDATE_H__
    #define __FWUPDATE_H__

#include "main.h"

#define _FW_UDATE_TWINDOW   40//50

typedef struct 
{
    uint8_t stg;
    uint8_t status;
    uint16_t offset;
    uint16_t totframes;
    uint16_t framesize;
    uint8_t newver[3];
    uint16_t txtimer;
}stFwUpdate;

typedef enum
{
    _FW_STG_INIT    = 0,
    _FW_STG_GET_FRAME,


}efwupdatestg;

typedef enum
{
    _UPD_STS_REQ_VERSION    = BIT0,
    _UPD_STS_REQ_VER_STORED = BIT1,
    _UPD_STS_REQ_res2       = BIT2,
    _UPD_STS_REQ_res3       = BIT3,
    _UPD_STS_REQ_res4       = BIT4,
    _UPD_STS_REQ_res5       = BIT5,
    _UPD_STS_REQ_res6       = BIT6,
    _UPD_STS_REQ_res7       = BIT7
}eupdstsbits;


void _SetFwReqVersion(uint8_t *ver);
void _SetFramesInfo(uint16_t tframes, uint16_t fsize);
uint16_t _GetFrameTotFrame(void);
uint16_t _GetFrameFrameSize(void);
void _SetFwReqEnd(void);
uint16_t _GetFwUpdateReqFrame(void);
void _SetFwUpdateReqFrame(uint16_t f);
uint8_t *_GetFwUpdateVer(void);
uint8_t _GetFwUpdateStatus(void);
void _FwUpdateCtrl(void);
uint16_t _GetFwUpdateTxTimer(void);
void _SetFwUpdateTxTimer(uint16_t t);
int _StoreRxFrameEEPROM(uint16_t framepos, uint8_t *buf, uint16_t len);
void _FwUpdateInit(void);


#endif