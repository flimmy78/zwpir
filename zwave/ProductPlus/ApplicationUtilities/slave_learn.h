/**
 * @file
 * This file contains a sample of how learn mode could be implemented on ZW0102 standard slave,
 * routing slave and enhanced slave devices. The module works for both battery operated and always
 * listening devices.
 * @copyright Copyright (c) 2001-2016, Sigma Designs Inc., All Rights Reserved
 */

#ifndef _SLAVE_LEARN_H_
#define _SLAVE_LEARN_H_
/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/
typedef enum
{
  LEARN_MODE_DISABLE =  0,     /**< Disable learn process */
  LEARN_MODE_INCLUSION  = 1,   /**< Enable the learn process to do an inclusion */
  LEARN_MODE_EXCLUSION  = 2,   /**< Enable the learn process to do an exclusion */
  LEARN_MODE_EXCLUSION_NWE = 3 /**< Enable the learn process to do an network wide exclusion */
} LEARN_MODE_ACTION;


/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/

/**
 * @brief LearnCompleted
 * Should be implemented by the Application. Called when nodeID have been
 * assigned , deleted or the learn times out nodeID parameter is 0xFF if the
 * learn process times out
 * @param nodeID IN parameter the nodeID assigned
 */
extern void
LearnCompleted(BYTE nodeID);


/**
 * @brief Call this function from the application whenever learnmode should be
 * enabled / Disabled.
 * @details This function do the following:
 *  If the node is not included in network
 *    Set the Slave in classic Learnmode
 *    Starts a two seconds timeout after which we switch to NWI mode
 *    Broadcast the NODEINFORMATION frame once when called.
 *    If classic learn mode timeout start NWI learn mode
 *    If bInclusionReqCount > 1 send explorer inclusion frame
 *     start a 4 + random time timer
 *    If bInclusionReqCount == 1 send explorer inclusion request frame and wait 4 seconds
 *    when timer timeout and bInclusionReqCount == 0 disable NWI mode and call LearnCompleted
 *   If node is not included in a network
 *     Set the Slave in classic Learnmode
 *     Starts a two seconds timeout after which we stop learn mode
 *
 * LearnCompleted will be also called after the end of learn process or a timeout
 * if LearnComplete called due timeout out the nodeID parameter would be 0xFF
 * @param bMode The mode of the learn process:
 *        LEARN_MODE_INCLUSION   Enable the learn mode to do an inclusion
 *        LEARN_MODE_EXCLUSION   Enable the learn mode to do an exclusion
 *        LEARN_MODE_EXCLUSION_NWE Enable the learn mode to do an network wide exclusion
 *        LEARN_MODE_DISABLE      Disable learn mode
 */
void
StartLearnModeNow(LEARN_MODE_ACTION bMode);

#endif /*_SLAVE_LEARN_H_*/
