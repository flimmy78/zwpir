/***************************************************************************
*
* Copyright (c) 2001-2013
* Sigma Designs, Inc.
* All Rights Reserved
*
*---------------------------------------------------------------------------
*
* Description: Implements functions for transporting frames over the
*              native Z-Wave Network
*
* Author: Thomas Roll
*
* Last Changed By: $Author: tro $
* Revision: $Revision: 0.00 $
* Last Changed: $Date: 2013/08/26 10:47:09 $
*
****************************************************************************/

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_TransportNative.h>
#include <ZW_TransportLayer.h>
#include <ZW_transport_api.h>
#include <ZW_uart_api.h>
#include <ZW_TransportLayer.h>
#include "config_app.h"
#include <misc.h>
/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/
#ifdef ZW_DEBUG_NATIVE
#define ZW_DEBUG_NATIVE_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_NATIVE_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_NATIVE_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_NATIVE_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_NATIVE_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_NATIVE_SEND_BYTE(data)
#define ZW_DEBUG_NATIVE_SEND_STR(STR)
#define ZW_DEBUG_NATIVE_SEND_NUM(data)
#define ZW_DEBUG_NATIVE_SEND_WORD_NUM(data)
#define ZW_DEBUG_NATIVE_SEND_NL()
#endif


#ifndef SEC_2_POWERDOWNTIMEOUT
#define SEC_2_POWERDOWNTIMEOUT 20
#endif

#ifndef SEC_SCHEMENONCE_POWERDOWNTIMEOUT
#define SEC_SCHEMENONCE_POWERDOWNTIMEOUT 100
#endif

#ifndef MSEC_200_POWERDOWNTIMEOUT
#define MSEC_200_POWERDOWNTIMEOUT 2
#endif

#define SECURITY_KEY_NONE_MASK 0x00


typedef struct _ST_TRANSPORT_NODE_INFORMATION_
{
  APP_NODE_INFORMATION *pNifs;
  BYTE *pCmdClassListNonSecureActive;            /* Nonsecure-list of supported command classes, when node communicate by this transport */
  BYTE cmdClassListNonSecureActiveCount;        /* Count of elements in supported command classes Nonsecure-list */
} TRANSPORT_NODE_INFORMATION;

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/
VOID_CALLBACKFUNC(cbReqFuncStatusCb)(BYTE txStatus) = NULL;
VOID_CALLBACKFUNC(cbResFuncStatusCb)(BYTE txStatus) = NULL;

static TRANSPORT_NODE_INFORMATION m_AppInfo = {NULL,NULL,0};

BYTE Transport_wakeUpReason;
/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/
void ZCB_ReqFuncStatusCb(BYTE txStatus);
void ZCB_ResFuncStatusCb(BYTE txStatus);
VOID_CALLBACKFUNC(ZCB_pSetPowerDownTimeout)(BYTE);

/*==============   Transport_OnApplicationInitHW   ============================
**      This function must be called in ApplicationInitHW
**
**    Side effects :
**
**--------------------------------------------------------------------------*/
BYTE                                     /* return TRUE on success */
Transport_OnApplicationInitHW(
  BYTE bStatus)                          /* bStatus */
{
  Transport_wakeUpReason = bStatus;
  return TRUE;
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
  ZW_DEBUG_NATIVE_SEND_STR("Transport_OnApplicationInitSW\r\n ");
  m_AppInfo.pNifs = pAppNode;
  ZCB_pSetPowerDownTimeout = pSetPowerDownTimeout;

#ifdef FLIRS
  if(ZW_WAKEUP_SENSOR == Transport_wakeUpReason)
  {
    ZCB_pSetPowerDownTimeout(SEC_2_POWERDOWNTIMEOUT);
  }
#endif

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
  ZW_DEBUG_NATIVE_SEND_STR("AppNIF ");
  ZW_DEBUG_NATIVE_SEND_NUM(m_AppInfo.pNifs->cmdClassListNonSecureCount);
  ZW_DEBUG_NATIVE_SEND_NUM(m_AppInfo.pNifs->cmdClassListNonSecureIncludedSecureCount);
  ZW_DEBUG_NATIVE_SEND_NUM(m_AppInfo.pNifs->cmdClassListSecureCount);
  ZW_DEBUG_NATIVE_SEND_NUM(m_AppInfo.cmdClassListNonSecureActiveCount);

  *deviceOptionsMask  = m_AppInfo.pNifs->deviceOptionsMask;  //APPLICATION_NODEINFO_LISTENING | APPLICATION_NODEINFO_OPTIONAL_FUNCTIONALITY;
  nodeType->generic   = m_AppInfo.pNifs->nodeType.generic;  /* Generic device class */
  nodeType->specific  = m_AppInfo.pNifs->nodeType.specific; /* Specific device class */

  *nodeParm = m_AppInfo.pNifs->cmdClassListNonSecure;
  *parmLength = m_AppInfo.pNifs->cmdClassListNonSecureCount;

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

}

