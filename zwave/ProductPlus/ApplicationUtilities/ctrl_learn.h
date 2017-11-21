/****************************************************************************
 *
 * Copyright (c) 2001-2013
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
 *---------------------------------------------------------------------------
 *
 * Description: This file contains a sample of how learn mode could be implemented
 *              on ZW0201 standard controllers.
 *              The module works for both battery operated and always listening
 *              devices.
 *
 * Author:   Henrik Holm
 *
 * Last Changed By:  $Author: efh $
 * Revision:         $Revision: 24920 $
 * Last Changed:     $Date: 2013-03-06 16:43:02 +0200 (Ср, 06 мар 2013) $
 *
 ****************************************************************************/
#ifndef _CTRL_LEARN_H_
#define _CTRL_LEARN_H_
/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/

/*============================   LearnCompleted   ===========================
**    Function description
**      Should be implemented by the Application.
**      Called when nodeID have been assigned or deleted.
**    Side effects:
**
**--------------------------------------------------------------------------*/
extern void LearnCompleted(
BYTE nodeID,                  /*IN The nodeID assigned*/
BYTE bStatus);                /*IN The status of the learn TRUE = SUCCESS, FALSE = FAILURE */

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/
extern BOOL learnInProgress;        /*Application can use this flag to check if learn
                                  mode is active*/
/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/

/*============================   StartLearnModeNow   ======================
**    Function description
**      Call this function whenever learnmode should be entered.
**      This function does the following:
**        - Set the controller in Learnmode
**        - Starts a one second timeout after which learn mode is disabled
**        - learnState will be TRUE until learnmode is done.
**      If the Controller is added or removed to/from a network the function
**      LearnCompleted will be called.
**    Side effects:
**
**--------------------------------------------------------------------------*/
extern void StartLearnModeNow(BYTE bMode);

/*============================   StopLearnModeNow   ======================
**    Function description
**      Call this function from the application whenever learnmode
**      should be disabled.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
extern BYTE StopLearnModeNow();

/*==========================   ReArmLearnModeTimeout   =======================
**    Function description
**      Rearms the LearnMode timout handler and thereby extending the time
**      that the controller are to be in LearnMode/Receive.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ReArmLearnModeTimeout();

extern void ZCB_EndLearnNodeState(void);

extern void ZCB_LearnModeCompleted(LEARN_INFO *glearnNodeInfo);

extern void ZCB_SendExplorerRequest(void);

#endif /*_CTRL_LEARN_H_*/
