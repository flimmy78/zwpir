/****************************************************************************
 *
 * Copyright (c) 2001-2013
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
 *---------------------------------------------------------------------------
 *
 * Description: Implements functions for transporting frames over the
 *               secure Z-Wave Network
 *
 * Author:   Valeriy Vyshnyak
 *
 * Last Changed By:  $Author: vvi $
 * Revision:         $Revision: 13417 $
 * Last Changed:     $Date: 2009-03-10 16:17:52 +0200 (Вв, 10 Бер 2009) $
 *
 ****************************************************************************/

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#ifdef ZW_CONTROLLER
#include <ZW_controller_api.h>
#else
#include <ZW_slave_api.h>
#endif
#include <ZW_tx_mutex.h>
#include <ZW_Security_AES_module.h>
#include <ZW_TransportSecurity.h>
#include <ZW_TransportLayer.h>
#include <ZW_cmd_class_list.h>
#include <battery_plus.h>
#include <ZW_debug_api.h>
#include <ZW_uart_api.h>
#include <ZW_Security_FSM.h>
#include <eeprom.h>
#include <slave_learn.h>
/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

#ifdef ZW_DEBUG_SECURITY_NIF
#define ZW_DEBUG_SECURITY_NIFS_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_SECURITY_NIFS_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_SECURITY_NIFS_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_SECURITY_NIFS_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_SECURITY_NIFS_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_SECURITY_NIFS_SEND_BYTE(data)
#define ZW_DEBUG_SECURITY_NIFS_SEND_STR(STR)
#define ZW_DEBUG_SECURITY_NIFS_SEND_NUM(data)
#define ZW_DEBUG_SECURITY_NIFS_SEND_WORD_NUM(data)
#define ZW_DEBUG_SECURITY_NIFS_SEND_NL()
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

#define NETWORK_KEY_SET_SIZE 18

#define SCHEME_GET_SIZE 3

#define EEPROM_MAGIC_BYTE_VALUE       0x56
#define TRANSPORT_EEPROM_SETTINGS_SIZE      (sizeof(EEOFFS_NETWORK_SECURITY_STRUCT) + 2)

//TO# 03444 TO# 03445
#define SECURITY_INCLUSION_TIMER_STOP 0xFF
#define SECURITY_INCLUSION_TIME_OUT   20
#define SECURITY_SEND_DATA_TIMER_STOP 0xFF

/* Scheme bitmask for use in Scheme Reports.
 * Bit 0 must be 0 to indicate support for Scheme 0. */
#define SECURITY_SCHEME_0_SUPPORTED 0

#ifndef SEC_2_POWERDOWNTIMEOUT
#define SEC_2_POWERDOWNTIMEOUT 20
#endif

#ifndef SEC_SCHEMENONCE_POWERDOWNTIMEOUT
#define SEC_SCHEMENONCE_POWERDOWNTIMEOUT 100
#endif

#ifndef MSEC_200_POWERDOWNTIMEOUT
#define MSEC_200_POWERDOWNTIMEOUT 2
#endif

#define SECURITY_KEY_S0_BIT 0x80
#define SECURITY_KEY_NONE_MASK 0x00

typedef struct _T_LAST_RXFRAME_
{
  BYTE cmdclass;
  BYTE secureCmdHandling;
}T_LAST_RXFRAME;


typedef struct _ST_TRANSPORT_NODE_INFORMATION_
{
  APP_NODE_INFORMATION *pNifs;
  BYTE *pCmdClassListNonSecureActive;            /* Nonsecure-list of supported command classes, when node communicate by this transport */
  BYTE cmdClassListNonSecureActiveCount;        /* Count of elements in supported command classes Nonsecure-list */
} TRANSPORT_NODE_INFORMATION;


typedef enum _SEC_SCHEME_ {
  SEC_SCHEME_NO = 0x00,    /**< No SCHEME */
  SEC_SCHEME_0 = 0x01,     /**< SCHEME 0  */
  SEC_SCHEME_AUTO = 0xFF   /**< SCHEME auto  */
} SEC_SCHEME;

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

//TO# 03444 TO# 03445
static BYTE securityInclusionTimeOut = SECURITY_INCLUSION_TIMER_STOP;
static BYTE securityInclusionTimerHandle = 0xFF;
static BYTE securitySendDataTimeOut = SECURITY_SEND_DATA_TIMER_STOP;
static BYTE securitySendDataTimerHandle = 0xFF;

#ifdef ZW_CONTROLLER
BYTE SECURITY_SCHEME = 0;
#endif
BYTE nodeSecure;
BYTE bTransportNID;
BYTE securityLife;

/* Timer handle for the timer turning node nonsecure 10 seconds
 * after inclusion complete callback from lower layer.
 * Value is 0 on power-up, exclude or SetDefault and 0xFF if timer has been cancelled.
 * Any other value indicates the timer is running. */
BYTE securityTimerHandle = 0;

BYTE nodeSecureStatus=0;
BYTE notSleep=0;

BYTE Transport_wakeUpReason;
BYTE code tmpkey[NETWORK_KEY_LENGTH] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static TRANSPORT_NODE_INFORMATION m_AppInfo = {NULL,NULL,0};

VOID_CALLBACKFUNC(ZCB_pSetPowerDownTimeout)(BYTE);

#define TSEC_CMD_HANDLING_DEFAULT   0
#define TSEC_CMD_HANDLING_SECURE    1
#define TSEC_CMD_HANDLING_UNSECURE  2
BYTE secureCmdHandling = TSEC_CMD_HANDLING_DEFAULT;  /* type of calling of ApplicationCommandHandler*/

static T_LAST_RXFRAME lastRxFrame;
VOID_CALLBACKFUNC(cbReqFuncStatusCb)(BYTE txStatus) = NULL;
VOID_CALLBACKFUNC(cbResFuncStatusCb)(BYTE txStatus) = NULL;
//static BOOL isInclusionMode = FALSE;

#define INVALID_NODE_ID 0xFF
#define APP_MAX_PAYLOAD 120   /* TODO: What is a reasonable size here? */
static struct {
  BYTE rxStatus;
  BYTE sourceNode;
  BYTE abCmd[APP_MAX_PAYLOAD];
  BYTE cmdLength;
} delayedAppCommandHandler;

/* We should only accept a network key set when a valid scheme get has been received.
 * This flag remembers if a scheme get has been received, and is checked
 * before accepting a network key set. */
static BYTE scheme_get_received;

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/
extern BYTE enNodeID;      /* Associated host id */
extern BYTE enNonce[8];    /* External nonce */

extern BYTE nrDataLength;                      /* Length of the payload data */
extern BYTE nrNodeID;                          /* Destignation node ID */
extern BYTE nrTXSecOptions;                    /* Security Options */
extern VOID_CALLBACKFUNC(nrCompletedFunc)(BYTE txStatus); /* Completed callback function */

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/


