/******************************* slave_learn.c *******************************
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
 *              Copyright (c) 2001
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
 * Description: This file contains a sample of how learn mode could be implemented
 *              on ZW0102 standard slave, routing slave and enhanced slave devices.
 *              The module works for both battery operated and always listening
 *              devices.
 *
 * Author:   Henrik Holm
 *
 * Last Changed By:  $$
 * Revision:         $$
 * Last Changed:     $$
 *
 ****************************************************************************/

#include "config_app.h"

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#ifdef ZW_CONTROLLER
#include <ZW_controller_api.h>
#else
#include <ZW_slave_api.h>
#endif
#include <ZW_uart_api.h>
#include <slave_learn.h>
#include <ZW_TransportLayer.h>
#include <misc.h>
/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

#ifdef ZW_DEBUG_LEARNPLUS
#define ZW_DEBUG_LEARNPLUS_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_LEARNPLUS_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_LEARNPLUS_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_LEARNPLUS_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_LEARNPLUS_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_LEARNPLUS_SEND_BYTE(data)
#define ZW_DEBUG_LEARNPLUS_SEND_STR(STR)
#define ZW_DEBUG_LEARNPLUS_SEND_NUM(data)
#define ZW_DEBUG_LEARNPLUS_SEND_WORD_NUM(data)
#define ZW_DEBUG_LEARNPLUS_SEND_NL()
#endif


#define ZW_LEARN_NODE_STATE_TIMEOUT 100   /* The base learn timer timeout value */

#define LEARN_MODE_CLASSIC_TIMEOUT      2   /* Timeout count for classic innlusion */
#define LEARN_MODE_NWI_TIMEOUT          4  /* Timeout count for network wide innlusion */


#define STATE_LEARN_IDLE     0
#define STATE_LEARN_STOP     1
#define STATE_LEARN_CLASSIC  2
#define STATE_LEARN_NWI      3


typedef enum { NW_NONE, NW_INCLUSION, NW_EXCLUSION} NW_ACTION;
/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/
static BYTE learnStateHandle = 0xFF;
static BYTE bInclusionTimeoutCount;
static BYTE bIncReqCount;
static BYTE bIncReqCountSave = 4;

#ifdef NOT_USED
/* Function pointer to WakeUpSet function so this module can work
   with both battery and non-battery products.
   Is NULL in non-battery nodes. */
static VOID_CALLBACKFUNC(pWakeUpStateSet)(BYTE) = NULL;
#endif

static BOOL nodeInfoTransmitDone  = TRUE;

static BYTE learnState = STATE_LEARN_IDLE;   /*Application can use this flag to check if learn mode is active*/

static NW_ACTION NW_Action = NW_NONE;

/* Remember the nodeid after inclusion to pass it on when secure
   inclusion finishes. Do NOT use for any other purpose.
   Is only valid */
static BYTE bCachedNodeID = 0xFF;


static void HandleLearnState(void);
void ZCB_LearnNodeStateTimeout(void);
#ifdef ZW_CONTROLLER
void ZCB_LearnModeCompleted(
  LEARN_INFO* psLearnIfo);
#else
void ZCB_LearnModeCompleted(
  BYTE bStatus,         /* IN Current status of Learnmode*/
  BYTE nodeID);         /* IN resulting nodeID */
#endif

void ZCB_TransmitNodeInfoComplete(BYTE bTXStatus, TX_STATUS_TYPE *txStatusReport);

/****************************************************************************/
/*                               PROTOTYPES                                 */
/****************************************************************************/


/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/

#ifdef ZW_CONTROLLER
code const void (code * ZCB_LearnModeCompleted_p)(LEARN_INFO* psLearnInfo) = &ZCB_LearnModeCompleted;
/*==========================   ZCB_LearnModeCompleted   ======================
**    Function description
**      Callback which is called on learnmode completes
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void                  /*RET Nothing */
ZCB_LearnModeCompleted(
  LEARN_INFO* psLearnInfo)
#else
#ifdef NOT_USED
code const void (code * ZCB_LearnModeCompleted_p)(BYTE bStatus, BYTE nodeID) = &ZCB_LearnModeCompleted;
/*============================   LearnModeCompleted   ========================
**    Function description
**      Callback which is called on learnmode completes
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void                  /*RET Nothing */
#endif
PCB(ZCB_LearnModeCompleted)(
  BYTE bStatus,         /* IN Current status of Learnmode*/
  BYTE nodeID)          /* IN resulting nodeID */
