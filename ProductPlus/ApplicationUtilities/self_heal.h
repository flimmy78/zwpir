/****************************************************************************
 *
 * Copyright (c) 2001-2013
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
 *---------------------------------------------------------------------------
 *
 * Description: Implements functions that make is easy to support
 *              self-heal Operated Nodes
 *
 * Author:   Jonas Roum-Møller
 *
 * Last Changed By:  $Author: efh $
 * Revision:         $Revision: 24920 $
 * Last Changed:     $Date: 2013-03-06 15:43:02 +0100 (on, 06 mar 2013) $
 *
 ****************************************************************************/
#ifndef _SELF_HEAL_H_
#define _SELF_HEAL_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/
/* Recovery states */
#define HEAL_NONE                 0x00  /* No recovery operation in progress */
#define HEAL_SUC                  0x01  /* Next timeout - try talking to SUC if available */
#define HEAL_WAKEUPNODE           0x02  /* Next timeout - try talking to wakeupnode */
#define HEAL_GENERAL              0x03  /* Next timeout - try talking to SearchNodeID */

/* DEFAULT_LOST_COUNTER_MAX is used to determine when the node */
/* has lost the ability to communicate with it's wakeup */
/* notification node. The value specifies how many failed */
/* attempts is allowed, before the node starts yelling for help . */
#define DEFAULT_LOST_COUNTER_MAX      3

#define REDISCOVERY_TIMEOUT 100 /* 100 x 10ms */


/* Network Update timings */
/* Default 30 minutes between "Network Update Request"s */
#define DEFAULT_NETWORK_UPDATE_COUNT 7 // 30 //IZ:Fix TO# 02957
/* Minimum 30 minutes between "Network Update Request"s */
#define NETWORK_UPDATE_MIN_COUNT  DEFAULT_NETWORK_UPDATE_COUNT
/* Maximum 180 minutes between "Network Update Request"s */
#define NETWORK_UPDATE_MAX_COUNT  19 // 180 //IZ:Fix TO# 02957


/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

extern BYTE currentHealMode;
extern BYTE networkUpdateTimerHandle;

/* Data that must be maintained after powerdown */
extern XBYTE networkUpdateDownCount ;
extern XBYTE networkUpdateFailureCount;
extern XBYTE lostCount;

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/




/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/
void SetDefaultNetworkUpdateConfiguration(void);
void VerifyAssociatedTransmit( BYTE txStatus, BYTE attemptedNodeId );
BOOL VerifyLostCount(void);
void UpdateNetworkUpdateCount( BYTE reset );
extern void ZCB_UpdateNetworkUpdateCountOneMinute(void);
void HealComplete( BYTE success );
void AskNodeForHelp(BYTE NodeID);
void UpdateLostCounter(BYTE txStatus);
BYTE ZW_GetSUCNodeID( void );
void CancelRediscoveryTimer( void );

extern void ZCB_callbackAskNodeForHelp(BYTE txStatus);

extern void ZCB_callbackDelayNextAskNodeForHelp(void);

extern void ZCB_UpdateNetworkUpdateCountCallback(BYTE txStatus);

extern void ZCB_VerifyAssociatedTransmitCallback(BYTE SUCStatus);

#endif /*_SELF_HEAL_H_*/