/*==============   Transport_OnLearnCompleted   ============================
**      This function must be called in LearnCompleted application function
**       callback
**    Side effects :
**
**--------------------------------------------------------------------------*/
BYTE                                      /* return TRUE on success */
Transport_OnLearnCompleted(
  BYTE nodeID)                             /* IN resulting nodeID */
{
  UNUSED(nodeID);
  if(NULL != ZCB_pSetPowerDownTimeout)
  {
    ZCB_pSetPowerDownTimeout(MSEC_200_POWERDOWNTIMEOUT);
  }
  return TRUE;
}


/*===========   Transport_ApplicationCommandHandler   ======================
**      Called, when frame is received
**    Side effects :
**
**--------------------------------------------------------------------------*/
void
ApplicationCommandHandler(
    BYTE  rxStatus,                 /* IN Frame header info */
    BYTE  sourceNode,               /* IN Command sender Node ID */
    ZW_APPLICATION_TX_BUFFER *pCmd, /* IN Payload from the received frame, the union */
                                    /*    should be used to access the fields */
    BYTE   cmdLength                /* IN Number of command bytes including the command */
  )
{
  ZW_DEBUG_NATIVE_SEND_STR("NATIVE ApplCmdH Cmd ");
  ZW_DEBUG_NATIVE_SEND_NUM(pCmd->ZW_Common.cmdClass);
  ZW_DEBUG_NATIVE_SEND_NL();

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


/*========================   Transport_SendRequest   ============================
**      Send request command over secure network
**    Side effects :
**
**--------------------------------------------------------------------------*/
BYTE
Transport_SendRequest(
  BYTE nodeID,
  ZW_APPLICATION_TX_BUFFER *pBufData,
  BYTE dataLength,
  BYTE txOptions,
  VOID_CALLBACKFUNC_BYTE completedFunc,
  BYTE isForcedNonSencure)                     /* FALSE: send dependend of connection secure level. TRUE: send nonsecure */
{
  UNUSED(isForcedNonSencure);
  cbReqFuncStatusCb = completedFunc;

  ZW_DEBUG_NATIVE_SEND_STR("NATIVE SendRequest Cmd ");
  ZW_DEBUG_NATIVE_SEND_NUM(pBufData->ZW_Common.cmdClass);
  ZW_DEBUG_NATIVE_SEND_NL();
  return ZW_SEND_DATA(nodeID, (BYTE_P)pBufData, dataLength, txOptions, ZCB_ReqFuncStatusCb);
}


/*========================   Transport_SendResponse   ============================
**      This function must be called instead of Transport_SendRequest, if report
**      frame is sent.
**    Side effects :
**
**--------------------------------------------------------------------------*/
BYTE
Transport_SendResponse(
  BYTE nodeID,
  ZW_APPLICATION_TX_BUFFER *pBufData,
  BYTE dataLength,
  BYTE txOptions,
  //VOID_CALLBACKFUNC_BYTE completedFunc)
  VOID_CALLBACKFUNC(completedFunc)(BYTE))
{
  cbResFuncStatusCb = completedFunc;
  ZW_DEBUG_NATIVE_SEND_STR("NATIVE SendResponse Cmd ");
  ZW_DEBUG_NATIVE_SEND_NUM(pBufData->ZW_Common.cmdClass);
  ZW_DEBUG_NATIVE_SEND_NL();
  return ZW_SEND_DATA(nodeID, (BYTE_P)pBufData, dataLength, txOptions, ZCB_ResFuncStatusCb);
}

/**
 * Callback for requests.
 */
PCB(ZCB_ReqFuncStatusCb)(BYTE txStatus)
{
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

/**
 * Callback for responses.
 */
PCB(ZCB_ResFuncStatusCb)(BYTE txStatus)
{
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
