/***************************************************************************
*
* Copyright (c) 2001-2013
* Sigma Designs, Inc.
* All Rights Reserved
*
*---------------------------------------------------------------------------
*
* Description: This module implements functions used in combination with
*              command class firmware update.
*
* Author: Samer  Seoud
*
* Last Changed By: $Author: tro $
* Revision: $Revision: 0.00 $
* Last Changed: $Date: 2013/07/02 14:03:30 $
*
****************************************************************************/

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_sysdefs.h>
#include <ZW_tx_mutex.h>
#ifdef ZW_CONTROLLER
/*These are a part of the standard static controller API*/
#include <ZW_controller_api.h>
#endif
/* Enhanced Slave - needed for battery operation (RTC timer) on 100 series */
/* 200 Series have WUT */
#ifdef ZW_SLAVE_32
#include <ZW_slave_32_api.h>
#else
#ifdef  ZW_SLAVE
#include <ZW_slave_api.h>
#endif
#endif
/* ASIC power management functionality */
#include <ZW_power_api.h>
/* Allows data storage of application data even after reset */
#include <ZW_non_zero.h>
#include <eeprom.h>
#include <ZW_TransportLayer.h>

#include <ZW_crc.h>
#include <ZW_firmware_descriptor.h>
#include <ZW_Firmware_bootloader_defs.h>
#include <ZW_firmware_update_nvm_api.h>

#include <ZW_uart_api.h>
#include "misc.h"

#include <CommandClassFirmwareUpdate.h>
#include <ota_util.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

#ifdef ZW_DEBUG_CMD_OTA

#define ZW_DEBUG_CMD_OTA_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_CMD_OTA_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_CMD_OTA_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_CMD_OTA_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_CMD_OTA_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_CMD_OTA_SEND_BYTE(data)
#define ZW_DEBUG_CMD_OTA_SEND_STR(STR)
#define ZW_DEBUG_CMD_OTA_SEND_NUM(data)
#define ZW_DEBUG_CMD_OTA_SEND_WORD_NUM(data)
#define ZW_DEBUG_CMD_OTA_SEND_NL()
#endif

#ifdef ZW_DEBUG_SECURITY
#define ZW_DEBUG_SECURITY_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_SECURITY_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_SECURITY_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_SECURITY_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_SECURITY_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_SECURITY_SEND_BYTE(data)
#define ZW_DEBUG_SECURITY_SEND_STR(STR)
#define ZW_DEBUG_SECURITY_SEND_NUM(data)
#define ZW_DEBUG_SECURITY_SEND_WORD_NUM(data)
#define ZW_DEBUG_SECURITY_SEND_NL()
#endif

