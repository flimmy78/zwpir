/***************************************************************************
*
* Copyright (c) 2001-2011
* Sigma Designs, Inc.
* All Rights Reserved
*
*---------------------------------------------------------------------------
*
* Description: Multilevel switch utilites source file
*
* Author: Samer Seoud
*
* Last Changed By:  $Author:  $
* Revision:         $Revision:  $
* Last Changed:     $Date:  $
*
****************************************************************************/

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_basis_api.h>
#include <ZW_tx_mutex.h>
#include <ZW_TransportLayer.h>
#include <ZW_uart_api.h>
#include "config_app.h"
#include <CommandClassMultiLevelSwitch.h>
#include <misc.h>
#include <endpoint_lookup.h>
/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/
#ifdef ZW_DEBUG_MULTISWITCH
#define ZW_DEBUG_MULTISWITCH_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_MULTISWITCH_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_MULTISWITCH_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_MULTISWITCH_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_MULTISWITCH_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_MULTISWITCH_SEND_BYTE(data)
#define ZW_DEBUG_MULTISWITCH_SEND_STR(STR)
#define ZW_DEBUG_MULTISWITCH_SEND_NUM(data)
#define ZW_DEBUG_MULTISWITCH_SEND_WORD_NUM(data)
#define ZW_DEBUG_MULTISWITCH_SEND_NL()
#endif

#ifdef ENDPOINT_SUPPORT
#define SWITCH_ENDPOINT_IDX  endpointidx
#else
#define SWITCH_ENDPOINT_IDX  0
#endif

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

#define SWITCH_DIMMING_UP   0x01
#define SWITCH_IS_DIMMING   0x02
#define SWITCH_IS_ON        0x04

typedef struct __MultiLvlSwitch_
{
  BYTE bOnStateSwitchLevel;  /*save the on state level value when we set the HW to off*/
  BYTE bCurrentSwitchLevel;  /*hold the current switch level value*/
  BYTE bTargetSwitchLevel;   /*hold the value we want to set the switch to when we are changing*/
  DWORD lTicksCountReload;
  DWORD lTicksCount;
  BYTE switchFlag;
}_MultiLvlSwitch;


static _MultiLvlSwitch MultiLvlSwitch[SWITCH_MULTI_ENDPOINTS];
static BYTE bMultiLevelSwTimerHandle;
static ENDPOINT_LOOKUP multiLevelEpLookup;

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/
void ZCB_SwitchLevelHandler(void);

#ifdef NOT_USED
code const void (code * ZCB_SwitchLevelHandler_p)(void) = &ZCB_SwitchLevelHandler;
/*============================ ZCB_SwitchLevelHandler ===============================
** Function description
** This timer function handle the switch level changing
**
** Side effects:
**
**-------------------------------------------------------------------------*/
void
#endif
PCB(ZCB_SwitchLevelHandler)(void)
{
  BYTE i;
  BOOL boAllTimersOff = TRUE;
  for (i = 0; i < GetEndPointCount(&multiLevelEpLookup); i++)
  {
    if (!(MultiLvlSwitch[i].switchFlag & SWITCH_IS_DIMMING))
      continue;
    if (!--MultiLvlSwitch[i].lTicksCount)
    {
      ZW_DEBUG_MULTISWITCH_SEND_STR("Timeout :");
      ZW_DEBUG_MULTISWITCH_SEND_NUM(MultiLvlSwitch[i].bCurrentSwitchLevel);

      (MultiLvlSwitch[i].switchFlag & SWITCH_DIMMING_UP)? MultiLvlSwitch[i].bCurrentSwitchLevel++: MultiLvlSwitch[i].bCurrentSwitchLevel--;
      CommandClassMultiLevelSwitchSet(MultiLvlSwitch[i].bCurrentSwitchLevel, FindEndPointID(&multiLevelEpLookup,i));
      if (MultiLvlSwitch[i].bCurrentSwitchLevel == MultiLvlSwitch[i].bTargetSwitchLevel)
      {
        ZW_DEBUG_MULTISWITCH_SEND_NL();
        ZW_DEBUG_MULTISWITCH_SEND_BYTE('S');
        ZW_DEBUG_MULTISWITCH_SEND_NUM(MultiLvlSwitch[i].bTargetSwitchLevel);
        MultiLvlSwitch[i].switchFlag &= ~SWITCH_IS_DIMMING;
        MultiLvlSwitch[i].bCurrentSwitchLevel =  CommandClassMultiLevelSwitchGet( FindEndPointID(&multiLevelEpLookup,i) );
        if (MultiLvlSwitch[i].bCurrentSwitchLevel)
          MultiLvlSwitch[i].bOnStateSwitchLevel = MultiLvlSwitch[i].bCurrentSwitchLevel;
      }
      else
      {
        MultiLvlSwitch[i].lTicksCount =  MultiLvlSwitch[i].lTicksCountReload;
        boAllTimersOff = FALSE;
      }
      ZW_DEBUG_MULTISWITCH_SEND_NL();
    }
    else
    {
      boAllTimersOff = FALSE;
    }
  }
  if (boAllTimersOff)
  {
    TimerCancel(bMultiLevelSwTimerHandle);
    bMultiLevelSwTimerHandle = 0;
    ZW_DEBUG_MULTISWITCH_SEND_BYTE('T');
    ZW_DEBUG_MULTISWITCH_SEND_NL();
  }
}