void ZCB_ReqFuncStatusCb(BYTE txStatus, void *psTxResult);
void ZCB_ResFuncStatusCb(BYTE txStatus, void *psTxResult);

static void DelayApplicationCommandHandler(
  BYTE rxStatus,
  BYTE sourceNode,
  BYTE *pCmd,
  BYTE cmdLength);

void SetupActiveNIF();

BYTE
Transport_SendRequestExt(
  BYTE nodeID,
  ZW_APPLICATION_TX_BUFFER *pBufData,
  BYTE dataLength,
  BYTE txOptions,
  VOID_CALLBACKFUNC_BYTE completedFunc,
  SEC_SCHEME secScheme);

code const void (code * ZCB_FuncZWSecure_p)(BYTE txStatus) = &ZCB_FuncZWSecure;
/*==============   ZCB_FuncZWSecure   ============================
**     callback function for Transport_OnApplicationInitSW
**
**    Side effects :
**
**--------------------------------------------------------------------------*/
void
ZCB_FuncZWSecure(BYTE txStatus)
{
  ZW_DEBUG_SECURITY_SEND_STR("funcZWSecure: ");
  ZW_DEBUG_SECURITY_SEND_NUM(txStatus);
  ZW_DEBUG_SECURITY_SEND_NL();

  if(NULL != ZCB_pSetPowerDownTimeout)
  {
    if (txStatus == TRANSPORT_WORK_END)
    {
      ZCB_pSetPowerDownTimeout(SEC_2_POWERDOWNTIMEOUT);
    }
    else if (txStatus == TRANSPORT_WORK_START)
    {
      ZCB_pSetPowerDownTimeout(SEC_SCHEMENONCE_POWERDOWNTIMEOUT); /*Add 10 sec timeout*/
    }
    else if (txStatus == TRANSPORT_WORK_ERROR)
    {
      ZCB_pSetPowerDownTimeout(SEC_2_POWERDOWNTIMEOUT);
    }
  }
}


code const void (code * ZCB_ReqFuncZWSecureCb_p)(BYTE txStatus, void *psTxResult) = &ZCB_ReqFuncStatusCb;
/*==============   ZCB_ReqFuncStatusCb   ============================
**     callback function for Transport_OnApplicationInitSW
**
**    Side effects :
**
**--------------------------------------------------------------------------*/
void
ZCB_ReqFuncStatusCb(BYTE txStatus, void *psTxResult)
{
  UNUSED(psTxResult);
  ZW_DEBUG_SECURITY_SEND_STR("ZCB_ReqFuncStatusCb: ");
  ZW_DEBUG_SECURITY_SEND_NUM(txStatus);
  ZW_DEBUG_SECURITY_SEND_NL();

  if(NULL != ZCB_pSetPowerDownTimeout)
  {
#ifdef FLIRS
    ZCB_pSetPowerDownTimeout(SEC_2_POWERDOWNTIMEOUT);

#else
    ZCB_pSetPowerDownTimeout(MSEC_200_POWERDOWNTIMEOUT);
#endif
  }

  if(NULL != cbReqFuncStatusCb)
  {
    cbReqFuncStatusCb(txStatus);
  }
}

code const void (code * ZCB_ResFuncZWSecureCb_p)(BYTE txStatus, void *psTxResult) = &ZCB_ResFuncStatusCb;
/*==============   ZCB_ResFuncStatusCb   ============================
**     callback function for Transport_OnApplicationInitSW
**
**    Side effects :
**
**--------------------------------------------------------------------------*/
void
ZCB_ResFuncStatusCb(BYTE txStatus, void *psTxResult)
{
  UNUSED(psTxResult);
  ZW_DEBUG_SECURITY_SEND_STR("ZCB_ResFuncStatusCb: ");
  ZW_DEBUG_SECURITY_SEND_NUM(txStatus);
  ZW_DEBUG_SECURITY_SEND_NL();

  if(NULL != ZCB_pSetPowerDownTimeout)
  {
#ifdef FLIRS
    ZCB_pSetPowerDownTimeout(SEC_2_POWERDOWNTIMEOUT);

#else
    ZCB_pSetPowerDownTimeout(MSEC_200_POWERDOWNTIMEOUT);
#endif
  }

  if(NULL != cbResFuncStatusCb)
  {
    cbResFuncStatusCb(txStatus);
  }
}

void ZCB_TxSecureDone(BYTE bStatus);
code const void (code * ZCB_TxSecureDone_p)(BYTE bStatus) = &ZCB_TxSecureDone;
/*==============================   TxSecureDone   ============================
**
**  Function:  Transmit of a frame done, start the powerdown timer
**
**  Side effects: None
**
**--------------------------------------------------------------------------*/
void
ZCB_TxSecureDone(BYTE bStatus)
{
  UNUSED(bStatus);
  ZCB_FuncZWSecure(TRANSPORT_WORK_END);
}

code const void (code * ZCB_SecureSessionDone_p)(BYTE bStatus, void *psTxResult) = &ZCB_SecureSessionDone;
/*===================== ZCB_SecureSessionDone =================================
**    Function description:
**      Called when a security session is complete and callback to application
**      should be made. A security session is defined as the Nonce Get,
**      Nonce Report, Secure Msg Encap sequence.
**
**      This function is suitable for use as ZW_SendData callback.
**
**    Global state changed:
**      FreeSecurityMutex
**
**    Global vars read:
**    nrCompletedFunc
**
**--------------------------------------------------------------------------*/
void											/* RET nothing */
ZCB_SecureSessionDone(
  BYTE bStatus,                           /* IN TX status */
  void *psTxResult)             /* IN Detailed results of tx */
{
  UNUSED(psTxResult);
  ZW_DEBUG_SECURITY_SEND_STR("security app callback\r\n");
  FreeSecurityMutex();
  PostEvent(EV_RETURN_TO_IDLE);
  ZCB_FuncZWSecure(TRANSPORT_WORK_END);
  if (NON_NULL(nrCompletedFunc))
  {
    nrCompletedFunc(bStatus);
  }
}

/*=============================   SaveSecureNetworkKey   =====================
**    SaveSecureNetworkKey
**
**    Side effects :
**
**--------------------------------------------------------------------------*/
void
SaveSecureNetworkKey(void)
{
  ZW_MEM_PUT_BUFFER_NO_CB((WORD)&nvmApplDescriptor.EEOFFS_SECURITY.EEOFFS_NETWORK_KEY_START_field[0], networkKey, NETWORK_KEY_LENGTH);
}


BYTE IsNetworkIncludeKeyInclude()
{
	BYTE y=0,i = 0;

    for (i = 0; i < NETWORK_KEY_LENGTH; i++)
    {
      y= y | networkKey[i];
    }
	return !y;
}