#endif
{
  ZW_DEBUG_LEARNPLUS_SEND_STR("\r\n**ZCB_LearnModeCompleted");
  ZW_DEBUG_LEARNPLUS_SEND_NUM(bStatus);
  ZW_DEBUG_LEARNPLUS_SEND_NUM(nodeID);
  ZW_DEBUG_LEARNPLUS_SEND_NL();
  /* Learning in progress. Protocol will do timeout for us */
  if (learnStateHandle != 0xff)
  {
    TimerCancel(learnStateHandle);
    learnStateHandle = 0xff;
  }

#ifdef ZW_CONTROLLER
#else
  if (ASSIGN_NODEID_DONE == bStatus)
  {
    bCachedNodeID = nodeID;
  }
  if (bStatus == ASSIGN_RANGE_INFO_UPDATE)
  {
    nodeInfoTransmitDone = FALSE;
  }
  else
  {
    nodeInfoTransmitDone = TRUE;
  }
  if (bStatus == ASSIGN_COMPLETE)
  {
    /* Assignment was complete. Tell application */
    if (learnState)
    {
      learnState = STATE_LEARN_STOP;
      HandleLearnState();
    }
    LearnCompleted(nodeID);
  }
#endif
}

#ifdef NOT_USED
code const void (code * ZCB_TransmitNodeInfoComplete_p)(BYTE bTXStatus, TX_STATUS_TYPE *) = &ZCB_TransmitNodeInfoComplete;
/*========================   TransmitNodeInfoComplete   ======================
**    Function description
**      Callbackfunction called when the nodeinformation frame has
**      been transmitted. This function ensures that the Transmit Queue is not
**      flooded.
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
#endif
PCB(ZCB_TransmitNodeInfoComplete)(
  BYTE bTXStatus,
  TX_STATUS_TYPE *txStatusReport
)
{
  UNUSED(bTXStatus);
  UNUSED(txStatusReport);
  nodeInfoTransmitDone = TRUE;
}

#ifdef NOT_USED
code const void (code * ZCB_LearnNodeStateTimeout_p)(void) = &ZCB_LearnNodeStateTimeout;
/*============================   EndLearnNodeState   ========================
**    Function description
**      Timeout function that stop a learn mode
**      if we are in classic mode then switch to NWI else stop learn process
**      Should not be called directly.
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
#endif
PCB(ZCB_LearnNodeStateTimeout)(void)
{

  if (!(--bInclusionTimeoutCount))
  {
    ZW_DEBUG_LEARNPLUS_SEND_NL();
    ZW_DEBUG_LEARNPLUS_SEND_STR("ZCB_LearnNodeStateTimeout learnStateHandle");
    ZW_DEBUG_LEARNPLUS_SEND_NUM(learnStateHandle);
    ZW_DEBUG_LEARNPLUS_SEND_STR(" learnState ");
    ZW_DEBUG_LEARNPLUS_SEND_NUM(learnState);
    ZW_DEBUG_LEARNPLUS_SEND_STR(" NW_Action ");
    ZW_DEBUG_LEARNPLUS_SEND_NUM(NW_Action);
    ZW_DEBUG_LEARNPLUS_SEND_NL();
    if (learnStateHandle != 0xff)
    {
      TimerCancel(learnStateHandle);
      learnStateHandle = 0xff;
    }
    if (learnState == STATE_LEARN_CLASSIC)
    {
      ZW_SetLearnMode(ZW_SET_LEARN_MODE_DISABLE, NULL);
      learnState = STATE_LEARN_NWI;
      HandleLearnState();
    }
    else if (learnState == STATE_LEARN_NWI)
    {
      ZW_DEBUG_LEARNPLUS_SEND_BYTE('R');
      if (bIncReqCount)
      {
        if(NW_Action == NW_INCLUSION)
        {
          ZW_ExploreRequestInclusion();
        }
        else if(NW_Action == NW_EXCLUSION)
        {
          ZW_ExploreRequestExclusion();
        }

        bIncReqCount--;

      /* Start timer sending  out a explore inclusion request after 4 + random sec */
        bInclusionTimeoutCount = bIncReqCount? (LEARN_MODE_NWI_TIMEOUT + (ZW_Random() & 0x07)):LEARN_MODE_NWI_TIMEOUT;
        learnStateHandle = TimerStart(ZCB_LearnNodeStateTimeout,
                                      ZW_LEARN_NODE_STATE_TIMEOUT,
                                      TIMER_FOREVER);
      }
      else
      {
        learnState = STATE_LEARN_STOP;
        HandleLearnState();
        /*return nodeID 0xFF if the learn process timeout*/
        LearnCompleted(0xFF);
      }

    }
  }
}


