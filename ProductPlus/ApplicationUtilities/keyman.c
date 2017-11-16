/***************************************************************************
*
* Copyright (c) 2001-2014
* Sigma Designs, Inc.
* All Rights Reserved
*
*---------------------------------------------------------------------------
*
* Description: Some nice descriptive description.
*
* Author: Thomas Roll
*
* Last Changed By: $Author: tro $
* Revision: $Revision: 0.00 $
* Last Changed: $Date: 2014/06/09 12:01:22 $
*
****************************************************************************/

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <keyman.h>
#include <one_button.h>
#include <p_button.h>
#include <ZW_uart_api.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/
#ifdef ZW_DEBUG_KEYMAN
#define ZW_DEBUG_KEYMAN_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_KEYMAN_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_KEYMAN_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_KEYMAN_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_KEYMAN_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_KEYMAN_SEND_BYTE(data)
#define ZW_DEBUG_KEYMAN_SEND_STR(STR)
#define ZW_DEBUG_KEYMAN_SEND_NUM(data)
#define ZW_DEBUG_KEYMAN_SEND_WORD_NUM(data)
#define ZW_DEBUG_KEYMAN_SEND_NL()
#endif

typedef struct _KEYMAN_
{
  VOID_CALLBACKFUNC(pAppStMan)(BYTE);
  VOID_CALLBACKFUNC(pSetPowerDownTimeout)(BYTE);
} t_KEYMAN;


#ifndef KEY_SCAN_POWERDOWNTIMEOUT
#define KEY_SCAN_POWERDOWNTIMEOUT 3
#endif
/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

t_KEYMAN mykeyMan = {NULL,NULL};
BYTE keyHeldFlag;
/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/


/*============================ InitKeyManager ===============================
** Function description
** This function...
**
** Side effects:
**
**-------------------------------------------------------------------------*/
void
InitKeyManager(
  VOID_CALLBACKFUNC(AppStMan)(BYTE),             /*function-pointer to application state machine*/
  VOID_CALLBACKFUNC(pSetPowerDownTimeout)(BYTE)) /*function-pointer to battery ZCB_pSetPowerDownTimeout(BYTE)*/
{
  mykeyMan.pAppStMan = AppStMan;
  mykeyMan.pSetPowerDownTimeout = pSetPowerDownTimeout;

  /* Init button module */
  initButton();
  OneButtonInit(ONE_BUTTON_PB0);
  keyHeldFlag = 0;

}