/*============================ StartLevelChange ===============================
** Function description
** Start level changing, caluclate the number of ms it should take to reach the target
** level
**
** Side effects:
**
**-------------------------------------------------------------------------*/
void
StartLevelChange(BYTE bEndPointIdx,
                 BYTE bDuration, /*the duration value accoring to the command class specification*/
                 BYTE blevels /*The number of levels from the current hw levels to the target level*/
 )
{
  BYTE endpointID = FindEndPointID(&multiLevelEpLookup,bEndPointIdx);
  ZW_DEBUG_MULTISWITCH_SEND_STR("StartLevelChange :");
  ZW_DEBUG_MULTISWITCH_SEND_NUM(bEndPointIdx);
  ZW_DEBUG_MULTISWITCH_SEND_NUM(bDuration);
  ZW_DEBUG_MULTISWITCH_SEND_NUM(blevels);

  if ((bDuration == 0) || (bDuration == 0xFF)) /*set the level instantly*/
                                               /*0xff is used to indicate factory default duration whic is 0 in our case.*/
  {
    CommandClassMultiLevelSwitchSet( MultiLvlSwitch[bEndPointIdx].bTargetSwitchLevel, endpointID );
    MultiLvlSwitch[bEndPointIdx].bCurrentSwitchLevel = CommandClassMultiLevelSwitchGet(endpointID);
    if (MultiLvlSwitch[bEndPointIdx].bCurrentSwitchLevel)
      MultiLvlSwitch[bEndPointIdx].bOnStateSwitchLevel = MultiLvlSwitch[bEndPointIdx].bCurrentSwitchLevel;
    ZW_DEBUG_MULTISWITCH_SEND_NUM(MultiLvlSwitch[bEndPointIdx].bCurrentSwitchLevel);
  }
  else
  {

    if (bDuration> 0x7F) /*duration is in minutes*/
    {
      MultiLvlSwitch[bEndPointIdx].lTicksCountReload = bDuration - 0x7F;
      /*convert the minutes to 10ms units */
      MultiLvlSwitch[bEndPointIdx].lTicksCountReload *= 6000; /*convert the minutes to 10ms units*/
    }
    else
    {/*duration in seconds*/
      MultiLvlSwitch[bEndPointIdx].lTicksCountReload = bDuration;
      MultiLvlSwitch[bEndPointIdx].lTicksCountReload *= 100; /*convert the seconds to 10ms units*/
    }
    /*calculate the number of 10ms ticks between each level change*/
    MultiLvlSwitch[bEndPointIdx].lTicksCountReload /= blevels;
    MultiLvlSwitch[bEndPointIdx].lTicksCount = MultiLvlSwitch[bEndPointIdx].lTicksCountReload; /* lTicksCount is used to count down */
    MultiLvlSwitch[bEndPointIdx].switchFlag |= SWITCH_IS_DIMMING;

    if (!bMultiLevelSwTimerHandle)
      bMultiLevelSwTimerHandle = TimerStart(ZCB_SwitchLevelHandler, 1,  TIMER_FOREVER);
    ZW_DEBUG_MULTISWITCH_SEND_NUM(bMultiLevelSwTimerHandle);
    ZW_DEBUG_MULTISWITCH_SEND_STR("EN");
    ZW_DEBUG_MULTISWITCH_SEND_NL();
  }
}

