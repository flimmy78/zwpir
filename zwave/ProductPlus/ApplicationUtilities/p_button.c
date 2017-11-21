/******************************* p_button.c  *******************************
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
 * Description: Button module for development kit controller board.
 *
 * Author:   Henrik Holm
 *
 * Last Changed By:  $Author: efh $
 * Revision:         $Revision: 23679 $
 * Last Changed:     $Date: 2012-11-09 13:38:48 +0200 (Пт, 09 ноя 2012) $
 *
 ****************************************************************************/


/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <p_button.h>
#include <ZW_pindefs.h>
#include <ZW_timer_api.h>
#include <ZW_uart_api.h>

#ifdef ZM5202
/*S1 P2_4 */
#define PB0Port  P2
#define PB0SHADOW P2Shadow
#define PB0SHADOWDIR  P2ShadowDIR
#define PB0DIR   P2DIR
#define PB0DIR_PAGE   P2DIR_PAGE
#define PB0      4

/*S2 P3_6 */
#define PB1Port  P3
#define PB1SHADOW P3Shadow
#define PB1SHADOWDIR  P3ShadowDIR
#define PB1DIR   P3DIR
#define PB1DIR_PAGE   P3DIR_PAGE
#define PB1      6

/*S3 P2_3 */
#define PB2Port  P2
#define PB2SHADOW P2Shadow
#define PB2SHADOWDIR  P2ShadowDIR
#define PB2DIR   P2DIR
#define PB2DIR_PAGE   P2DIR_PAGE
#define PB2      3

/*S4 P2_2 */
#define PB3Port  P2
#define PB3SHADOW P2Shadow
#define PB3SHADOWDIR  P2ShadowDIR
#define PB3DIR   P2DIR
#define PB3DIR_PAGE   P2DIR_PAGE
#define PB3      2

/*S5 P3_5 */
#define PB4Port  P3
#define PB4SHADOW P2Shadow
#define PB4SHADOWDIR  P2ShadowDIR
#define PB4DIR   P2DIR
#define PB4DIR_PAGE   P2DIR_PAGE
#define PB4      5

#define EEPCSPort P2
#define EEPCSSHADOW P2Shadow
#define EEPCSSHADOWDIR  P2ShadowDIR
#define EEPCSDIR  P2DIR
#define EEPCSDIR_PAGE   P2DIR_PAGE
#define EEPCS     5

#else /*ZM5101 and ZM3502*/

/*S1 P2_4 */
#define PB0Port  P2
#define PB0SHADOW P2Shadow
#define PB0SHADOWDIR  P2ShadowDIR
#define PB0DIR   P2DIR
#define PB0DIR_PAGE   P2DIR_PAGE
#define PB0      4

/*S2 P3_6 */
#define PB1Port  P3
#define PB1SHADOW P3Shadow
#define PB1SHADOWDIR  P3ShadowDIR
#define PB1DIR   P3DIR
#define PB1DIR_PAGE   P3DIR_PAGE
#define PB1      6

/*S3 P2_3 */
#define PB2Port  P2
#define PB2SHADOW P2Shadow
#define PB2SHADOWDIR  P2ShadowDIR
#define PB2DIR   P2DIR
#define PB2DIR_PAGE   P2DIR_PAGE
#define PB2      3

/*S4 P2_2 */
#define PB3Port  P2
#define PB3SHADOW P2Shadow
#define PB3SHADOWDIR  P2ShadowDIR
#define PB3DIR   P2DIR
#define PB3DIR_PAGE   P2DIR_PAGE
#define PB3      2

/*S5 P2_1 */
#define PB4Port  P2
#define PB4SHADOW P2Shadow
#define PB4SHADOWDIR  P2ShadowDIR
#define PB4DIR   P2DIR
#define PB4DIR_PAGE   P2DIR_PAGE
#define PB4      1

#define EEPCSPort P2
#define EEPCSSHADOW P2Shadow
#define EEPCSSHADOWDIR  P2ShadowDIR
#define EEPCSDIR  P2DIR
#define EEPCSDIR_PAGE   P2DIR_PAGE
#define EEPCS     5
#endif
/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

/*Timing used when checking button status*/
#define PB_DEBOUNCE_TIME 2      /*10-20ms debounce should be enough*/
#define PB_LONG_TIME 1000 //60         /*600ms before we assume the key is held down*/

#define PB_HOLD_TIME 50          /*1000ms before we assume the key is held down*/