void ZCB_SecurityInclusionTimer(void);
code const void (code * ZCB_SecurityInclusionTimer_p)(void) = &ZCB_SecurityInclusionTimer;
void
ZCB_SecurityInclusionTimer()
{
  ZW_DEBUG_SECURITY_SEND_STR("\r\nInclTim");
  ZW_DEBUG_SECURITY_SEND_NUM(securityInclusionTimeOut);
  ZW_DEBUG_SECURITY_SEND_NL();
  if (securityInclusionTimeOut == 0)
  {
    if (securityInclusionTimerHandle != 0xff)
    {
      ZW_TIMER_CANCEL(securityInclusionTimerHandle);
      securityInclusionTimerHandle = 0xff;
    }
    Transport_SetNodeSecure(NON_SECURE_NODE);
    scheme_get_received = FALSE;
    ZCB_FuncZWSecure(TRANSPORT_WORK_END);
  }
  else if ((securityInclusionTimeOut != SECURITY_INCLUSION_TIMER_STOP)&&(securityInclusionTimeOut > 0))
  {
    securityInclusionTimeOut--;
  }
}

static void
StartSecurityInclusionTimer(BYTE timeOut)
{
  if (securityInclusionTimerHandle == 0xFF)
  {
    securityInclusionTimeOut = timeOut;
    securityInclusionTimerHandle = ZW_TIMER_START(ZCB_SecurityInclusionTimer, TIMER_ONE_SECOND, TIMER_FOREVER);
  }
}

static void
StopSecurityInclusionTimer()
{
  if (securityInclusionTimerHandle != 0xff)
  {
    ZW_TIMER_CANCEL(securityInclusionTimerHandle);
    securityInclusionTimerHandle = 0xff;
  }
  securityInclusionTimeOut = SECURITY_INCLUSION_TIMER_STOP;
}


void ZCB_SecuritySendDataTimer(void);
code const void (code * ZCB_SecuritySendDataTimer_p)(void) = &ZCB_SecuritySendDataTimer;
void
ZCB_SecuritySendDataTimer()
{
  if (securitySendDataTimeOut == 0)
  {
    if (securitySendDataTimerHandle != 0xff)
    {
      ZW_TIMER_CANCEL(securitySendDataTimerHandle);
      securitySendDataTimerHandle = 0xff;
    }
    ZCB_FuncZWSecure(TRANSPORT_WORK_END);
  }
  else if (securitySendDataTimeOut != SECURITY_SEND_DATA_TIMER_STOP)
  {
    securitySendDataTimeOut--;
  }
}
void
StartSecuritySendDataTimer(BYTE timeOut)
{
  securitySendDataTimeOut = timeOut;
  if (securitySendDataTimerHandle != 0xff)
  {
    ZW_TIMER_CANCEL(securitySendDataTimerHandle);
  }
  securitySendDataTimerHandle = ZW_TIMER_START(ZCB_SecuritySendDataTimer, TIMER_ONE_SECOND, TIMER_FOREVER);
}
static void
StopSecuritySendDataTimer()
{
  if (securitySendDataTimerHandle != 0xff)
  {
    ZW_TIMER_CANCEL(securitySendDataTimerHandle);
    securitySendDataTimerHandle = 0xff;
  }
  securitySendDataTimeOut = SECURITY_SEND_DATA_TIMER_STOP;
}

void ZCB_cbTransportSecurityVoidByte(BYTE b);
code const void (code * ZCB_cbTransportSecurityVoidByte_p)(BYTE b) = &ZCB_cbTransportSecurityVoidByte;
/*==============================   cbVoidByte   ============================
**
**  Function:  stub for callback
**
**  Side effects: None
**
**--------------------------------------------------------------------------*/
void ZCB_cbTransportSecurityVoidByte(BYTE b)
{
  UNUSED(b);
  //ZW_DEBUG_SEND_BYTE('C');
}