BYTE GetTargetLevel(uint8_t endpoint)
{
//#ifdef ENDPOINT_SUPPORT
  BYTE endpointidx = 0;
  endpointidx = FindEndPointIndex(&multiLevelEpLookup,endpoint);
  if (endpointidx == 0xFF)
    return 0;
//#endif

  ZW_DEBUG_MULTISWITCH_SEND_STR("GetTargetLevel :");
//#ifdef ENDPOINT_SUPPORT
  ZW_DEBUG_MULTISWITCH_SEND_NUM(endpoint);
//#endif
  if (MultiLvlSwitch[SWITCH_ENDPOINT_IDX].switchFlag & SWITCH_IS_DIMMING )
  {
    return MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bTargetSwitchLevel;
  }
  else
  {
    return MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bCurrentSwitchLevel;
  }
}

BYTE GetCurrentDuration(uint8_t endpoint)
{
  DWORD tmpDuration;
  BYTE tmpLvl;
//#ifdef ENDPOINT_SUPPORT
  BYTE endpointidx = 0;
  endpointidx = FindEndPointIndex(&multiLevelEpLookup,endpoint);
  if (endpointidx == 0xFF)
    return 0xFE;
//#endif
  ZW_DEBUG_MULTISWITCH_SEND_STR("GetTargetLevel :");
//#ifdef ENDPOINT_SUPPORT
  ZW_DEBUG_MULTISWITCH_SEND_NUM(bEndPointIdx);
//#endif
  if (MultiLvlSwitch[SWITCH_ENDPOINT_IDX].switchFlag & SWITCH_IS_DIMMING )
  {
    if (MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bTargetSwitchLevel > MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bCurrentSwitchLevel)
    {
      tmpLvl = MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bTargetSwitchLevel - MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bCurrentSwitchLevel;
    }
    else
    {
      tmpLvl = MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bCurrentSwitchLevel - MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bTargetSwitchLevel;
    }
    /*convert duration from 10ms ticks per level to seconds*/
    /*lTicksCountReload hold the number of 10ms ticks per level*/
    /*tmpLvl hild the number of remaining levels to reach the target.*/
    tmpDuration = (MultiLvlSwitch[SWITCH_ENDPOINT_IDX].lTicksCountReload * tmpLvl)/100 ;
    if (tmpDuration > 127)
    {
      tmpDuration /= 60; /*convert to minuts*/
      tmpDuration += 127; /*add offset according to command class specification*/
    }
    return (BYTE) tmpDuration;
  }
  else
  {
    return 0;
  }
}

/*============================ GetTargetLevel ===============================
** Function description
** This function..returns the remaining duration time to reach the target level
**  The time is zero if traget level was reached
** Side effects:
**
**-------------------------------------------------------------------------*/

void
StopSwitchDimming( BYTE endpoint)
{
  BYTE endpointidx = 0;
ZW_DEBUG_MULTISWITCH_SEND_STR("StopSwitchDimming :");
  endpointidx = FindEndPointIndex(&multiLevelEpLookup,endpoint);
  if (endpointidx == 0xFF)
    return;

  if (MultiLvlSwitch[SWITCH_ENDPOINT_IDX].switchFlag & SWITCH_IS_DIMMING)
  {
    MultiLvlSwitch[SWITCH_ENDPOINT_IDX].switchFlag &= ~SWITCH_IS_DIMMING;
    MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bCurrentSwitchLevel = CommandClassMultiLevelSwitchGet(endpoint);
    if (MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bCurrentSwitchLevel)
      MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bOnStateSwitchLevel = MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bCurrentSwitchLevel;
  }
  ZW_DEBUG_MULTISWITCH_SEND_NUM(MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bCurrentSwitchLevel);
  ZW_DEBUG_MULTISWITCH_SEND_NL();
}