#define ZCB(func) void func(void); \
  code const void (code * func ## _p)(void) = &func; \
  void func()

typedef enum _FW_STATE_
{
  FW_STATE_DISABLE,
  FW_STATE_READY,
  FW_STATE_ACTIVE,
  FW_STATE_AWAITING_FORCE_DISABLE,
} FW_STATE;

typedef enum _FW_EV_
{
  FW_EV_WAIT_FOR_FORCE_DISABLE,
  FW_EV_VALID_COMBINATION,
  FW_EV_MD_REQUEST_REPORT_SUCCESS,
  FW_EV_MD_REQUEST_REPORT_FAILED,
  FW_EV_GET_NEXT_FW_FRAME,
  FW_EV_RETRY_NEXT_FW_FRAME,
  FW_EV_UPDATE_STATUS_SUCCESS,
  FW_EV_UPDATE_STATUS_UNABLE_TO_RECEIVE,
  FW_EV_UPDATE_STATUS_CRC_ERROR

} FW_EV;

#define FIRMWARE_UPDATE_PACKET_SIZE 40

#define FIRMWARE_UPDATE_REQUEST_PACKET_TIMEOUT  1000   /* unit: 10 ms ticks */

#define FIRMWARE_UPDATE_REQEUST_TIMEOUT_UNIT    200    /* unit: 10 ms ticks per sub-timeout */

/* number of sub-timeouts to achieve total of FIRMWARE_UPDATE_REQUEST_PACKET_TIMEOUT */
#define FIRMWARE_UPDATE_REQUEST_TIMEOUTS        (FIRMWARE_UPDATE_REQUEST_PACKET_TIMEOUT / FIRMWARE_UPDATE_REQEUST_TIMEOUT_UNIT)

#define FIRMWARE_UPDATE_MAX_RETRY 10

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/
 static WORD fw_firmwareDescriptorOffset;
 static t_firmwareDescriptor fw_firmwareDescriptor;
 static BYTE firmware_update_packetsize = FIRMWARE_UPDATE_PACKET_SIZE;

typedef struct _OTA_UTIL_
{
  BOOL (CODE *pOtaStart)(void);
  VOID_CALLBACKFUNC(pOtaFinish)(BYTE val);
  FW_STATE fwState;
  OTA_STATUS finishStatus;
  BYTE txOption;
  BYTE firmwareCrc1;
  BYTE firmwareCrc2;
  BYTE fw_numOfRetries;
  BYTE destNode;
  BYTE timerFwUpdateFrameGetHandle;
  WORD firmwareUpdateReportNumberPrevious;
  WORD fw_crcrunning;
  BYTE timerFWUpdateCount;
} OTA_UTIL;

OTA_UTIL myOta = {NULL, NULL, FW_STATE_DISABLE, OTA_STATUS_DONE ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0, 0};


/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/
void InitEvState();
WORD NVMCheckCRC16( WORD crc, DWORD nvmOffset, WORD blockSize);
void SetEvState( FW_EV ev);
void TimerCancelFwUpdateFrameGet();
void TimerStartFwUpdateFrameGet();
void ZCB_TimerOutFwUpdateFrameGet();
void ZCB_FinishFwUpdate();
void Reboot();
void OTA_WriteData(DWORD offset, BYTE* pData, WORD legth);
void OTA_Invalidate();


/*============================ Reboot ===============================
** Function description
** Reboot the node by enabling watchdog.
**
** Side effects:
****------------------------------------------------------------------*/
void
Reboot()
{
  ZW_WatchDogEnable(); /*reset asic*/
  while(1);
}


/*============================ OtaInit ===============================
** Function description
** Init Ota transmit options.
**
** Side effects:
**
**-------------------------------------------------------------------------*/
void
OtaInit(
  BYTE txOption,
  BOOL (CODE *pOtaStart)(void),
  VOID_CALLBACKFUNC(pOtaFinish)(BYTE val))
{
  myOta.txOption = txOption;
  myOta.pOtaStart = pOtaStart;
  myOta.pOtaFinish = pOtaFinish;
  ZW_FirmwareUpdate_NVM_Init();
}


/*============================ OTA_WriteData ===============================
** Function description
** Write data to the OTA NVM
**
** Side effects:
**
**-------------------------------------------------------------------------*/
void
OTA_WriteData(DWORD offset, BYTE *pData, WORD length)
{
  ZW_FirmwareUpdate_NVM_Write(pData, length, offset);
}


/*============================ OTA_Invalidate ===============================
** Function description
** Invalidate the FW update NVM to write a new image
**
** Side effects:
**
**-------------------------------------------------------------------------*/
void
OTA_Invalidate()
{
  /* Mark possible current NVM Image Not Valid */
  ZW_FirmwareUpdate_NVM_Set_NEWIMAGE(0);
}

/*======================== ZCB_UpdateStatusSuccess ==========================
** Timer callback to start CRC calculation *after* we have ack/routed-ack'ed
** the last fw update frame.
**
** Side effects:
**
**-------------------------------------------------------------------------*/
ZCB(ZCB_UpdateStatusSuccess)
{
  ZW_DEBUG_SECURITY_SEND_STR("OTA_SUCCESS_CB");
  SetEvState(FW_EV_UPDATE_STATUS_SUCCESS);
}


/*============================ handleCmdClassFirmwareUpdateMdReport ===============================
** Function description
** This function...
**
** Side effects:
**
**-------------------------------------------------------------------------*/
void
handleCmdClassFirmwareUpdateMdReport( WORD crc16Result,
                                      WORD firmwareUpdateReportNumber,
                                      BYTE properties,
                                      BYTE* pData,
                                      BYTE fw_actualFrameSize)
{
  ZW_DEBUG_CMD_OTA_SEND_STR("UpdateMdReport");
  ZW_DEBUG_CMD_OTA_SEND_NUM( crc16Result);
  ZW_DEBUG_CMD_OTA_SEND_NUM( firmwareUpdateReportNumber);
  ZW_DEBUG_CMD_OTA_SEND_NUM( properties);
  ZW_DEBUG_CMD_OTA_SEND_NUM( fw_actualFrameSize);
  ZW_DEBUG_CMD_OTA_SEND_NL();

  /*Check correct state*/
  if( FW_STATE_ACTIVE != myOta.fwState)
  {
    /*Not correct state.. just stop*/
    return;
  }

  /*Check checksum*/
  if (0 == crc16Result)
  {
    ZW_DEBUG_CMD_OTA_SEND_BYTE('C');
    myOta.fw_numOfRetries = 0;
    /* Check report number */
    if (firmwareUpdateReportNumber == myOta.firmwareUpdateReportNumberPrevious + 1)
    {
      /* Right number*/
      DWORD firstAddr = 0;
      IBYTE fw_frameIndex = 0;
      if (0 == myOta.firmwareUpdateReportNumberPrevious)
      {
        /* First packet sets the packetsize for the whole firmware update transaction */
        /* TODO: Make negativ response if packetsize too big... */
        firmware_update_packetsize = fw_actualFrameSize;
      }
      else
      {
        if ((firmware_update_packetsize != fw_actualFrameSize) && (!(properties & FIRMWARE_UPDATE_MD_REPORT_PROPERTIES1_LAST_BIT_MASK_V2)))
        {
          ZW_DEBUG_CMD_OTA_SEND_BYTE('p');
          /* (firmware_update_packetsize != fw_actualFrameSize) and not last packet - do not match.. do nothing. */
          /* Let the timer handle retries */
          return;
        }
      }
      ZW_DEBUG_CMD_OTA_SEND_BYTE('N');
      myOta.firmwareUpdateReportNumberPrevious = firmwareUpdateReportNumber;

      firstAddr = ((DWORD)(firmwareUpdateReportNumber - 1) * firmware_update_packetsize);
      myOta.fw_crcrunning = ZW_CheckCrc16(myOta.fw_crcrunning, pData, fw_actualFrameSize);

      OTA_WriteData(firstAddr, pData, fw_actualFrameSize);
      /* Is this the last report ? */
      if (properties & FIRMWARE_UPDATE_MD_REPORT_PROPERTIES1_LAST_BIT_MASK_V2)
      {
        /*check CRC for received dataBuffer*/
        if (((BYTE)(myOta.fw_crcrunning >> 8) == myOta.firmwareCrc1) && ((BYTE)myOta.fw_crcrunning == myOta.firmwareCrc2))
        {
          /* Delay starting the CRC calculation so we can transmit
           * the ack or routed ack first */
          if(0xFF == ZW_TIMER_START(ZCB_UpdateStatusSuccess, 10, 1))
          {
            ZW_DEBUG_SECURITY_SEND_STR("OTA_SUCCESS_NOTIMER");
            SetEvState(FW_EV_UPDATE_STATUS_SUCCESS);
          }
        }
        else
        {
          ZW_DEBUG_CMD_OTA_SEND_BYTE('_');
          SetEvState(FW_EV_UPDATE_STATUS_UNABLE_TO_RECEIVE);
        }
      }
      else
      {
        SetEvState(FW_EV_GET_NEXT_FW_FRAME);
      }
    }
    else{
      ZW_DEBUG_CMD_OTA_SEND_BYTE('n');
      /* (firmwareUpdateReportNumber == myOta.firmwareUpdateReportNumberPrevious + 1) do noth match.. do nothing.
         Let the timer hande retries*/
    }

  }
  else
  {
    ZW_DEBUG_CMD_OTA_SEND_BYTE('c');
    // TODO: else we have no indication if the checksum check fails ?????
    if (FIRMWARE_UPDATE_REQUEST_TIMEOUTS < myOta.timerFWUpdateCount)
    {
      if (FIRMWARE_UPDATE_MAX_RETRY < myOta.fw_numOfRetries)
      {
        ZW_DEBUG_CMD_OTA_SEND_STR("FAILED!");
        /* Send invalid status*/
        SetEvState(FW_EV_UPDATE_STATUS_UNABLE_TO_RECEIVE);
      }
    }
  }
}


/*============================ handleCmdClassFirmwareUpdateMdReqGet ========
** Function description
** This function...
**
** Side effects:
**
**-------------------------------------------------------------------------*/
void
handleCmdClassFirmwareUpdateMdReqGet(
  BYTE node,
  FW_UPDATE_GET* pData,
  BYTE* pStatus)
{
  ZW_DEBUG_CMD_OTA_SEND_STR("MdReqGet");
  ZW_DEBUG_CMD_OTA_SEND_NUM( pData->manufacturerId1);
  ZW_DEBUG_CMD_OTA_SEND_NUM( pData->manufacturerId2);
  ZW_DEBUG_CMD_OTA_SEND_NUM( pData->firmwareId1);
  ZW_DEBUG_CMD_OTA_SEND_NUM( pData->firmwareId2);
  ZW_DEBUG_CMD_OTA_SEND_NL();
  if(NULL != myOta.pOtaStart)
  {
    if(FALSE == myOta.pOtaStart())
    {
      ZW_DEBUG_CMD_OTA_SEND_BYTE('%');
      //SetEvState(FW_EV_FORCE_DISABLE);
      *pStatus = FIRMWARE_UPDATE_MD_REQUEST_REPORT_REQUIRES_AUTHENTICATION_V2;
      return;
    }
  }
  InitEvState();
  myOta.destNode = node;

  if (( pData->manufacturerId1 == (firmwareDescriptor.manufacturerID >> 8)) &&
      (pData->manufacturerId2 == (firmwareDescriptor.manufacturerID & 0xFF)) &&
      (pData->firmwareId1 == (firmwareDescriptor.firmwareID >> 8)) &&
      (pData->firmwareId2 == (firmwareDescriptor.firmwareID & 0xFF)))
  {
    ZW_DEBUG_CMD_OTA_SEND_BYTE('#');
    SetEvState(FW_EV_VALID_COMBINATION);
    myOta.firmwareCrc1 = pData->checksum1;
    myOta.firmwareCrc2 = pData->checksum2;
    firmware_update_packetsize = FIRMWARE_UPDATE_PACKET_SIZE;
    *pStatus = FIRMWARE_UPDATE_MD_REQUEST_REPORT_VALID_COMBINATION_V2;
  }
  else
  {
    ZW_DEBUG_CMD_OTA_SEND_BYTE('(');
    SetEvState(FW_EV_WAIT_FOR_FORCE_DISABLE);
    *pStatus = FIRMWARE_UPDATE_MD_REQUEST_REPORT_INVALID_COMBINATION_V2;
  }
}


code const void (code * ZCB_CmdClassFwUpdateMdReqReport_p)(BYTE txStatus) = &ZCB_CmdClassFwUpdateMdReqReport;
/*============================ ZCB_CmdClassFwUpdateMdReqReport ===============================
** Function description
** Callback function receive status on Send data FIRMWARE_UPDATE_MD_REQUEST_REPORT_V2
**
** Side effects:
**
**-------------------------------------------------------------------------*/
void
ZCB_CmdClassFwUpdateMdReqReport(BYTE txStatus)
{
  if(txStatus == TRANSMIT_COMPLETE_OK)
  {
    SetEvState(FW_EV_MD_REQUEST_REPORT_SUCCESS);
  }
  else{
    SetEvState(FW_EV_MD_REQUEST_REPORT_FAILED);
  }
}


/*============================ InitEvState ===============================
** Function description
** This function...
**
** Side effects:
**
**-------------------------------------------------------------------------*/
void
InitEvState()
{
  myOta.fwState = FW_STATE_DISABLE;
  myOta.fw_crcrunning = 0x1D0F;
  myOta.firmwareUpdateReportNumberPrevious = 0;
  myOta.fw_numOfRetries = 0;
  myOta.firmwareCrc1 = 0;
  myOta.firmwareCrc2 = 0;
  myOta.destNode = 0;
  myOta.timerFWUpdateCount = 0;
  if(0 == myOta.txOption)
  {
    myOta.txOption = TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_ACK;
  }
}


/*============================ SetState ===============================
** Function description
** This function...
**
** Side effects:
**
**-------------------------------------------------------------------------*/
void
SetEvState(FW_EV ev)
{
  ZW_DEBUG_CMD_OTA_SEND_STR("SetEvState ev=");
  ZW_DEBUG_CMD_OTA_SEND_NUM(ev);
  ZW_DEBUG_CMD_OTA_SEND_STR(", st=");
  ZW_DEBUG_CMD_OTA_SEND_NUM(myOta.fwState);
  ZW_DEBUG_CMD_OTA_SEND_NL();

  switch(myOta.fwState)
  {
    case FW_STATE_DISABLE:
      if(ev == FW_EV_VALID_COMBINATION)
      {
        myOta.fwState = FW_STATE_READY;
        SetEvState(ev);
      }
      else if(ev == FW_EV_WAIT_FOR_FORCE_DISABLE)
      {
        /* Force disable by rebooting when response done event arrives */
        myOta.fwState = FW_STATE_AWAITING_FORCE_DISABLE;
      }
      else{
        if(NULL != myOta.pOtaFinish)
        {
          myOta.pOtaFinish(OTA_STATUS_ABORT);
        }
        else
        {
          Reboot();
        }
      }
      break;


    case FW_STATE_READY:
      if (ev == FW_EV_VALID_COMBINATION)
      {
        OTA_Invalidate();
        myOta.fw_crcrunning = 0x1D0F;
        myOta.firmwareUpdateReportNumberPrevious = 0;
        //myOta.fw_numOfRetries = 0;
        //myOta.timerFwUpdateFrameGetHandle = 0;
        TimerCancelFwUpdateFrameGet();
        /*Stop timer*/
      }
      else if(ev == FW_EV_MD_REQUEST_REPORT_SUCCESS)
      {
        myOta.fwState = FW_STATE_ACTIVE;
        SetEvState(FW_EV_GET_NEXT_FW_FRAME);
      }
      else if((ev == FW_EV_MD_REQUEST_REPORT_FAILED)||
              (ev == FW_EV_GET_NEXT_FW_FRAME)||
              (ev == FW_EV_RETRY_NEXT_FW_FRAME)||
              (ev == FW_EV_UPDATE_STATUS_SUCCESS)||
              (ev == FW_EV_UPDATE_STATUS_UNABLE_TO_RECEIVE))
      {
        myOta.fwState = FW_STATE_DISABLE;
        if(NULL != myOta.pOtaFinish)
        {
          myOta.pOtaFinish(OTA_STATUS_ABORT);
        }
        else
        {
          Reboot();
        }

        /*Stop timer*/
      }
      break;


    case FW_STATE_ACTIVE:
      switch(ev)
      {
        case FW_EV_VALID_COMBINATION:
          TimerCancelFwUpdateFrameGet();
          /*Tell application it is aborted*/
          if(NULL != myOta.pOtaFinish)
          {
            myOta.pOtaFinish(OTA_STATUS_ABORT);
          }
          else
          {
            Reboot();
          }

          myOta.fwState = FW_STATE_DISABLE;
          break;

        case FW_EV_MD_REQUEST_REPORT_SUCCESS:
        case FW_EV_MD_REQUEST_REPORT_FAILED:
          /* Ignore - this happens when someone sends us Request Gets
           * while we are busy updating */
          break;

        case FW_EV_GET_NEXT_FW_FRAME:
          TimerStartFwUpdateFrameGet();
          CmdClassFirmwareUpdateMdGet(myOta.destNode, myOta.firmwareUpdateReportNumberPrevious + 1, myOta.txOption);
          /*Start/restart timer*/
          break;
        case FW_EV_RETRY_NEXT_FW_FRAME:
          ZW_DEBUG_CMD_OTA_SEND_STR(" FWUpdateCount ");
          ZW_DEBUG_CMD_OTA_SEND_NUM(myOta.timerFWUpdateCount);
          ZW_DEBUG_CMD_OTA_SEND_STR(" retry nbr ");
          ZW_DEBUG_CMD_OTA_SEND_NUM(myOta.fw_numOfRetries);
          if( FIRMWARE_UPDATE_REQUEST_TIMEOUTS < ++(myOta.timerFWUpdateCount))
          {
            myOta.timerFWUpdateCount = 0;
            if (FIRMWARE_UPDATE_MAX_RETRY > ++(myOta.fw_numOfRetries))
            {
              CmdClassFirmwareUpdateMdGet(myOta.destNode, myOta.firmwareUpdateReportNumberPrevious + 1, myOta.txOption);
              /*Start/restart timer*/
            }
            else
            {
              ZW_DEBUG_CMD_OTA_SEND_BYTE('+');
              SetEvState(FW_EV_UPDATE_STATUS_UNABLE_TO_RECEIVE);
            }
          }
          break;
        case FW_EV_UPDATE_STATUS_SUCCESS:
          TimerCancelFwUpdateFrameGet();
          myOta.finishStatus = OTA_STATUS_DONE;
          if(JOB_STATUS_BUSY == CmdClassFirmwareUpdateMdStatusReport( myOta.destNode,
                                                                      FIRMWARE_UPDATE_MD_STATUS_REPORT_SUCCESSFULLY_V2,
                                                                      myOta.txOption,
                                                                      ZCB_FinishFwUpdate))
          {
            /*Failed to send frame and we do not get a CB. Inform app we are finish*/
            if(NULL != myOta.pOtaFinish)
            {
              myOta.pOtaFinish(myOta.finishStatus);
            }
            else
            {
              Reboot();
            }
          }
          myOta.fwState = FW_STATE_DISABLE;
          break;
        case FW_EV_UPDATE_STATUS_UNABLE_TO_RECEIVE:
          TimerCancelFwUpdateFrameGet();
          myOta.finishStatus = OTA_STATUS_ABORT;
          ZW_DEBUG_CMD_OTA_SEND_STR("FIRMWARE_UPDATE_MD_STATUS_REPORT_UNABLE_TO_RECEIVE_V2");
          if (JOB_STATUS_BUSY == CmdClassFirmwareUpdateMdStatusReport(myOta.destNode,
                                                                      FIRMWARE_UPDATE_MD_STATUS_REPORT_UNABLE_TO_RECEIVE_V2,
                                                                      myOta.txOption,
                                                                      ZCB_FinishFwUpdate))
          {
            /*Failed to send frame and we do not get a CB. Inform app we are finish*/
            if(NULL != myOta.pOtaFinish)
            {
              myOta.pOtaFinish(myOta.finishStatus);
            }
            else
            {
              Reboot();
            }
          }
          myOta.fwState = FW_STATE_DISABLE;
          break;

        case FW_EV_UPDATE_STATUS_CRC_ERROR:
          TimerCancelFwUpdateFrameGet();
          myOta.finishStatus = OTA_STATUS_ABORT;
          if(JOB_STATUS_BUSY == CmdClassFirmwareUpdateMdStatusReport( myOta.destNode,
                                                                      FIRMWARE_UPDATE_MD_STATUS_REPORT_UNABLE_TO_RECEIVE_WITHOUT_CHECKSUM_ERROR_V2,
                                                                      myOta.txOption,
                                                                      ZCB_FinishFwUpdate))
          {
            /*Failed to send frame and we do not get a CB. Inform app we are finish*/
            if(NULL != myOta.pOtaFinish)
            {
              myOta.pOtaFinish(myOta.finishStatus);
            }
            else
            {
              Reboot();
            }
          }
          myOta.fwState = FW_STATE_DISABLE;
          break;

      }
      break;

    case FW_STATE_AWAITING_FORCE_DISABLE:
      if((ev == FW_EV_MD_REQUEST_REPORT_SUCCESS) ||
         (ev == FW_EV_MD_REQUEST_REPORT_FAILED))
      {
        /*Tell application it is aborted*/
        if(NULL != myOta.pOtaFinish)
        {
          myOta.pOtaFinish(OTA_STATUS_ABORT);
        }
        else
        {
          Reboot();
        }
        myOta.fwState = FW_STATE_DISABLE;
      }
      break;

  }
}


code const void (code * ZCB_FinishFwUpdate_p)(void) = &ZCB_FinishFwUpdate;
/*============================ ZCB_FinishFwUpdate ==========================
** Function description
** Callback Finish Fw update status to application.
** a new Get frame.
**
** Side effects:
**
**-------------------------------------------------------------------------*/
void
ZCB_FinishFwUpdate()
{
  if(NULL != myOta.pOtaFinish)
  {
    myOta.pOtaFinish(myOta.finishStatus);
  }
  else
  {
    /*Reoot device*/
    Reboot();
  }

}


/*============================ TimerCancelFwUpdateFrameGet ==================
** Function description
** Cancel timer for retries on Get next firmware update frame.
**
** Side effects:
**
**-------------------------------------------------------------------------*/
void
TimerCancelFwUpdateFrameGet()
{
  if (myOta.timerFwUpdateFrameGetHandle)
  {
    ZW_TIMER_CANCEL(myOta.timerFwUpdateFrameGetHandle);
    myOta.timerFwUpdateFrameGetHandle = 0;
  }
  myOta.fw_numOfRetries = 0;
}


code const void (code * ZCB_TimerOutFwUpdateFrameGet_p)(void) = &ZCB_TimerOutFwUpdateFrameGet;
/*============================ ZCB_TimerOutFwUpdateFrameGet =================
** Function description
** Callback on timeout on Get next firmware update frame. It retry to Send
** a new Get frame.
**
** Side effects:
**
**-------------------------------------------------------------------------*/
void
ZCB_TimerOutFwUpdateFrameGet()
{
  SetEvState(FW_EV_RETRY_NEXT_FW_FRAME);
}


/*============================ TimerStartFwUpdateFrameGet ==================
** Function description
** Start or restart timer for retries on Get next firmware update frame.
**
** Side effects:
**
**-------------------------------------------------------------------------*/
void
TimerStartFwUpdateFrameGet()
{
  myOta.fw_numOfRetries = 0;
  myOta.timerFWUpdateCount = 0;
  if (0 == myOta.timerFwUpdateFrameGetHandle)
  {
    /* Allocate timer for calling ZCB_TimerOutFwUpdateFrameGet every DIM_TIMER_FREQ, forever */
    myOta.timerFwUpdateFrameGetHandle = ZW_TIMER_START(ZCB_TimerOutFwUpdateFrameGet, FIRMWARE_UPDATE_REQEUST_TIMEOUT_UNIT, TIMER_FOREVER);
    if (0xFF == myOta.timerFwUpdateFrameGetHandle)
    {
      /* No timer! we update without a timer for retries */
      myOta.timerFwUpdateFrameGetHandle = 0;
    }
  }
  else
  {
    ZW_TIMER_RESTART(myOta.timerFwUpdateFrameGetHandle);
  }
}