/*========================   ApplicationCommandHandler   ====================
**    Handling of a received application commands and requests
**
*
**--------------------------------------------------------------------------*/
void                              /*RET Nothing                  */
ApplicationCommandHandler(
  BYTE  rxStatus,                 /* IN Frame header info */
  BYTE  sourceNode,               /* IN Command sender Node ID */
  ZW_APPLICATION_TX_BUFFER *pCmd, /* IN Payload from the received frame, the union */
                                  /*    should be used to access the fields */
  BYTE   cmdLength)               /* IN Number of command bytes including the command */
{
  BYTE cbFuncCall;
  BYTE txOption;

  cbFuncCall = FALSE;


  txOption = ((rxStatus & RECEIVE_STATUS_LOW_POWER) ? TRANSMIT_OPTION_LOW_POWER : 0)
             | TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_RETURN_ROUTE|TRANSMIT_OPTION_EXPLORE;


  ZW_DEBUG_SECURITY_NIFS_SEND_STR("\r\nAppCmdH ");
  ZW_DEBUG_SECURITY_NIFS_SEND_BYTE('U');
  ZW_DEBUG_SECURITY_NIFS_SEND_NUM(nodeSecure);
  ZW_DEBUG_SECURITY_NIFS_SEND_BYTE(' ');
  ZW_DEBUG_SECURITY_NIFS_SEND_NUM(pCmd->ZW_Common.cmdClass);
  ZW_DEBUG_SECURITY_NIFS_SEND_NUM(pCmd->ZW_Common.cmd);

  if (pCmd->ZW_Common.cmdClass == COMMAND_CLASS_SECURITY)
  {
    //ZW_DEBUG_SEND_STR("SppH ");
    //ZW_DEBUG_SEND_NUM(pCmd->ZW_Common.cmd);
    //ZW_DEBUG_SEND_NL();
    /* TO#4147 Ignore Scheme get when already included */
    if (pCmd->ZW_Common.cmd == SECURITY_SCHEME_GET
        && !IsNetworkIncludeKeyInclude())
    {
      ZW_DEBUG_SECURITY_SEND_STR("\r\nScheme Get with Netkey: ");
      ZW_DEBUG_SEND_NUM(networkKey[0]);
      ZW_DEBUG_SEND_NL();
      return;
    }

    ZCB_FuncZWSecure(TRANSPORT_WORK_START);
    cbFuncCall = TRUE;

    if (nodeSecure != NON_SECURE_NODE)
    {

      if (pCmd->ZW_Common.cmd == SECURITY_SCHEME_GET)
      {

        StartSecurityInclusionTimer(SECURITY_INCLUSION_TIME_OUT);

        if (cmdLength >= SCHEME_GET_SIZE)
        {
          /* Scheme byte is valid regardless of contents.
           * We can safely assume that any controller sending Scheme Get
           * is using scheme0 to include us. */
          code BYTE schemeReportBuf[] = {COMMAND_CLASS_SECURITY, SECURITY_SCHEME_REPORT, SECURITY_SCHEME_0_SUPPORTED};
          memset(networkKey, 0x00, NETWORK_KEY_LENGTH);
          SaveSecureNetworkKey();
          LoadKeys();
          scheme_get_received = TRUE;
          ZW_DEBUG_SECURITY_SEND_BYTE('=');
          /* For once, don't check ZW_SendData return value. Either way we
           * take the same action. */
          ZW_SEND_DATA(sourceNode, schemeReportBuf, sizeof(schemeReportBuf), txOption|TRANSMIT_OPTION_EXPLORE, NULL);
          StartSecurityTimeOut(TIME_SECURITY_LIFE);
          cbFuncCall = FALSE;   /* do not call status callback in this handler because work in progress and it will be called by the timeout or when security layer are finish work */
        }
        else
        {
          /* Scheme Get too short - abort */
          Transport_SetNodeSecure(NON_SECURE_NODE);
          StopSecurityInclusionTimer();
        }
      }
      else if (securityInclusionTimeOut == 0)
      {
        ZCB_FuncZWSecure(TRANSPORT_WORK_END);
        return;
      }

      if (pCmd->ZW_Common.cmd == SECURITY_NONCE_REPORT)
      {
        ZW_DEBUG_SECURITY_SEND_STR("\r\nComparing expected/actual nonce sender nodeid: ");
        ZW_DEBUG_SECURITY_SEND_NUM(sourceNode);
        ZW_DEBUG_SECURITY_SEND_STR(" ");
        ZW_DEBUG_SECURITY_SEND_NUM(nrNodeID);
        ZW_DEBUG_SECURITY_SEND_NL();

        if (sourceNode != nrNodeID)
        {
          ZW_DEBUG_SEND_STR("\r\nNonce from wrong sender\r\n");
          ZCB_FuncZWSecure(TRANSPORT_WORK_END);
          return;
        }

        if (securitySendDataTimeOut == 0)
        {
          ZCB_FuncZWSecure(TRANSPORT_WORK_END);
          return;
        }
        else if (securitySendDataTimeOut != SECURITY_SEND_DATA_TIMER_STOP)
        {
          StopSecuritySendDataTimer();
        }

        StopNonceGetTimerB();

        /* Register the external nonce */
        enNodeID = sourceNode;
        memcpy(enNonce, ((BYTE*)pCmd) + 2, 8);
        // TO# 03329
        // When a node sends a routed nonce_get frame.
        // Then there are possiblity that we received a nonce_report
        // while we are still waiting for the nonce_get tx complete callback.
        ZW_DEBUG_SECURITY_SEND_BYTE('.');
        ZW_DEBUG_SECURITY_SEND_BYTE('1');
        ZW_DEBUG_SECURITY_SEND_BYTE('.');
        ZW_DEBUG_SECURITY_SEND_NUM(sourceNode);
        ZW_DEBUG_SECURITY_SEND_BYTE('.');
        ZW_DEBUG_SECURITY_SEND_BYTE('.');
        ZW_DEBUG_SECURITY_SEND_STR("\r\nRX nonce: ");
        ZW_DEBUG_SECURITY_SEND_NUM(enNonce[0]);
        ZW_DEBUG_SECURITY_SEND_NUM(enNonce[1]);
        ZW_DEBUG_SECURITY_SEND_NUM(enNonce[2]);
        ZW_DEBUG_SECURITY_SEND_NL();

        if (SecurityNonceGetInProgress == FALSE)
        {
          // nonce_get tx complete callback is allready finished at this moment
          SecurityDelayedNonceReport = FALSE;
          NonceHasBeenReceived();
        }
        else
        {
          // nonse_get callback is in waiting state,
          // we will invoke NonceHasBeenReceived() later
          SecurityDelayedNonceReport = TRUE;
          ZW_DEBUG_SECURITY_SEND_BYTE('9');
        }
      }
      else
      {
        /* All other Security command class commands are handled in ProcessIncommingSecure !!!*/
        /* Which also decrypts the encapsulated message if any present and its proper authenticated */
        ((BYTE*)pCmd)+= 1;        /* 1 - this is a size of command class value, removed, because ProcessIncomingSecure need Security Header byte as first byte. */
        cmdLength -= 1;           /* length is corrected accordingly to the above changes of pointer */

        cmdLength = ProcessIncomingSecure(sourceNode, (BYTE_P)pCmd, txOption | TRANSMIT_OPTION_EXPLORE, cmdLength);
        ZW_DEBUG_SECURITY_SEND_NUM(cmdLength);

        if (cmdLength != 0 && cmdLength != 0xff)
        {
          ((BYTE*)pCmd) += 9 + 1; /* 9 - is a offset of decrypted data, decrypted by ProcessIncomingSecure. */
                                  /* 1 - this is a size of service byte in message encapsulation frame with fields: Reserved, Second Frame, Sequenced, Sequence Counter. */
          cmdLength -= 1;         /* 1 - this is a size of service byte described above. */

          if(cmdLength >= 2)
          {
            switch (pCmd->ZW_Common.cmdClass)
            {
              case COMMAND_CLASS_SECURITY:
              {
                switch (pCmd->ZW_Common.cmd)
                {
                case SECURITY_COMMANDS_SUPPORTED_GET:
                  {
                    ZW_APPLICATION_TX_BUFFER *pTxBuf = GetRequestBuffer(ZCB_TxSecureDone);
                    //ZW_DEBUG_SEND_BYTE('R');
                    /*Check pTxBuf is free*/
                    if(NULL != pTxBuf)
                    {
                      enNodeID = 0xff;
                      pTxBuf->ZW_SecurityCommandsSupportedReport1byteFrame.cmdClass = COMMAND_CLASS_SECURITY;
                      pTxBuf->ZW_SecurityCommandsSupportedReport1byteFrame.cmd = SECURITY_COMMANDS_SUPPORTED_REPORT;
                      pTxBuf->ZW_SecurityCommandsSupportedReport1byteFrame.reportsToFollow = 0;

                      if ( NULL != m_AppInfo.pNifs->cmdClassListSecure )
                      {
                        memcpy(&(pTxBuf->ZW_SecurityCommandsSupportedReport1byteFrame.commandClassSupport1),
                            m_AppInfo.pNifs->cmdClassListSecure, m_AppInfo.pNifs->cmdClassListSecureCount);
                      }
                      (&(pTxBuf->ZW_SecurityCommandsSupportedReport1byteFrame.commandClassSupport1))[m_AppInfo.pNifs->cmdClassListSecureCount] = COMMAND_CLASS_MARK;

                      //ZW_DEBUG_SEND_BYTE('R');
                      if (Transport_SendRequestExt(sourceNode, (ZW_APPLICATION_TX_BUFFER *)pTxBuf,
                        sizeof(pTxBuf->ZW_SecurityCommandsSupportedReport1byteFrame) - 2 + m_AppInfo.pNifs->cmdClassListSecureCount,
                        txOption | TRANSMIT_OPTION_EXPLORE, ZCB_RequestJobStatus, SEC_SCHEME_0))
                      {
                        cbFuncCall = FALSE;    /* do not call status callback in this handler because it will be called when report will be sent */
                      }
                      else
                      {
                        /*Job failed, free transmit-buffer pTxBuf by clearing mutex */
                        FreeRequestBuffer();
                      }
                    }
                    else
                    {
                      /*pTxBuf is occupied.. Hmm..!?*/
                      ZW_DEBUG_SEND_STR("SECURITY_COMMANDS_SUPPORTED_REPORT p NULL");
                    }
                  }
                  break;

                case SECURITY_COMMANDS_SUPPORTED_REPORT:
                  /* */
                  /* Drop through */
                  break;

                case NETWORK_KEY_SET:
                  if (cmdLength < NETWORK_KEY_SET_SIZE)
                  {
                    Transport_SetNodeSecure(NON_SECURE_NODE);
                    scheme_get_received = FALSE;
                    ZCB_FuncZWSecure(TRANSPORT_WORK_END);
                    return;
                  }
                  if(!IsNetworkIncludeKeyInclude()
                      || !scheme_get_received)
                  {
                    ZCB_FuncZWSecure(TRANSPORT_WORK_END);
                    return;
                  }
                  scheme_get_received = FALSE;
                  if(securityTimerHandle != 0xFF)
                  {
                    ZW_TIMER_CANCEL(securityTimerHandle);
                    securityTimerHandle = 0xFF;
                  }

                  StopSecurityInclusionTimer();

                  memcpy(networkKey, &pCmd->ZW_NetworkKeySet1byteFrame.networkKeyByte1, NETWORK_KEY_LENGTH);
                  /* Now network Key should contain the new networkKey */
                  /* Save the new networkKey in NV RAM */
                  SaveSecureNetworkKey();
                  Transport_SetNodeSecure(SECURE_NODE);
                  /* Go and make the Authentication and the encryption keys */
                  LoadKeys();
                  enNodeID = 0xff;
                  {
                    /*Most be App-Tx-buffer because of the hijack functionality.*/
                    ZW_APPLICATION_TX_BUFFER *pTxBuf = GetRequestBuffer(ZCB_TxSecureDone);
                    /*Check pTxBuf is free*/
                    if(NULL != pTxBuf)
                    {
                      pTxBuf->ZW_NetworkKeyVerifyFrame.cmdClass = COMMAND_CLASS_SECURITY;
                      pTxBuf->ZW_NetworkKeyVerifyFrame.cmd = NETWORK_KEY_VERIFY;
                      //ZW_DEBUG_SEND_BYTE('K');
                      if (Transport_SendRequestExt(sourceNode, (ZW_APPLICATION_TX_BUFFER *)pTxBuf, sizeof(pTxBuf->ZW_NetworkKeyVerifyFrame), txOption | TRANSMIT_OPTION_EXPLORE, ZCB_RequestJobStatus, SEC_SCHEME_0))
                      {
                        cbFuncCall = FALSE;    /* do not call status callback in this handler because it will be called when report will be sent */
                      }
                      else
                      {
                        /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
                        FreeRequestBuffer();
                      }
                    }
                    else
                    {
                      /*pTxBuf is occupied.. Hmm..!?*/
                      ZW_DEBUG_SECURITY_SEND_STR("NETWORK_KEY_VERIFY p NULL");
                    }
                  }
                  break;
                }
                break;
              }

              default:
              {
                /* All other commands, decrypted from the message encapsulation - pass to the application layer: */

                /* call status callback with "work end" flag here because security are finished their job: */
                ZCB_FuncZWSecure(TRANSPORT_WORK_END);

                cbFuncCall = FALSE;
                /* Check if cmd Class are supported in current mode (unsecure or secure)
                 * and only pass application level secure commands after we have been included.
                 * Otherwise, we would pass zero-key encrypted frames to app. The zero-key is only
                 * intended for exchanging the actual network key.
                 * */
                if((nodeSecure == SECURE_NODE)
                    && (TRUE == CmdClassSupported( TRUE,
                                               pCmd->ZW_Common.cmdClass,
                                               m_AppInfo.pNifs->cmdClassListSecure,
                                               m_AppInfo.pNifs->cmdClassListSecureCount,
                                               m_AppInfo.pCmdClassListNonSecureActive,
                                               m_AppInfo.cmdClassListNonSecureActiveCount)))
                {
                  lastRxFrame.cmdclass = pCmd->ZW_Common.cmdClass;
                  lastRxFrame.secureCmdHandling = TSEC_CMD_HANDLING_SECURE;     //message is secured, so reports also must be secured.
                  if(NULL != ZCB_pSetPowerDownTimeout)
                  {
                    /*Msg received restart timer for 2 sec.*/
                    ZCB_pSetPowerDownTimeout(SEC_2_POWERDOWNTIMEOUT);
                  }

                  /* call application layer handler: */
                  if (ST_IDLE != GetSecurityFsmState())
                  {
                    /* Delay app callback until security FSM is idle */
                    DelayApplicationCommandHandler(rxStatus, sourceNode, (BYTE_P)pCmd, cmdLength);
                  }
                  else
                  {
                    Transport_ApplicationCommandHandler(rxStatus, sourceNode, pCmd, cmdLength);
                  }
                  break;
                }

              }
            }
          }
        }
      }
    }
  }
  else
  {
    /*Check if cmd Class are supported in current mode (unsecure or secure)*/
    if( TRUE == CmdClassSupported( FALSE,
                                   pCmd->ZW_Common.cmdClass,
                                   m_AppInfo.pNifs->cmdClassListSecure,
                                   m_AppInfo.pNifs->cmdClassListSecureCount,
                                   m_AppInfo.pCmdClassListNonSecureActive,
                                   m_AppInfo.cmdClassListNonSecureActiveCount))
    {
      lastRxFrame.cmdclass = pCmd->ZW_Common.cmdClass;
      lastRxFrame.secureCmdHandling = TSEC_CMD_HANDLING_UNSECURE;       //message is not secured, so reports also must be sent non-secure.
      if(NULL != ZCB_pSetPowerDownTimeout)
      {
        /*Msg received restart timer for 2 sec.*/
        ZCB_pSetPowerDownTimeout(SEC_2_POWERDOWNTIMEOUT);
      }

      /* All other non encrypted commands and non-security class commands - pass to the application layer: */
  //    if(!nodeSecure)
    	{
        if(NULL != ZCB_pSetPowerDownTimeout)
        {
          #ifdef FLIRS
          ZCB_pSetPowerDownTimeout(SEC_2_POWERDOWNTIMEOUT);
          #else
          ZCB_pSetPowerDownTimeout(MSEC_200_POWERDOWNTIMEOUT);
          #endif
        }
    		Transport_ApplicationCommandHandler(rxStatus, sourceNode, pCmd, cmdLength);
    	}
    }
  }

  if(cbFuncCall)
  {
    ZCB_FuncZWSecure(TRANSPORT_WORK_END);
  }
}