/*============================ HandleStartChangeCmd ===============================
** Function description
** This function...
**
** Side effects:
**
**-------------------------------------------------------------------------*/
void
HandleStartChangeCmd( BYTE bStartLevel,
                     BOOL boIgnoreStartLvl,
                     BOOL boDimUp,
                     BYTE bDimmingDuration,
                     BYTE endpoint )
{
#ifdef ENDPOINT_SUPPORT
  BYTE endpointidx = 0;
  endpointidx = FindEndPointIndex(&multiLevelEpLookup,endpoint);
  if (endpointidx == 0xFF)
    return;
#endif

  ZW_DEBUG_MULTISWITCH_SEND_STR("HandleStartChangeCmd :");
  ZW_DEBUG_MULTISWITCH_SEND_NUM(SWITCH_ENDPOINT_IDX);
  ZW_DEBUG_MULTISWITCH_SEND_NUM(SWITCH_ENDPOINT_IDX);
  ZW_DEBUG_MULTISWITCH_SEND_NUM(boDimUp);
  ZW_DEBUG_MULTISWITCH_SEND_NUM(bDimmingDuration);
  ZW_DEBUG_MULTISWITCH_SEND_NUM(bStartLevel);
  ZW_DEBUG_MULTISWITCH_SEND_NUM(boIgnoreStartLvl);
  ZW_DEBUG_MULTISWITCH_SEND_NUM(MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bCurrentSwitchLevel);
  ZW_DEBUG_MULTISWITCH_SEND_NL();


      /*primary switch Up/Down bit field value are Up*/
  if (boDimUp)
  {
    MultiLvlSwitch[SWITCH_ENDPOINT_IDX].switchFlag |= SWITCH_DIMMING_UP;
    MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bTargetSwitchLevel = 99;
  }
  else
  {
    MultiLvlSwitch[SWITCH_ENDPOINT_IDX].switchFlag &= ~SWITCH_DIMMING_UP; /*we assume the up/down flag is down*/
    MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bTargetSwitchLevel = 0;
  }

  if (!boIgnoreStartLvl)
  {
    if (bStartLevel == 0xFF) /*On state*/
    {/*if we are in off state set the target level to the most recent non-zero level value*/
      bStartLevel = MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bOnStateSwitchLevel;
    }
    else if (bStartLevel == 0x00)
    {/*set off state */
      if (MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bCurrentSwitchLevel)  /*we are in off state then save the current level */
        MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bOnStateSwitchLevel = MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bCurrentSwitchLevel;
    }

  CommandClassMultiLevelSwitchSet( bStartLevel, endpoint);
  }
  MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bCurrentSwitchLevel = CommandClassMultiLevelSwitchGet(endpoint);
  ZW_DEBUG_MULTISWITCH_SEND_NL();
  if (MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bTargetSwitchLevel != MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bCurrentSwitchLevel)
  {
     StartLevelChange(SWITCH_ENDPOINT_IDX, bDimmingDuration, 100);
  }
}


