/****************************************************************************
 *
 * Copyright (c) 2001-2013
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
 *---------------------------------------------------------------------------
 *
 * Description: Implements functions that detects if a button has
 *              been pressed shortly or is beeing held
 *
 *              The module has 2 functions that can be used by an
 *              application:
 *
 *              OneButtonInit()  Initializes the 10ms timer that polls the
 *                               button state
 *
 *              OneButtonLastAction() This function returns the last action
 *                                    performed with the button.
 *
 *              The definitions of the timers used to determine when a
 *              button is pressed or held is in the one_button.h file and
 *              they are defined in 10ms counts.
 *
 * Author:   Peter Shorty
 *
 * Last Changed By:  $Author: efh $
 * Revision:         $Revision: 24920 $
 * Last Changed:     $Date: 2013-03-06 15:43:02 +0100 (on, 06 mar 2013) $
 *
 ****************************************************************************/
#ifndef _ONE_BUTTON_H_
#define _ONE_BUTTON_H_


/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

/* Minimum tinmes the button should be detected before we accept it */
#define DEBOUNCE_COUNT        5

/* Key shall be press in min time to be an event*/
#define MIN_PRESS_COUNT      20

/* Minimum number of times a button should be detected before we say that it
   is held down */
#define BUTTON_HELD_COUNT   100


/****************************************************************************/
/*                              EXTERNAL DEFINED FUNCTIONS/DATA             */
/****************************************************************************/

/* Return values from LastButtonAction */
typedef enum _E_BUTTON_ACTION_
{
  BUTTON_NOTHING      = 0,
  BUTTON_WAS_PRESSED  = 1,
  BUTTON_IS_HELD      = 2,
  BUTTON_WAS_RELEASED = 3,
  BUTTON_TRIPLE_PRESS = 4,
  BUTTON_ACTIVE       = 5
} E_BUTTON_ACTION;

#define ONE_BUTTON_P11     0x01
#define ONE_BUTTON_PB0     0x02
#define ONE_BUTTON_PB1     0x03
#define ONE_BUTTON_PB2     0x04
#define ONE_BUTTON_PB3     0x05
#define ONE_BUTTON_PB4     0x06
/*==============================   LastButtonAction   =======================
**    This function returns the last button action detected.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
 /**
 * @brief OneButtonLastAction
 * Read last One-button last action.
 * Event types:
 *    BUTTON_WAS_PRESSED: Button press (Minimum press time is MIN_PRESS_COUNT and maximum is SHORT_PRESS_COUNT).
 *    BUTTON_IS_HELD:  Button held (min time is BUTTON_HELD_COUNT and event is send out each
 *    BUTTON_HELD_COUNT time.)
 *    BUTTON_WAS_RELEASED: Button release is only send after BUTTON_IS_HELD event.
 *    BUTTON_TRIPLE_PRESS: triple press. Each triple press most be under time BUTTON_IS_HELD
 * @param par description..
 * @return description..
 */
E_BUTTON_ACTION OneButtonLastAction(void);

/*===============================   OneButtonInit   ========================
**    This function initializes the one button polling
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
BOOL OneButtonInit(BYTE bButton);


#endif /*_ONE_BUTTON_H_*/