/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/
BYTE
Transport_SendRequest(
  BYTE tnodeID,
  ZW_APPLICATION_TX_BUFFER *pBufData,
  BYTE dataLength,
  BYTE txOptions,
  VOID_CALLBACKFUNC_BYTE completedFunc,
  BYTE isForcedNonSencure)                     /* FALSE: send dependend of connection secure level. TRUE: send nonsecure */
{
  SEC_SCHEME secScheme = SEC_SCHEME_AUTO;

  if(TRUE == isForcedNonSencure)
  {
    secScheme = SEC_SCHEME_NO;
  }

  return Transport_SendRequestExt( tnodeID, pBufData, dataLength, txOptions, completedFunc, secScheme);
}


/*========================   Transport_SendRequest   ============================
**      Send request command over secure network
**    Side effects :
**
**--------------------------------------------------------------------------*/
BYTE
Transport_SendRequestExt(
  BYTE tnodeID,
  ZW_APPLICATION_TX_BUFFER *pBufData,
  BYTE dataLength,
  BYTE txOptions,
  VOID_CALLBACKFUNC_BYTE completedFunc,
  SEC_SCHEME secScheme)
{
  ZW_DEBUG_SECURITY_NIFS_SEND_STR("\r\nTransport_SendRequestExt ");
  ZW_DEBUG_SECURITY_NIFS_SEND_NUM(tnodeID);
  ZW_DEBUG_SECURITY_NIFS_SEND_NUM(secScheme);
  ZW_DEBUG_SECURITY_NIFS_SEND_NUM(nodeSecure);
  ZW_DEBUG_SECURITY_NIFS_SEND_NL();

  /* Expire cached external nonce.
   * This disables the optimization of reusing the IV from previous message as
   * external nonce for the reply. This optimization is disabled for backward
   * compatibility reasons. */
  enNodeID = 0xff;
  cbReqFuncStatusCb = completedFunc;

  if(NULL != ZCB_pSetPowerDownTimeout)
  {
    ZCB_pSetPowerDownTimeout(SEC_SCHEMENONCE_POWERDOWNTIMEOUT);
  }

  if( (SEC_SCHEME_AUTO == secScheme) || (UNINITIALIZED_NODE == GetNodeSecure()) )
  {
    if( SECURE_NODE == GetNodeSecure())
    {
      secScheme = SEC_SCHEME_0;
    }
    else
    {
      secScheme = SEC_SCHEME_NO;
    }
  }

  ZW_DEBUG_SECURITY_NIFS_SEND_STR("\r\nTExt ");
  ZW_DEBUG_SECURITY_NIFS_SEND_NUM(tnodeID);
  ZW_DEBUG_SECURITY_NIFS_SEND_NUM(secScheme);
  ZW_DEBUG_SECURITY_NIFS_SEND_NUM(nodeSecure);
  ZW_DEBUG_SECURITY_NIFS_SEND_NL();

  if( SEC_SCHEME_NO == secScheme )
  {
    ZW_DEBUG_SECURITY_NIFS_SEND_BYTE('=');
    return ZW_SEND_DATA(tnodeID, (BYTE_P)pBufData, dataLength, txOptions, ZCB_ReqFuncStatusCb);
  }
  else
  {
    return ZW_SendDataSecure(tnodeID, (BYTE_P)pBufData, dataLength, txOptions, ZCB_ReqFuncStatusCb);
  }
}

