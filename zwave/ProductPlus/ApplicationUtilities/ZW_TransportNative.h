/******************************* ZW_TransportNative.h *******************************
 *           #######
 *           ##  ##
 *           #  ##    ####   #####    #####  ##  ##   #####
 *             ##    ##  ##  ##  ##  ##      ##  ##  ##
 *            ##  #  ######  ##  ##   ####   ##  ##   ####
 *           ##  ##  ##      ##  ##      ##   #####      ##
 *          #######   ####   ##  ##  #####       ##  #####
 *                                           #####
 *          Z-Wave, the wireless language.
 *
 *              Copyright (c) 2009
 *              Zensys A/S
 *              Denmark
 *
 *              All Rights Reserved
 *
 *    This source file is subject to the terms and conditions of the
 *    Zensys Software License Agreement which restricts the manner
 *    in which it may be used.
 *
 *---------------------------------------------------------------------------
 *
 * Description: Implements functions for transporting frames over the
 *               native Z-Wave Network.
 *
 * Author:   Valeriy Vyshnyak
 *
 * Last Changed By:  $Author: vvi $
 * Revision:         $Revision: 13417 $
 * Last Changed:     $Date: 2009-03-10 16:17:52 +0200 (Вв, 10 Бер 2009) $
 *
 ****************************************************************************/
#ifndef _TRANSPORT_NATIVE_H_
#define _TRANSPORT_NATIVE_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_typedefs.h>
#include <ZW_classcmd.h>
#include <ZW_basis_api.h>
/****************************************************************************/
/*                              IMPORTED DATA                               */
/****************************************************************************/

/**
 * Structure holding information about node lists, device option mask and node type.
 */
typedef struct _APP_NODE_INFORMATION_
{
  BYTE *cmdClassListNonSecure;            /**< Nonsecure-list of supported command classes, when node communicate by this transport */
  BYTE cmdClassListNonSecureCount;        /**< Count of elements in supported command classes Nonsecure-list */
  BYTE *cmdClassListNonSecureIncludedSecure; /**< Nonsecure-list of supported command classes, when node communicate by this transport */
  BYTE cmdClassListNonSecureIncludedSecureCount;  /**< Count of elements in supported command classes Nonsecure-list */
  BYTE *cmdClassListSecure;               /**< Secure-list of supported command classes, when node communicate by this transport */
  BYTE cmdClassListSecureCount;           /**< Count of elements in supported command classes Secure-list */
  BYTE deviceOptionsMask; /**< See ZW_basic_api.h for ApplicationNodeInformation field deviceOptionMask */
  APPL_NODE_TYPE nodeType;
} APP_NODE_INFORMATION;


enum secure_node_status {
  UNINITIALIZED_NODE=0x00,
  SECURE_NODE=0x01,
  NON_SECURE_NODE=0x02
};

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/
#define TRANSPORT_EEPROM_SETTINGS_SIZE  0
typedef void (CODE *VOID_CALLBACKFUNC_BYTE)(BYTE txStatus);			/* callback function, which called by the transport layer when data is transmitted */
typedef void (CODE *TRANSPORT_STATUS_CALLBACK)(BYTE transportStatus);	/* callback function, which called by transport to inform application layer of its status */

/****************************************************************************/
/*                           FUNCTIONS                                      */
/****************************************************************************/


/*==============   Transport_OnApplicationInitHW   ============================
**      This function must be called in ApplicationInitHW
**
**    Side effects :
**
**--------------------------------------------------------------------------*/
BYTE                               /* return TRUE on success */
Transport_OnApplicationInitHW(
  BYTE bStatus);                          /* bStatus */


/**
 * @brief Initializes the framework transport layer. It must be called at the end of
 * ApplicationInitSW() in the application.
 * @param pAppNode Pointer to a struct holding information about the node.
 * @param requestedSecurityKeyBits A bitmask based on values defined in ZW_security_api.h.
 * @param pSetPowerDownTimeout Callback function pointer to a function which postpones sleep. Only
 * relevant for battery powered devices. If mains powered, set to NULL.
 * @return TRUE if transport layer is initialized, FALSE otherwise.
 */
BYTE
Transport_OnApplicationInitSW(
  APP_NODE_INFORMATION* pAppNode,
  VOID_CALLBACKFUNC(pSetPowerDownTimeout)(BYTE));


/*============================ Transport_SetDefault ===============================
** Function description
** Node is reset. Clear up security memory.
**
** Side effects:
**
**-------------------------------------------------------------------------*/
void
Transport_SetDefault();

/*==============   Transport_OnLearnCompleted   ============================
**      This function must be called in LearnCompleted application function
**       callback
**    Side effects :
**
**--------------------------------------------------------------------------*/
BYTE                                /* return TRUE on success */
Transport_OnLearnCompleted(
  BYTE nodeID)  ;                          /* IN resulting nodeID */
//#define Transport_OnLearnCompleted(nodeID)

/*===========   Transport_ApplicationCommandHandler   ======================
**      Called, when frame is received
**    Side effects :
**
**--------------------------------------------------------------------------*/
extern void
Transport_ApplicationCommandHandler(
  BYTE  rxStatus,                 /* IN Frame header info */
  BYTE  sourceNode,               /* IN Command sender Node ID */
  ZW_APPLICATION_TX_BUFFER *pCmd, /* IN Payload from the received frame, the union */
                                  /*    should be used to access the fields */
  BYTE   cmdLength                /* IN Number of command bytes including the command */
);


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
  BYTE isForcedNonSencure);                     /* FALSE: send dependend of connection secure level. TRUE: send nonsecure */

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
  VOID_CALLBACKFUNC(completedFunc)(BYTE));
//#define Transport_SendResponse(tnodeID, pBufData, dataLength, txOptions, completedFunc) ZW_SEND_DATA(tnodeID, pBufData, dataLength, txOptions, completedFunc)

/**
 * @brief CommandClassSecurityVersionGet
 * get version
 * @return version
 */
#define CommandClassSecurityVersionGet() 0

/*==============   GetNodeSecure   ============================
**      Get nodeSecure state
**
**    Side effects :
**
**--------------------------------------------------------------------------*/
#define GetNodeSecure() NON_SECURE_NODE

#endif /*_TRANSPORT_NATIVE_H_*/