/*============================   StartLearnInternal   ======================
**    Function description
**      Call this function from the application whenever learnmode
**      should be enabled.
**      This function do the following:
**        - Set the Slave in Learnmode
**        - Starts a one second timeout after which learn mode is disabled
**        - Broadcast the NODEINFORMATION frame once when called.
**      LearnCompleted will be called if a controller performs an assignID.
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void
HandleLearnState(void)
{
  ZW_DEBUG_LEARNPLUS_SEND_NL();
  ZW_DEBUG_LEARNPLUS_SEND_STR("HandleLearnState learnState ");
  ZW_DEBUG_LEARNPLUS_SEND_NUM(learnState);
  ZW_DEBUG_LEARNPLUS_SEND_STR(" bInclusionTimeoutCount ");
  ZW_DEBUG_LEARNPLUS_SEND_NUM(bInclusionTimeoutCount);
  ZW_DEBUG_LEARNPLUS_SEND_NL();
  if (learnState == STATE_LEARN_CLASSIC)
  {
    ZW_SetLearnMode(ZW_SET_LEARN_MODE_CLASSIC, ZCB_LearnModeCompleted);
    if (nodeInfoTransmitDone)
    {
      if (ZW_SendNodeInformation(NODE_BROADCAST, 0, ZCB_TransmitNodeInfoComplete))
      {
        nodeInfoTransmitDone = FALSE;
      }
    }
    /*Disable Learn mode after 2 sec.*/
    bInclusionTimeoutCount = LEARN_MODE_CLASSIC_TIMEOUT;
    learnStateHandle = TimerStart(ZCB_LearnNodeStateTimeout, ZW_LEARN_NODE_STATE_TIMEOUT, TIMER_FOREVER);
  }
  else if (learnState == STATE_LEARN_NWI)
  {
    if(NW_EXCLUSION == NW_Action)
    {
      ZW_SetLearnMode(ZW_SET_LEARN_MODE_NWE, ZCB_LearnModeCompleted);
      bInclusionTimeoutCount = 1;
      ZCB_LearnNodeStateTimeout();
    }
    else
    {
      ZW_SetLearnMode(ZW_SET_LEARN_MODE_NWI, ZCB_LearnModeCompleted);
      bInclusionTimeoutCount = 1;
      ZCB_LearnNodeStateTimeout();
    }
  }
  else if (learnState == STATE_LEARN_STOP)
  {
    ZW_SetLearnMode(ZW_SET_LEARN_MODE_DISABLE, NULL);
    if (learnStateHandle != 0xff)
    {
      TimerCancel(learnStateHandle);
      learnStateHandle = 0xff;
    }

    learnState = STATE_LEARN_IDLE;
  }
}


/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/

void
StartLearnModeNow(LEARN_MODE_ACTION bMode)
{

  if (LEARN_MODE_DISABLE != bMode)
  {
#ifdef NOT_USED
    /* Stay awake until inclusion (and security and wakeup configuraion) is complete */
    if (NON_NULL( pWakeUpStateSet ))
    {
      pWakeUpStateSet(TRUE);
    }
#endif

    NW_Action = NW_NONE;
    if (learnState != STATE_LEARN_IDLE) /* Learn mode is started, stop it */
    {
      learnState = STATE_LEARN_STOP;
      HandleLearnState();
    }
    if (bMode == LEARN_MODE_INCLUSION)
    {
      bIncReqCount = bIncReqCountSave;
      NW_Action = NW_INCLUSION;
    }
    else if(LEARN_MODE_EXCLUSION_NWE == bMode)
    {
      bIncReqCount = bIncReqCountSave;
      NW_Action = NW_EXCLUSION;
    }
    else
    {
      bIncReqCount = 0;
    }
    learnState = STATE_LEARN_CLASSIC;
    HandleLearnState();
  }
  else
  {
    NW_Action = NW_NONE;
    learnState = STATE_LEARN_STOP;
    HandleLearnState();
  }
}