/*========================   Transport_SendResponse   ============================
**      This function must be called instead of Transport_SendRequest, if
**      sending a RESPONSE, i.e. a solicited answer. The RESPONSE is sent with
**      the same security scheme as the REQUEST. This function must
**      be called directly after receiving the request.
**
**    Side effects :
**
**--------------------------------------------------------------------------*/
BYTE
Transport_SendResponse(
  BYTE tnodeID,
  ZW_APPLICATION_TX_BUFFER *pBufData,
  BYTE dataLength,
  BYTE txOptions,
  VOID_CALLBACKFUNC_BYTE completedFunc)
{
  cbResFuncStatusCb = completedFunc;

  ZW_DEBUG_SECURITY_NIFS_SEND_STR("\r\nTransport_SendResponse ");
  ZW_DEBUG_SECURITY_NIFS_SEND_NUM(tnodeID);
  ZW_DEBUG_SECURITY_NIFS_SEND_NL();

  //ZW_DEBUG_SEND_BYTE('@');
  /* Expire cached external nonce.
   * This disables the optimization of reusing the IV from previous message as
   * external nonce for the reply. This optimization is disabled for backward
   * compatibility reasons. */
  enNodeID = 0xff;

  if(NULL != ZCB_pSetPowerDownTimeout)
  {
    ZCB_pSetPowerDownTimeout(SEC_2_POWERDOWNTIMEOUT);
  }

  /*Check last RX-frame trigger response-frame*/
  if(lastRxFrame.cmdclass == pBufData->ZW_Common.cmdClass)
  {
    secureCmdHandling = lastRxFrame.secureCmdHandling;
  }

  if( (secureCmdHandling == TSEC_CMD_HANDLING_UNSECURE) || (UNINITIALIZED_NODE == GetNodeSecure()))
  {
    ZW_DEBUG_SECURITY_SEND_BYTE('=');
    return ZW_SEND_DATA(tnodeID, (BYTE_P)pBufData, dataLength, txOptions, ZCB_ResFuncStatusCb);
  }
  else
  {
    return ZW_SendDataSecure(tnodeID, (BYTE_P)pBufData, dataLength, txOptions, ZCB_ResFuncStatusCb);
  }
}

/*==============   Transport_OnApplicationInitHW   ============================
**      This function must be called in ApplicationInitHW
**
**    Side effects :
**
**--------------------------------------------------------------------------*/
BYTE                                      /* return TRUE on success */
Transport_OnApplicationInitHW(
  BYTE bStatus)                          /* bStatus */
{
  UNUSED(bStatus);
  Transport_wakeUpReason = bStatus;
  return TRUE;
}


/*==============   Transport_SetNodeSecure   ============================
**      This function must be called if changed nodeSecure
**
**    Side effects :
**
**--------------------------------------------------------------------------*/
void                                      /* return TRUE on success */
Transport_SetNodeSecure(
  BYTE vNodeSecure)
{
  ZW_DEBUG_SECURITY_NIFS_SEND_STR("\r\nTransport_SetNodeSecure ->");
  ZW_DEBUG_SECURITY_NIFS_SEND_NUM(vNodeSecure);
  ZW_DEBUG_SECURITY_NIFS_SEND_NL();

  nodeSecure = vNodeSecure;
  ZW_MEM_PUT_BYTE((WORD)&nvmApplDescriptor.EEOFFS_SECURITY.EEOFFS_NETWORK_SECURITY_field, nodeSecure);
  SetupActiveNIF();
}