/*============================ KeyScan ===============================
** Function description
** scan input pins for change and convert it to events.
**
** Side effects:
**
**-------------------------------------------------------------------------*/
void
KeyScan()
{
  static BYTE B0_10Sec_press = 0;
  static BYTE bPinState = 0;
  E_BUTTON_ACTION lastAction = OneButtonLastAction();
  BYTE PButtons = ButtonPressed();

  if(NULL == mykeyMan.pAppStMan)
  {
    /*Not initalized*/
    return;
  }

  if(bPinState || (BUTTON_NOTHING != lastAction))
  {
    /* pin active.. do not sleep*/
    /*Kick timerout each time an event is received*/
    if(NULL != mykeyMan.pSetPowerDownTimeout)
    {
      mykeyMan.pSetPowerDownTimeout(KEY_SCAN_POWERDOWNTIMEOUT);
    }
  }


  /*S1 P2_4 */
  if(lastAction == BUTTON_TRIPLE_PRESS )
  {
    ZW_DEBUG_KEYMAN_SEND_STR("*S1 triple");
    ZW_DEBUG_KEYMAN_SEND_NL();
    mykeyMan.pAppStMan(EVENT_KEY_B0_TRIPLE_PRESS);
  }
  else if(lastAction == BUTTON_WAS_PRESSED )
  {
    ZW_DEBUG_KEYMAN_SEND_STR("*S1 down");
    ZW_DEBUG_KEYMAN_SEND_NL();
    mykeyMan.pAppStMan(EVENT_KEY_B0_PRESS);
  }
  else if(lastAction == BUTTON_IS_HELD )
  {

    ZW_DEBUG_KEYMAN_SEND_STR("*S1 held");
    ZW_DEBUG_KEYMAN_SEND_NL();
    if(10 == ++B0_10Sec_press){
    ZW_DEBUG_KEYMAN_SEND_STR("->S1 held");
      mykeyMan.pAppStMan(EVENT_KEY_B0_HELD);
    }
  }
  else if(lastAction == BUTTON_WAS_RELEASED )
  {
    ZW_DEBUG_KEYMAN_SEND_STR("*S1 release");
    ZW_DEBUG_KEYMAN_SEND_NL();
    mykeyMan.pAppStMan(EVENT_KEY_B0_RELEASE);
    B0_10Sec_press = 0;
  }


  /*S2 P3_6 */
  if (IS_DOWN_PB1(PButtons) )
  {

    if(0 == (bPinState & 0x2))
    {
      ZW_DEBUG_KEYMAN_SEND_STR("*S2 down");
      ZW_DEBUG_KEYMAN_SEND_NL();
      /*key press*/
      bPinState |= 0x2;
      mykeyMan.pAppStMan(EVENT_KEY_B1_PRESS);
    }
    else if (IS_HELD_PB1())
    {
      if ((keyHeldFlag & DOWN_PB1) == 0)
      {
        ZW_DEBUG_KEYMAN_SEND_STR("*S2 Held");
        ZW_DEBUG_KEYMAN_SEND_NL();

        keyHeldFlag |= DOWN_PB1;
        mykeyMan.pAppStMan(EVENT_KEY_B1_HELD);
      }
    }
  }

  if (IS_RELEASED_PB1(PButtons) && (bPinState & 0x2))
  {
    ZW_DEBUG_KEYMAN_SEND_STR("*S2 release");
    ZW_DEBUG_KEYMAN_SEND_NL();
    keyHeldFlag &= ~DOWN_PB1;
    bPinState &= 0xFD;
    mykeyMan.pAppStMan(EVENT_KEY_B1_RELEASE);
  }


  /*S3 P2_3 */
  if (IS_DOWN_PB2(PButtons) )
  {

    if(0 == (bPinState & 0x4))
    {
      ZW_DEBUG_KEYMAN_SEND_STR("*S3 down");
      ZW_DEBUG_KEYMAN_SEND_NL();
      /*key press*/
      bPinState |= 0x4;
      mykeyMan.pAppStMan(EVENT_KEY_B2_PRESS);
    }
    else if (IS_HELD_PB2())
    {
      if ((keyHeldFlag & DOWN_PB2) == 0)
      {
        ZW_DEBUG_KEYMAN_SEND_STR("*S3 Held");
        ZW_DEBUG_KEYMAN_SEND_NL();
        keyHeldFlag |= DOWN_PB2;
        mykeyMan.pAppStMan(EVENT_KEY_B2_HELD);
      }
    }
  }

  if (IS_RELEASED_PB2(PButtons) && (bPinState & 0x4))
  {
    ZW_DEBUG_KEYMAN_SEND_STR("*S3 release");
    ZW_DEBUG_KEYMAN_SEND_NL();
    keyHeldFlag &= ~DOWN_PB2;
    bPinState &= 0xFB;
    mykeyMan.pAppStMan(EVENT_KEY_B2_RELEASE);
  }


  /*S4 P2_2 */
  if (IS_DOWN_PB3(PButtons) )
  {

    if(0 == (bPinState & 0x8))
    {
      ZW_DEBUG_KEYMAN_SEND_STR("*S4 down");
      ZW_DEBUG_KEYMAN_SEND_NL();
      /*key press*/
      bPinState |= 0x8;
      mykeyMan.pAppStMan(EVENT_KEY_B3_PRESS);
    }
    else if (IS_HELD_PB3())
    {
      if ((keyHeldFlag & DOWN_PB3) == 0)
      {
        ZW_DEBUG_KEYMAN_SEND_STR("*S4 Held");
        ZW_DEBUG_KEYMAN_SEND_NL();
        keyHeldFlag |= DOWN_PB3;
        mykeyMan.pAppStMan(EVENT_KEY_B3_HELD);
      }
    }
  }

  if (IS_RELEASED_PB3(PButtons) && (bPinState & 0x8))
  {
    ZW_DEBUG_KEYMAN_SEND_STR("*S4 release");
    ZW_DEBUG_KEYMAN_SEND_NL();
    keyHeldFlag &= ~DOWN_PB3;
    bPinState &= 0xF7;
    mykeyMan.pAppStMan(EVENT_KEY_B3_RELEASE);
  }

  /*S5 P3_5 */
  if (IS_DOWN_PB4(PButtons) )
  {

    if(0 == (bPinState & 0x10))
    {
      ZW_DEBUG_KEYMAN_SEND_STR("*S5 down");
      ZW_DEBUG_KEYMAN_SEND_NL();
      /*key press*/
      bPinState |= 0x10;
      mykeyMan.pAppStMan(EVENT_KEY_B4_PRESS);
    }


  /*S5 P3_5 */
    else if (IS_HELD_PB4())
  {
      if ((keyHeldFlag & DOWN_PB4) == 0)
    {
        ZW_DEBUG_KEYMAN_SEND_STR("*S5 Held");
      ZW_DEBUG_KEYMAN_SEND_NL();
        keyHeldFlag |= DOWN_PB4;
        mykeyMan.pAppStMan(EVENT_KEY_B4_HELD);
      }
    }
  }

  if (IS_RELEASED_PB4(PButtons) && (bPinState & 0x10))
  {
    ZW_DEBUG_KEYMAN_SEND_STR("*S5 release");
    ZW_DEBUG_KEYMAN_SEND_NL();
    keyHeldFlag &= ~DOWN_PB4;
    bPinState &= 0xEF;
    mykeyMan.pAppStMan(EVENT_KEY_B4_RELEASE);
  }
}
