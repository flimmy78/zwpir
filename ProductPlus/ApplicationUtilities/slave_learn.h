/****************************************************************************
 *
 * Copyright (c) 2001-2013
 * Sigma Designs, Inc.
 * All Rights Reserved
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
 * Last Changed By:  $Author: efh $
 * Revision:         $Revision: 24920 $
 * Last Changed:     $Date: 2013-03-06 15:43:02 +0100 (on, 06 mar 2013) $
 *
 ****************************************************************************/
#ifndef _SLAVE_LEARN_H_
#define _SLAVE_LEARN_H_
/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/
#define LEARN_MODE_INCLUSION 1  //Enable the learn process to do an inclusion
#define LEARN_MODE_EXCLUSION 2  //Enable the learn process to do an exclusion
#define LEARN_MODE_DISABLE   0  //Disable learn process

/*============================   LearnCompleted   ===========================
**    Function description
**      Should be implemented by the Application.
**      Called when nodeID have been assigned , deleted or the learn
**      times out
**      nodeID parameter is 0xFF if the learn process times out
**    Side effects:
**
**--------------------------------------------------------------------------*/
extern void LearnCompleted(
BYTE nodeID);                 /*IN The nodeID assigned*/


/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/

/*============================   StartLearnModeNow   ======================
**    Function description
**      Call this function from the application whenever learnmode
**      should be enabled / Disabled.
**      This function do the following:
**        If the node is not included in network 
**          Set the Slave in classic Learnmode
**          Starts a two seconds timeout after which we switch to NWI mode
**          Broadcast the NODEINFORMATION frame once when called.
**          If classic learn mode timeout start NWI learn mode
**          if bInclusionReqCount > 1 send explorer inclusion frame
**            start a 4 + random time timer
**          if bInclusionReqCount == 1 send explorer inclusion request frame and wait 4 seconds
**          when timer timeout and bInclusionReqCount == 0 disable NWI mode and call LearnCompleted
**        if node is not included in a network
**          Set the Slave in classic Learnmode
**          Starts a two seconds timeout after which we stop learn mode
**
**       LearnCompleted will be also called after the end of learn process or a timeout
**        if LearnComplete called due timeout out the nodeID parameter would be 0xFF
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
StartLearnModeNow(BYTE bMode); /* The mode of the learn process
                                 LEARN_MODE_INCLUSION   Enable the learn mode to do an inclusion
                                 LEARN_MODE_EXCLUSION   Enable the learn mode to do an exclusion
                                 LEARN_MODE_DISABLE      Disable learn mode
                                */
/*===========================   SetInclusionRequestCount   =======================
**    Function description
**      Set the number of timer we send the explorer inclusion request frame
**      if thsi function is not set then the default value is 4.
**
**    Side effects: None
**
**--------------------------------------------------------------------------------*/
void SetInclusionRequestCount(BYTE bInclusionRequestCount);

/*===========================   GetLearnModeState   =======================
**    Function description
**      Check if the learning mode is active**    Side effects: None
**
**--------------------------------------------------------------------------------*/
BOOL                    /*RET TRUE if the learning mode is active, else FALSE*/
GetLearnModeState(void);
#endif /*_SLAVE_LEARN_H_*/
