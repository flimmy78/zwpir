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
 *        All far variables (NVM offsets) should be defined in the application's eeprom.c module
 *        in the struct t_nvmApplDescriptor
 *
 * Author:   Valeriy Vyshnyak
 *
 * Last Changed By:  $Author: vvi $
 * Revision:         $Revision: 13417 $
 * Last Changed:     $Date: 2009-03-10 16:17:52 +0200 (Вв, 10 Бер 2009) $
 *
 ****************************************************************************/
#ifndef _TRANSPORT_SECURITY_H_
#define _TRANSPORT_SECURITY_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_Security_AES_module.h>
#include <ZW_Security_FSM.h>
#include <ZW_transport_api.h>
#include <ZW_basis_api.h>
/****************************************************************************/
/*                       PUBLIC TYPES and DEFINITIONS                       */
/****************************************************************************/


typedef struct _eeoffs_network_security_struct_
{
#ifdef ZW_CONTROLLER
  BYTE      EEOFFS_SECURITY_SCHEME_field;
#endif
  BYTE      EEOFFS_NETWORK_SECURITY_field;
  BYTE      EEOFFS_NETWORK_KEY_START_field[NETWORK_KEY_LENGTH];
//new eeprom variables add only before this magic byte variable (and don't forget to change offset of magic byte!!!)
  BYTE      EEOFFS_MAGIC_BYTE_field;
  BYTE      EEOFFS_NETWORK_SECURITY_RESERVED_field;  /* Deprecated field used to live here - is it okay to recycle it? */
} EEOFFS_NETWORK_SECURITY_STRUCT;

enum secure_node_status {
  UNINITIALIZED_NODE=0x00,
  SECURE_NODE=0x01,
  NON_SECURE_NODE=0x02
};


/****************************************************************************/
/*                              IMPORTED DATA                               */
/****************************************************************************/


/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/
typedef void (CODE *VOID_CALLBACKFUNC_BYTE)(BYTE txStatus);			/* callback function, which called by the transport layer when data is transmitted */
typedef void (CODE *TRANSPORT_STATUS_CALLBACK)(BYTE transportStatus);	/* callback function, which called by transport to inform application layer of its status */

/* Values of transportStatus argument of Transport status callback function: */
#define TRANSPORT_WORK_START		0x01	/* Transport layer begin the longly work operation. This status can be used to stop the power down timeout timer in battery operated devices */
#define TRANSPORT_WORK_END			0x02	/* Transport layer successfully end the longly work operation. This status can be used to start the power down timeout timer in battery operated devices */
#define TRANSPORT_WORK_ERROR		0x03	/* Transport layer abort the longly work operation. This status can be used to start the power down timeout timer in battery operated devices */


/* Extra txStatus codes that can be returned to callback func from
 * Transport_SendRequest() and Transport_SendResponse()
 * The status codes from ZW_SendData can also be returned. */
#define TRANSPORT_SECURITY_TIMEOUT 0x10  /* No reply from other node. If persistent, maybe poor network topology or failed node. */
#define TRANSPORT_SECURITY_BUSY 0x11   /* SendData buffer occupied, retry later */
#define TRANSPORT_SECURITY_MUTEX_LOCKED TRANSPORT_SECURITY_BUSY /* Retry later*/

#define SECURITY_SEND_DATA_TIME_OUT   20

/**
 * Number of seconds to wait for a nonce report before timing out. Timer starts when Nonce Get tx
 * complete callback arrives.
 *
 * We set to 2 seconds, because a last working route should be established by the nonce get, so no
 * routing delay is expected.
 *
 * TO#7284: Changed to 10 seconds.
 */
#define NONCE_GET_TIMEOUT_B 10

/* params used by ApplicationNodeInformation */
#define APPL_NODEPARM_MAX       35



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



/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/



/****************************************************************************/
/*                           IMPORTED FUNCTIONS                             */
/****************************************************************************/
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

/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/

/*========================   Transport_SendRequest   ============================
**      Send request command over secure network
**    Side effects :
**
** completedFunc(status)
** status:
** #define TRANSMIT_COMPLETE_OK      0x00
** #define TRANSMIT_COMPLETE_NO_ACK  0x01   retransmission error
** #define TRANSMIT_COMPLETE_FAIL    0x02   transmit error
** #define TRANSMIT_ROUTING_NOT_IDLE 0x03   transmit error
** #ifdef ZW_CONTROLLER
** #define TRANSMIT_COMPLETE_NOROUTE 0x04   no route found in assignroute
                                         therefore nothing was transmitted
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
  BYTE nodeID,
  ZW_APPLICATION_TX_BUFFER *pBufData,
  BYTE dataLength,
  BYTE txOptions,
  VOID_CALLBACKFUNC_BYTE completedFunc);

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
extern BYTE                                /* return TRUE on success */
Transport_OnLearnCompleted(
  BYTE nodeID)  ;                          /* IN resulting nodeID */


/*==============   Transport_SetNodeSecure   ============================
**      This function must be called if changed nodeSecure
**
**    Side effects :
**
**--------------------------------------------------------------------------*/
void                                      /* return TRUE on success */
Transport_SetNodeSecure(
  BYTE vNodeSecure);

/*==============   GetNodeSecure   ============================
**      Get nodeSecure state
**
**    Side effects :
**
**--------------------------------------------------------------------------*/
BYTE
GetNodeSecure(void);

extern void                      /* RET nothing */
ZCB_SecureSessionDone(
  BYTE bStatus, void *psTxResult);                 /* IN  status */

void
StartSecuritySendDataTimer(BYTE timeOut);

void StopNonceGetTimerB();

/**
 * @brief CommandClassSecurityVersionGet
 * get version
 * @return version
 */
BYTE
CommandClassSecurityVersionGet(void);

/* Call from ZW_Security_AES module when FSM goes idle.
 * This will deliver a delayed App Cmd handler to the
 * application. */
BOOL     /* RET FALSE if nothing is queued, TRUE otherwise. */
DeliverDelayedAppCmdHandler();


#endif /*_TRANSPORT_SECURITY_H_*/