/*============================ HandleSetCmd ===============================
** Function description
** This function...
**
** Side effects:
**
**-------------------------------------------------------------------------*/
void
HandleSetCmd( BYTE bTargetlevel,
             BYTE bDuration,
             BYTE endpoint )
{
  BYTE levels;
#ifdef ENDPOINT_SUPPORT
  BYTE endpointidx = 0;
  endpointidx = FindEndPointIndex(&multiLevelEpLookup,endpoint);
  if (endpointidx == 0xFF)
    return;
#endif
  StopSwitchDimming(endpoint);

  ZW_DEBUG_MULTISWITCH_SEND_STR("HandleSetCmd :");
  ZW_DEBUG_MULTISWITCH_SEND_NUM(SWITCH_ENDPOINT_IDX);
  MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bTargetSwitchLevel = bTargetlevel;
  ZW_DEBUG_MULTISWITCH_SEND_NUM(MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bTargetSwitchLevel);
  MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bCurrentSwitchLevel = CommandClassMultiLevelSwitchGet(endpoint);

  if (MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bTargetSwitchLevel == 0xFF) /*On state*/
  {/*if we are in off state set the target level to the most recent non-zero level value*/
    if (!MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bCurrentSwitchLevel)
    {
      if(MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bOnStateSwitchLevel)
      {
        MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bTargetSwitchLevel = MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bOnStateSwitchLevel;
      }
      else
      {
        MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bTargetSwitchLevel = 0x63; /*last level was 0. Set it to full level*/
      }
    }
    else
    {
      ZW_DEBUG_MULTISWITCH_SEND_BYTE('N');
      ZW_DEBUG_MULTISWITCH_SEND_NL();
      return; /*we are already on then ignore the on command*/
    }
  }
  else if (MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bTargetSwitchLevel == 0x00)
  {/*set off state */

      ZW_DEBUG_MULTISWITCH_SEND_STR(" bCurrentSwitchLevel ");
      ZW_DEBUG_MULTISWITCH_SEND_NUM(MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bCurrentSwitchLevel);
      ZW_DEBUG_MULTISWITCH_SEND_NL();
    if (MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bCurrentSwitchLevel)  /*we are in off state then save the current level */
    {
      MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bOnStateSwitchLevel = MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bCurrentSwitchLevel;
    }
    else
    {
      ZW_DEBUG_MULTISWITCH_SEND_BYTE('%');
      ZW_DEBUG_MULTISWITCH_SEND_NL();
      return; /*we are already off then ignore the off command*/
    }
  }

  if (MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bCurrentSwitchLevel < MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bTargetSwitchLevel)
  {
    MultiLvlSwitch[SWITCH_ENDPOINT_IDX].switchFlag |= SWITCH_DIMMING_UP;
    levels = MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bTargetSwitchLevel - MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bCurrentSwitchLevel;

  }
  else
  {
    MultiLvlSwitch[SWITCH_ENDPOINT_IDX].switchFlag &= ~SWITCH_DIMMING_UP;
    levels = MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bCurrentSwitchLevel - MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bTargetSwitchLevel;
  }
  ZW_DEBUG_MULTISWITCH_SEND_STR(" levels ");
  ZW_DEBUG_MULTISWITCH_SEND_NUM(MultiLvlSwitch[SWITCH_ENDPOINT_IDX].switchFlag & SWITCH_DIMMING_UP);
  ZW_DEBUG_MULTISWITCH_SEND_NUM(MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bTargetSwitchLevel);
  ZW_DEBUG_MULTISWITCH_SEND_NL();
  StartLevelChange(SWITCH_ENDPOINT_IDX, bDuration, levels);

}


/*============================ SetSwitchHwLevel ===============================
** Function description
** This function...
**
** Side effects:
**
**-------------------------------------------------------------------------*/
BOOL
SetSwitchHwLevel(BYTE bInitHwLevel, BYTE endpoint )
{
  BYTE i;
  if (endpoint)
  {
    i =  FindEndPointIndex(&multiLevelEpLookup,endpoint);
    if ( i != 0xFF)
    {
      MultiLvlSwitch[i].bCurrentSwitchLevel = bInitHwLevel;
      MultiLvlSwitch[i].bOnStateSwitchLevel = bInitHwLevel;
      return TRUE;
    }
  }
  return FALSE;
}



/*============================ MultiLevelSwitchInit ===============================
** Function description
** This function...
**
** Side effects:
**
**-------------------------------------------------------------------------*/
void
MultiLevelSwitchInit( BYTE bEndPointCount, BYTE * pEndPointList)
{
  BYTE i;
  multiLevelEpLookup.bEndPointsCount = bEndPointCount;
  multiLevelEpLookup.pEndPointList = pEndPointList;
  for (i = 0; i < bEndPointCount; i++)
  {
    MultiLvlSwitch[i].bCurrentSwitchLevel = 0;
    MultiLvlSwitch[i].bOnStateSwitchLevel = 0;
    MultiLvlSwitch[i].lTicksCount = 0;
    MultiLvlSwitch[i].switchFlag = 0;
  }
  bMultiLevelSwTimerHandle = 0;
}

