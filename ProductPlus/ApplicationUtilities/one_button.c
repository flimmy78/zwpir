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

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_basis_api.h>
#include <ZW_timer_api.h>
#include <ZW_pindefs.h>
#include <ZW_evaldefs.h>
#include <p_button.h>
#include <one_button.h>
#include <ZW_uart_api.h>
#ifdef ZW_DEBUG_ONE_BUTTON
#include <ZW_uart_api.h>
#define ZW_DEBUG_ONE_B_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_ONE_B_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_ONE_B_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_ONE_B_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_ONE_B_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_ONE_B_SEND_BYTE(data)
#define ZW_DEBUG_ONE_B_SEND_NUM(data)
#define ZW_DEBUG_ONE_B_SEND_NL()
#endif


#include <ZW_uart_api.h>
/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/
BYTE  buttonAction;
BYTE  buttonCount;
BYTE  oneButton;
BYTE bTriplePress;
BYTE bTriplePressHandle;

#define TIME_TRIPLE_PRESS     100 /* Triple press timeout is set to 1.5sec */

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

#ifdef ZW_DEBUG_ONE_BUTTON
extern void ZW_DebugXData(void);
#endif
/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/
void ZCB_OneButtonPoll(void);
void ZCB_OneButtonTriplePressTimeout(void);
static BOOL
OneButtonPressed(BYTE bButton)
{
  register BYTE btn;
  btn = ButtonPressed();
  if (bButton == ONE_BUTTON_P11)
    return ((!btn && BUTTON_PRESSED()));
  else if (bButton == ONE_BUTTON_PB0)
    return (IS_DOWN_PB0(btn));
  else if (bButton == ONE_BUTTON_PB1)
    return (IS_DOWN_PB1(btn));
  else if (bButton == ONE_BUTTON_PB2)
    return (IS_DOWN_PB2(btn));
  else if (bButton == ONE_BUTTON_PB3)
    return (IS_DOWN_PB3(btn));
  else if (bButton == ONE_BUTTON_PB4)
    return (IS_DOWN_PB4(btn));
  else
     return FALSE;



}
code const void (code * ZCB_OneButtonPoll_p)(void) = &ZCB_OneButtonPoll;
/*================================   OneButtonPoll   =========================
**    Poll function that polls the button every 10ms
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void             /*RET  Nothing                  */
ZCB_OneButtonPoll(void)  /*IN  Nothing                   */
{
  /* Check button state */
  if (OneButtonPressed(oneButton))
  {
    /* Check if button is pushed long */
    if (++buttonCount >= BUTTON_HELD_COUNT)
    {
      /*First BUTTON_HELD_COUNT event*/
      if(buttonCount == (BUTTON_HELD_COUNT + 1))
      {
        buttonAction = BUTTON_IS_HELD;
      }
      else
      {
        /*other BUTTON_HELD_COUNT event*/
        if(buttonCount == (2*BUTTON_HELD_COUNT + 1))
        {
          buttonAction = BUTTON_IS_HELD;
          buttonCount = BUTTON_HELD_COUNT + 2;
        }
      }
    }
  }
  else
  {
    if  (buttonCount>DEBOUNCE_COUNT)
    {
      ZW_DEBUG_ONE_B_SEND_BYTE('.');
      if ((buttonCount < BUTTON_HELD_COUNT))
      {
        /*Short press demands a minimun press time else it is for triple-press.!*/
        if((buttonCount > MIN_PRESS_COUNT))
        {
          buttonAction = BUTTON_WAS_PRESSED;

          /*Stop triple press*/
          if(0 != bTriplePressHandle)
          {
            TimerCancel(bTriplePressHandle);
            bTriplePressHandle = 0;
          }
          bTriplePress = 0;
        }
        else
        {
          /* Handle tripple press */
          bTriplePress++;
        }

        if (bTriplePress == 1)
        {
          /* First press, start timer */
          bTriplePressHandle = TimerStart(ZCB_OneButtonTriplePressTimeout, TIME_TRIPLE_PRESS, 1);
          if (bTriplePressHandle == 0xFF)
            bTriplePressHandle = 0;
        }
        else if (bTriplePress == 3)
        {
          /* Triple press detected */
          if (bTriplePressHandle)
          {
            buttonAction = BUTTON_TRIPLE_PRESS;
            TimerCancel(bTriplePressHandle);
            bTriplePressHandle = 0;
            ZW_DEBUG_ONE_B_SEND_BYTE('B');
            ZW_DEBUG_ONE_B_SEND_BYTE('T');
            ZW_DEBUG_ONE_B_SEND_NL();

          }
          bTriplePress = 0;
        }
      }
      else
      {
        /*We only send BUTTON_WAS_RELEASED after button was held*/
        buttonAction = BUTTON_WAS_RELEASED;
        ZW_DEBUG_ONE_B_SEND_BYTE('B');
        ZW_DEBUG_ONE_B_SEND_BYTE('R');
        ZW_DEBUG_ONE_B_SEND_NL();
      }
    }
      buttonCount = 0;
  }
}

code const void (code * ZCB_OneButtonTriplePressTimeout_p)(void) = &ZCB_OneButtonTriplePressTimeout;
/*=========================   OneButtonTriplePressTimeout   =================
**    Timeout function for the tripple press detection
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ZCB_OneButtonTriplePressTimeout(void)
{
  bTriplePress = 0;
  bTriplePressHandle = 0;
}


/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/

/*===============================   OneButtonInit   ========================
**    This function initializes the one button polling
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
BOOL
OneButtonInit(BYTE bButton)
{
/****************************************************************************/
/*                 Initialize PRIVATE TYPES and DEFINITIONS                 */
/****************************************************************************/
  buttonAction = 0;
  buttonCount = 0;
  bTriplePress = 0;
  bTriplePressHandle = 0;
  oneButton = bButton;

  if ( TimerStart(ZCB_OneButtonPoll, 1, TIMER_FOREVER) == 0xFF)
    return FALSE;
#ifdef ZW_DEBUG_ONE_BUTTON
  ZW_UART_INIT(1152);
  ZW_DEBUG_ONE_B_SEND_BYTE('s');
#endif
  return TRUE;
}


/*==============================   LastButtonAction   =======================
**    This function returns the last button action detected.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
BYTE
OneButtonLastAction(void)
{
  register BYTE bTemp;

  bTemp = buttonAction;

  if((bTemp == 0) && buttonCount)
  {
    bTemp = BUTTON_ACTIVE; /* Used to tell batt_man S1 is active*/
  }
  buttonAction = 0;
  return bTemp;
}