/*If the button is pressed set bit to 1*/
#define SET_PB_STATUS(val) if(PIN_GET(val##)==0) bPressed |= DOWN_##val

//#define IS_DOWN_REPEAT(val) ((bPressed&DOWN_##val)&&(bPressed&PB_REPEAT))
//#define IS_DOWN_HELD(val) ((bPressed&DOWN_##val)&&(bPressed&PB_HELD))

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

static BYTE bPressed = FALSE;
static BYTE bOldPressed = FALSE;
static WORD readyCount = 0;
static BYTE holdCount[5] = {0,0,0,0,0};

/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/
void ZCB_TimerCheckButtons(void);

code const void (code * ZCB_TimerCheckButtons_p)(void) = &ZCB_TimerCheckButtons;
/*============================   TimerCheckButtons   ======================
**    Function description
**    This function checks and updates the status of the Push buttons.
**    It is run every 10ms
**    Side effects:
**
**--------------------------------------------------------------------------*/
void ZCB_TimerCheckButtons(void)
{
  if(PIN_GET(EEPCS)!=0) /*Only check buttons if EEPROM is not selected*/
  {
    if(bPressed)
      bOldPressed = bPressed&PB_MASK; //Store old values if a key is down

    bPressed = FALSE;
  /*Call macro to set value of PBs*/
    PIN_IN(PB0,1);
    PIN_IN(PB1,1);
    PIN_IN(PB2,1);
    PIN_IN(PB3,1);
#ifndef ZM5202
    PIN_IN(PB4,1);
#endif    

    SET_PB_STATUS(PB0);
    SET_PB_STATUS(PB1);
    SET_PB_STATUS(PB2);
    SET_PB_STATUS(PB3);
#ifndef ZM5202
    SET_PB_STATUS(PB4);
#endif    
  }

  /*Make sure that MOSI is output after we checked the key. The rest is ok as inputs*/
  PIN_OUT(SPI1_SCK); // PB4 (P24) is used as SCK output to the EEPROM
  PIN_OUT(SPI1_MOSI); // PB4 (P22) is used as MOSI output to the EEPROM

  if(bPressed)
  {
    readyCount++; /*Increment readyCount if PB is down*/
  }
  else
  {
    /*A key has been released, increment debounce*/
    if(bOldPressed&PB_RELEASE)
      readyCount++;
    else if((bOldPressed&PB_MASK))
    {
      /*If last detection resulted in a value. Restart DEBOUNCE*/
      readyCount = 0;
      bPressed = 0;    /*Zero bPressed*/
      bOldPressed |= PB_RELEASE;         /*Set bit, release detect ongoing*/
    }

  }
  if(readyCount == 0xFFFF)
    readyCount = 0xFFFE;

  if(readyCount>PB_DEBOUNCE_TIME)
  {
    if (bPressed)
    {
      if (IS_DOWN_PB0(bPressed) &&(holdCount[0] < PB_HOLD_TIME))
      {
        holdCount[0]++;
      }
      if (IS_DOWN_PB1(bPressed) && (holdCount[1] < PB_HOLD_TIME))
      {
        holdCount[1]++;
      }
#if 0
      if (IS_DOWN_PB2(bPressed) && (holdCount[2] < PB_HOLD_TIME))
      {
        holdCount[2]++;
      }
#endif
      if (IS_DOWN_PB3(bPressed) && (holdCount[3] < PB_HOLD_TIME))
      {
        holdCount[3]++;
      }
      if (IS_DOWN_PB4(bPressed) && (holdCount[4] < PB_HOLD_TIME))
      {
        holdCount[4]++;
      }
    }
    else
    {
      holdCount[0] = 0;
      holdCount[1] = 0;
      holdCount[2] = 0;
      holdCount[3] = 0;
      holdCount[4] = 0;
    }
    bOldPressed = FALSE;              /*Zero old values. No need when PB_DEBOUNCE expires*/
  }

}

/*============================   ButtonPressed   ============================
**    Function description
**     Returns the status of the Push buttons
**    Side effects:
**
**--------------------------------------------------------------------------*/
BYTE ButtonPressed(void)
{
 if(readyCount>PB_DEBOUNCE_TIME)
   return bPressed;
 else
   return bOldPressed;
}

BOOL
IsButtonHeld(BYTE btnNr)
{
  if (holdCount[btnNr] == PB_HOLD_TIME)
    return TRUE;
  else
    return FALSE;
}
BYTE initButton(void)
{
  return TimerStart(ZCB_TimerCheckButtons, 1, TIMER_FOREVER);
}

