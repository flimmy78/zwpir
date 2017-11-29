/**
 * @file
 * Used for testing purposes.
 * @copyright Copyright (c) 2001-2016, Sigma Designs Inc., All Rights Reserved
 */

#ifndef _CTRL_LEARN_H_
#define _CTRL_LEARN_H_

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
extern BOOL learnInProgress;  /**< Application can use this flag to check if learn
                                  mode is active*/
/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/

/**
 * @brief LearnCompleted called when node is done learning
 * @param[in] glearnNodeInfo of type LEARN_INFO
 */
extern void LearnCompleted(LEARN_INFO *glearnNodeInfo);


/**
 * @brief StartLearnModeNow
 *  Call this function whenever learnmode should be entered.
 *  This function does the following:
 *    - Set the controller in Learnmode
 *    - Starts a one second timeout after which learn mode is disabled
 *    - learnState will be TRUE until learnmode is done.
 *  If the Controller is added or removed to/from a network the function
 *  LearnCompleted will be called.
 * @param[in] bMode mode to start of type:
 *            -ZW_SET_LEARN_MODE_DISABLE           0x00
 *            -ZW_SET_LEARN_MODE_CLASSIC           0x01
 *            -ZW_SET_LEARN_MODE_NWI               0x02
 *            -ZW_SET_LEARN_MODE_NWE               0x03
 * @return description..
 */
void StartLearnModeNow(LEARN_MODE_ACTION bMode);

/*============================   StopLearnModeNow   ======================
**    Function description
**      Call this function from the application whenever learnmode
**      should be disabled.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
/**
 * @brief StopLearnModeNow
 * Call this function from the application whenever learnmode
 * should be disabled.
 * @return if failing FALSE else TRUE
 */
BYTE StopLearnModeNow();

/*==========================   ReArmLearnModeTimeout   =======================
**    Function description
**      Rearms the LearnMode timout handler and thereby extending the time
**      that the controller are to be in LearnMode/Receive.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/

/**
 * @brief ReArmLearnModeTimeout
 * Rearms the LearnMode timout handler and thereby extending the time
 * that the controller are to be in LearnMode/Receive.
 */
void ReArmLearnModeTimeout();

//extern void ZCB_EndLearnNodeState(void);

//extern void ZCB_LearnModeCompleted(LEARN_INFO *glearnNodeInfo);

//extern void ZCB_SendExplorerRequest(void);

#endif /*_CTRL_LEARN_H_*/