/*============================ GetNodeSecure ===============================
** Function description
** This function...
**
** Side effects:
**
**-------------------------------------------------------------------------*/
BYTE
GetNodeSecure(void)
{
  return nodeSecure;
}

/*============================ LoadNodeSecure ===============================
** Function description
** This function...
**
** Side effects:
**
**-------------------------------------------------------------------------*/
void
LoadNodeSecure(void)
{
  nodeSecure = MemoryGetByte((WORD)&nvmApplDescriptor.EEOFFS_SECURITY.EEOFFS_NETWORK_SECURITY_field);
}

/*==============   Transport_OnApplicationInitSW   ============================
**      This function must be called in ApplicationInitSW
**
**    Side effects :
**
**--------------------------------------------------------------------------*/
BYTE                                      /* return TRUE on success */
Transport_OnApplicationInitSW(
  APP_NODE_INFORMATION* pAppNode,
  VOID_CALLBACKFUNC(pSetPowerDownTimeout)(BYTE))
{
  m_AppInfo.pNifs = pAppNode;
  ZCB_pSetPowerDownTimeout = pSetPowerDownTimeout;

#ifdef FLIRS
  if(ZW_WAKEUP_SENSOR == Transport_wakeUpReason)
  {
    ZCB_pSetPowerDownTimeout(SEC_2_POWERDOWNTIMEOUT);
  }
#endif

  //initialization:
  lastRxFrame.cmdclass = 0;
  lastRxFrame.secureCmdHandling = TSEC_CMD_HANDLING_DEFAULT;
  secureCmdHandling = TSEC_CMD_HANDLING_DEFAULT;
  scheme_get_received = FALSE;

  ZW_DEBUG_SECURITY_NIFS_SEND_STR("SecAppInit ");
  ZW_DEBUG_SECURITY_NIFS_SEND_WORD_NUM((WORD)&nvmApplDescriptor.EEOFFS_SECURITY.EEOFFS_MAGIC_BYTE_field);
  ZW_DEBUG_SECURITY_NIFS_SEND_STR(" ");
  ZW_DEBUG_SECURITY_NIFS_SEND_NUM(MemoryGetByte((WORD)&nvmApplDescriptor.EEOFFS_SECURITY.EEOFFS_MAGIC_BYTE_field));
  ZW_DEBUG_SECURITY_NIFS_SEND_NUM(MemoryGetByte((WORD)&nvmApplDescriptor.EEOFFS_SECURITY.EEOFFS_NETWORK_SECURITY_field));
  ZW_DEBUG_SECURITY_NIFS_SEND_NL();

  MemoryGetID(NULL, &bTransportNID);

  //eeprom handling:
  if ((MemoryGetByte((WORD)&nvmApplDescriptor.EEOFFS_SECURITY.EEOFFS_MAGIC_BYTE_field) != EEPROM_MAGIC_BYTE_VALUE))
  {
#ifdef ZW_CONTROLLER
    SECURITY_SCHEME = SECURITY;
    GetRNGData(networkKey, NETWORK_KEY_LENGTH);
#else
    memset(networkKey, 0, NETWORK_KEY_LENGTH);
#endif
    Transport_SetNodeSecure(UNINITIALIZED_NODE);
#ifdef ZW_CONTROLLER
    ZW_MEM_PUT_BYTE((WORD)&nvmApplDescriptor.EEOFFS_SECURITY.EEOFFS_SECURITY_SCHEME_field, SECURITY_SCHEME);
#endif
    ZW_MEM_PUT_BUFFER_NO_CB((WORD)&nvmApplDescriptor.EEOFFS_SECURITY.EEOFFS_NETWORK_KEY_START_field[0], NULL, NETWORK_KEY_LENGTH);
    ZW_MEM_PUT_BYTE((WORD)&nvmApplDescriptor.EEOFFS_SECURITY.EEOFFS_MAGIC_BYTE_field, EEPROM_MAGIC_BYTE_VALUE);
    //ZW_DEBUG_SECURITY_SEND_STR("\r\nSecResetNVM\r\n");
  }
  else
  {
#ifdef ZW_CONTROLLER
    SECURITY_SCHEME = MemoryGetByte((WORD)&nvmApplDescriptor.EEOFFS_SECURITY.EEOFFS_SECURITY_SCHEME_field);
#endif
    LoadNodeSecure();
    SetupActiveNIF();
    ZW_MEM_GET_BUFFER((WORD)&nvmApplDescriptor.EEOFFS_SECURITY.EEOFFS_NETWORK_KEY_START_field[0], networkKey, NETWORK_KEY_LENGTH);
  }

  //security init:
  InitSecurity(Transport_wakeUpReason);
  delayedAppCommandHandler.sourceNode = INVALID_NODE_ID;

  return TRUE;
}


/**
 * @brief See description for function prototype in ZW_basis_api.h.
 */
void
ApplicationNodeInformation(
  BYTE *deviceOptionsMask,
  APPL_NODE_TYPE *nodeType,
  BYTE **nodeParm,
  BYTE *parmLength)
{
  BYTE i, nodeListLen = 0;

  /* CHANGE THIS - Change the parameters in this function to the properties
     of your product. */
     /* this is a listening node And it support optional command classes*/
  *deviceOptionsMask  = m_AppInfo.pNifs->deviceOptionsMask;  //APPLICATION_NODEINFO_LISTENING | APPLICATION_NODEINFO_OPTIONAL_FUNCTIONALITY;
  nodeType->generic   = m_AppInfo.pNifs->nodeType.generic;  /* Generic device class */
  nodeType->specific  = m_AppInfo.pNifs->nodeType.specific; /* Specific device class */

  ZW_DEBUG_SECURITY_NIFS_SEND_NL();
  ZW_DEBUG_SECURITY_NIFS_SEND_STR("AppNIF ");
  ZW_DEBUG_SECURITY_NIFS_SEND_NUM(m_AppInfo.pNifs->cmdClassListNonSecureCount);
  ZW_DEBUG_SECURITY_NIFS_SEND_NUM(m_AppInfo.pNifs->cmdClassListNonSecureIncludedSecureCount);
  ZW_DEBUG_SECURITY_NIFS_SEND_NUM(m_AppInfo.pNifs->cmdClassListSecureCount);
  ZW_DEBUG_SECURITY_NIFS_SEND_NUM(m_AppInfo.cmdClassListNonSecureActiveCount);

  *nodeParm   = m_AppInfo.pCmdClassListNonSecureActive;
  *parmLength = m_AppInfo.cmdClassListNonSecureActiveCount;

  ZW_DEBUG_SECURITY_NIFS_SEND_STR(" length ");
  ZW_DEBUG_SECURITY_NIFS_SEND_NUM(*parmLength);
  ZW_DEBUG_SECURITY_NIFS_SEND_NL();
}


/*============================ Transport_SetDefault ===============================
** Function description
** Node is reset. Clear up security memory.
**
** Side effects:
**
**-------------------------------------------------------------------------*/
void
Transport_SetDefault()
{
  ZW_DEBUG_SECURITY_NIFS_SEND_STR("\r\nTransport_SetDefault");
  ZW_DEBUG_SECURITY_NIFS_SEND_NL();

  StopSecurityTimeOut();
  StopSecuritySendTimeOut();

  memset(networkKey, 0x00, NETWORK_KEY_LENGTH);
  SaveSecureNetworkKey();
  /* Go and make the Authentication and the encryption keys */
  LoadKeys();
  Transport_SetNodeSecure(UNINITIALIZED_NODE);
  scheme_get_received = FALSE;
  securityTimerHandle = 0;
}


/*==============   Transport_OnLearnCompleted   ============================
**      This function must be called in LearnCompleted application function
**       callback
**    Side effects :
**
**--------------------------------------------------------------------------*/
BYTE                                      /* return TRUE on success */
Transport_OnLearnCompleted(
  BYTE nodeID)                            /* IN resulting nodeID */
{

  ZW_DEBUG_SECURITY_NIFS_SEND_STR("\r\nTransport_OnLearnCompleted ->");
  ZW_DEBUG_SECURITY_NIFS_SEND_NUM(nodeID);
  ZW_DEBUG_SECURITY_NIFS_SEND_NL();
  if (nodeID == 0)    /* Was it reset, or did learn fail */
  {
  	StopSecurityTimeOut();
  	StopSecuritySendTimeOut();

    memset(networkKey, 0x00, NETWORK_KEY_LENGTH);
    SaveSecureNetworkKey();
    /* Go and make the Authentication and the encryption keys */
    LoadKeys();
    Transport_SetNodeSecure(UNINITIALIZED_NODE);
    securityTimerHandle = 0;
    scheme_get_received = FALSE;
  }
  else if ( NODE_BROADCAST != nodeID)
  {
    ZW_DEBUG_SECURITY_SEND_BYTE('m');
    ZW_DEBUG_SECURITY_SEND_NUM(nodeSecure);
    if (nodeSecure == UNINITIALIZED_NODE)
    {
      if(!securityTimerHandle)
      {
        StartSecurityTimeoutIfNotRunning(TIME_SECURITY_LIFE);
      }
    }
    else
    {
      if(!securityTimerHandle)
      {
        ZCB_FuncZWSecure(TRANSPORT_WORK_END);
      }
    }

  }
  return TRUE;
}

/* Postpone the call to Application Command Handler.
 * It will be delivered when the FSM goes idle. */
static void DelayApplicationCommandHandler(
  BYTE rxStatus,
  BYTE sourceNode,
  BYTE *pCmd,
  BYTE cmdLength)
{
  if (delayedAppCommandHandler.sourceNode == INVALID_NODE_ID)
  {
    if (cmdLength > APP_MAX_PAYLOAD)
    {
      ZW_DEBUG_SECURITY_SEND_STR("DelayApplicationCommandHandler: cmdLength too big\r\n");
      return;
    }
    memcpy(delayedAppCommandHandler.abCmd, pCmd, cmdLength);
    delayedAppCommandHandler.sourceNode = sourceNode;
    delayedAppCommandHandler.cmdLength = cmdLength;
    delayedAppCommandHandler.rxStatus = rxStatus;
  }
  else
  {
    ZW_DEBUG_SECURITY_SEND_STR("SecurityDelayAppCommandHandler: Buffer full\r\n");
  }
}

BOOL                             /*RET FALSE if nothing is queued, TRUE otherwise */
DeliverDelayedAppCmdHandler()
{
  if (delayedAppCommandHandler.sourceNode == INVALID_NODE_ID)
  {
    return FALSE;
  }
  ZW_DEBUG_SECURITY_SEND_STR("DeliverDelayedAppCmdHandler TRUE\r\n");
  /* Deliver a delayed app cmd handler to application */

  if(TRUE == CmdClassSupported( TRUE,
      ((ZW_APPLICATION_TX_BUFFER*)delayedAppCommandHandler.abCmd)->ZW_Common.cmdClass,
      m_AppInfo.pNifs->cmdClassListSecure,m_AppInfo.pNifs->cmdClassListSecureCount,
      m_AppInfo.pCmdClassListNonSecureActive, m_AppInfo.cmdClassListNonSecureActiveCount))
  {
    lastRxFrame.cmdclass = ((ZW_APPLICATION_TX_BUFFER*)delayedAppCommandHandler.abCmd)->ZW_Common.cmdClass;
    lastRxFrame.secureCmdHandling = TSEC_CMD_HANDLING_SECURE;     //message is secured, so reports also must be secured.
    Transport_ApplicationCommandHandler(delayedAppCommandHandler.rxStatus,
        delayedAppCommandHandler.sourceNode,
        (ZW_APPLICATION_TX_BUFFER *)&delayedAppCommandHandler.abCmd,
        delayedAppCommandHandler.cmdLength);
  }
  delayedAppCommandHandler.sourceNode = INVALID_NODE_ID;
  return TRUE;
}
/*============================ CommandClassSecurityVersionGet ===============
** Function description
** Return version
**
** Side effects:
**
**-------------------------------------------------------------------------*/
BYTE
CommandClassSecurityVersionGet(void)
{
  return SECURITY_VERSION;
}

/*============================ SetupActiveNIF ===============================
**-------------------------------------------------------------------------*/
void
SetupActiveNIF(void)
{
  ZW_DEBUG_SECURITY_NIFS_SEND_STR("\r\nSetupActiveNIF");
  ZW_DEBUG_SECURITY_NIFS_SEND_NL();
  if( SECURE_NODE != GetNodeSecure() )
  {
    ZW_DEBUG_SECURITY_NIFS_SEND_STR("\r\n NON-Secure");
    ZW_DEBUG_SECURITY_NIFS_SEND_NL();
    m_AppInfo.pCmdClassListNonSecureActive = m_AppInfo.pNifs->cmdClassListNonSecure;
    m_AppInfo.cmdClassListNonSecureActiveCount = m_AppInfo.pNifs->cmdClassListNonSecureCount;
  }
  else
  {
    ZW_DEBUG_SECURITY_NIFS_SEND_STR("\r\n NON-Secure IncludedSecure");
    ZW_DEBUG_SECURITY_NIFS_SEND_NL();
    m_AppInfo.pCmdClassListNonSecureActive = m_AppInfo.pNifs->cmdClassListNonSecureIncludedSecure;
    m_AppInfo.cmdClassListNonSecureActiveCount = m_AppInfo.pNifs->cmdClassListNonSecureIncludedSecureCount;
  }
  ZW_DEBUG_SECURITY_NIFS_SEND_STR("\r\n length");
  ZW_DEBUG_SECURITY_NIFS_SEND_NUM( m_AppInfo.cmdClassListNonSecureActiveCount);
  ZW_DEBUG_SECURITY_NIFS_SEND_NL();

}

