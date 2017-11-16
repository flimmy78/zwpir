/***************************************************************************
*
* Copyright (c) 2001-2011
* Sigma Designs, Inc.
* All Rights Reserved
*
*---------------------------------------------------------------------------
*
* Description: SensorPIR source file
*
* Author:
*
* Last Changed By:  $Author:  $
* Revision:         $Revision:  $
* Last Changed:     $Date:  $
*
****************************************************************************/

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

#include "config_app.h"

#include <slave_learn.h>
#include <ZW_slave_api.h>
#ifdef ZW_SLAVE_32
#include <ZW_slave_32_api.h>
#else
#include <ZW_slave_routing_api.h>
#endif  /* ZW_SLAVE_32 */

#include <ZW_classcmd.h>
#include <ZW_mem_api.h>
#include <ZW_TransportLayer.h>

#include <eeprom.h>
#include <ZW_uart_api.h>

#include <misc.h>
#ifdef BOOTLOADER_ENABLED
#include <ota_util.h>
#include <CommandClassFirmwareUpdate.h>
#endif

/*IO control*/
#include <ZW_pindefs.h>
#include <ZW_evaldefs.h>
#include <keyman.h>

#ifdef ZW_ISD51_DEBUG
#include "ISD51.h"
#endif

#include <association_plus.h>
#include <agi.h>
#include <CommandClassAssociation.h>
#include <CommandClassAssociationGroupInfo.h>
#include <CommandClassVersion.h>
#include <CommandClassZWavePlusInfo.h>
#include <CommandClassPowerLevel.h>
#include <CommandClassDeviceResetLocally.h>
#include <manufacturer_specific_device_id.h>
#include <CommandClassBasic.h>

#include <CommandClassBattery.h>
#include <CommandClassNotification.h>
#include <notification.h>
#include <ZW_adcdriv_api.h>
/* BATTERY defines if the node is a Battery Binary Sensor or just a Binary Sensor */
/* ASIC power management functionality */
#include <ZW_power_api.h>

#include <battery_plus.h>
#include <battery_monitor.h>


/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

#ifdef ZW_DEBUG_SENSORPIR
#define ZW_DEBUG_SENSORPIR_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_SENSORPIR_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_SENSORPIR_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_SENSORPIR_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_SENSORPIR_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_SENSORPIR_SEND_BYTE(data)
#define ZW_DEBUG_SENSORPIR_SEND_STR(STR)
#define ZW_DEBUG_SENSORPIR_SEND_NUM(data)
#define ZW_DEBUG_SENSORPIR_SEND_WORD_NUM(data)
#define ZW_DEBUG_SENSORPIR_SEND_NL()
#endif


typedef enum _EVENT_APP_
{
  EVENT_EMPTY = DEFINE_EVENT_APP_NBR,
  EVENT_APP_INIT,
  EVENT_APP_OTA_START,
  EVENT_APP_LEARN_MODE_FINISH,
  EVENT_APP_NEXT_EVENT_JOB,
  EVENT_APP_FINISH_EVENT_JOB,
  EVENT_APP_GET_NODELIST,
  EVENT_APP_IS_POWERING_DOWN,
  EVENT_BATT_LOW
} EVENT_APP;

typedef enum _STATE_APP_
{
  STATE_APP_STARTUP,
  STATE_APP_IDLE,
  STATE_APP_LEARN_MODE,
  STATE_APP_LOCAL_RESET,
  STATE_APP_OTA,
  STATE_APP_POWERDOWN,
  STATE_APP_NOTIFICATION_SEND
} STATE_APP;

/* The following values determines how long the sensor sleeps */
/* i.e. it sets the delay before next wakeup. This value */
/* is stored in 24-bits and is converted during execution */
/* depending on the chip. Default value is 10 minutes */
/* 0x258 = 600 seconds */
#define DEFAULT_SLEEP_TIME     0x012c //default 120 sec
#define MIN_SLEEP_TIME         0x003C  //min 60 sec
#define MAX_SLEEP_TIME         0x015180   // max 24 hours

#define STEP_SLEEP_TIME        0x003C  // step 60 sec.

#define NOTFICATION_TIME_WINDOW  30  // number of seconds used for making notfication frames avrage calculation

#define ST_NOTIFICATION_ASSOC_GROUP_IDLE 0
#define ST_NOTIFICATION_ASSOC_GROUP_SEND 1
#define ST_NOTIFICATION_ASSOC_GROUP_BUSY 2

#define nodeInfoForTransport nodeInfo
#define nodeInfoAfterIncluded nodeInfo



/* The following values determines how long the sensor sleeps */
/* i.e. it sets the delay before next wakeup. This value */
/* is stored in 24-bits and is converted during execution */
/* depending on the chip. Default value is 10 minutes */
/* 0x258 = 600 seconds */
#define DEFAULT_SLEEP_TIME_MSB     0x00 //default 120 sec
#define DEFAULT_SLEEP_TIME_M       0x01
#define DEFAULT_SLEEP_TIME_LSB     0x2c

#define MIN_SLEEP_TIME_MSB         0x00  //min 60 sec
#define MIN_SLEEP_TIME_M           0x00
#define MIN_SLEEP_TIME_LSB         0x3C

#define MAX_SLEEP_TIME_MSB         0x01   // max 24 hours
#define MAX_SLEEP_TIME_M           0x51
#define MAX_SLEEP_TIME_LSB         0x80

#define STEP_SLEEP_TIME_MSB        0x00  // step 60 sec.
#define STEP_SLEEP_TIME_M          0x00
#define STEP_SLEEP_TIME_LSB        0x3C



/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/



/* A list of the known command classes. Used when node info is send */
static code BYTE cmdClassListNonSecureNotIncluded[] =
{
  COMMAND_CLASS_ZWAVEPLUS_INFO,
  COMMAND_CLASS_ASSOCIATION,
  COMMAND_CLASS_ASSOCIATION_GRP_INFO,
  COMMAND_CLASS_VERSION,
  COMMAND_CLASS_MANUFACTURER_SPECIFIC,
  COMMAND_CLASS_DEVICE_RESET_LOCALLY,
  COMMAND_CLASS_POWERLEVEL,
  COMMAND_CLASS_BATTERY,
#ifdef SECURITY
  COMMAND_CLASS_SECURITY,
#endif
  COMMAND_CLASS_NOTIFICATION_V3,
  COMMAND_CLASS_WAKE_UP
#ifdef BOOTLOADER_ENABLED
  ,COMMAND_CLASS_FIRMWARE_UPDATE_MD_V2
#endif
};


/**
 * Unsecure node information list Secure included.
 * Be sure Command classes are not duplicated in both lists.
 * CHANGE THIS - Add all supported non-secure command classes here
 **/
static code BYTE cmdClassListNonSecureIncludedSecure[] =
{
#ifdef SECURITY
  COMMAND_CLASS_ZWAVEPLUS_INFO,
  COMMAND_CLASS_SECURITY
#else
  NULL
#endif
};


/**
 * Secure node inforamtion list.
 * Be sure Command classes are not duplicated in both lists.
 * CHANGE THIS - Add all supported secure command classes here
 **/
static code BYTE cmdClassListSecure[] =
{
#ifdef SECURITY
  COMMAND_CLASS_ASSOCIATION,
  COMMAND_CLASS_ASSOCIATION_GRP_INFO,
  COMMAND_CLASS_VERSION,
  COMMAND_CLASS_MANUFACTURER_SPECIFIC,
  COMMAND_CLASS_DEVICE_RESET_LOCALLY,
  COMMAND_CLASS_POWERLEVEL,
  COMMAND_CLASS_BATTERY,
  COMMAND_CLASS_NOTIFICATION_V3,
  COMMAND_CLASS_WAKE_UP
#ifdef BOOTLOADER_ENABLED
  ,COMMAND_CLASS_FIRMWARE_UPDATE_MD
#endif
#else
  NULL
#endif
};


/**
 * Structure includes application node information list's and device type.
 */
APP_NODE_INFORMATION m_AppNIF =
{
  cmdClassListNonSecureNotIncluded, sizeof(cmdClassListNonSecureNotIncluded),
  cmdClassListNonSecureIncludedSecure, sizeof(cmdClassListNonSecureIncludedSecure),
  cmdClassListSecure, sizeof(cmdClassListSecure),
  DEVICE_OPTIONS_MASK, GENERIC_TYPE, SPECIFIC_TYPE
};
CMD_CLASS_GRP  agiTableLifeLine[] = {AGITABLE_LIFELINE_GROUP};
AGI_GROUP agiTableRootDeviceGroups[] = {AGITABLE_ROOTDEVICE_GROUPS};

BYTE myNodeID = 0;

/*Handle only one event!*/
static EVENT_APP eventQueue = EVENT_EMPTY;

static STATE_APP currentState = STATE_APP_IDLE;
BYTE wakeupReason;
BOOL wakeupNotificationSend = FALSE;

NODE_LIST nList;

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/
void SetDefaultConfiguration(void);
void LoadConfiguration(void);
void ZCB_BattReportSentDone(BYTE txStatus);
void ZCB_BatteryLowReportDone(BYTE txStatus);
void ZCB_WatchdogCallback(void);
void ZCB_DeviceResetLocallyDone( BYTE status);
STATE_APP AppState();
void AppStateManager( EVENT_APP ev);
void ChangeState( STATE_APP st);
BOOL AddEvent(EVENT_APP ev);
void EventSchedularInit();
void EventSchedular(void);
JOB_STATUS NextUnsolicitedEvent(void);
void ZCB_JobStatus(BYTE bStatus);

#ifdef BOOTLOADER_ENABLED
void ZCB_OTAFinish(OTA_STATUS otaStatus);
BOOL ZCB_OTAStart();
#endif



/*===========================   ApplicationRfNotify   ===========================
**    Notify the application when the radio switch state
**    Called from the Z-Wave PROTOCOL When radio switch from Rx to Tx or from Tx to Rx
**    or when the modulator PA (Power Amplifier) turn on/off
**---------------------------------------------------------------------------------*/
void          /*RET Nothing */
ApplicationRfNotify(
  BYTE rfState)         /* IN state of the RF, the available values is as follow:
                               ZW_RF_TX_MODE: The RF switch from the Rx to Tx mode, the modualtor is started and PA is on
                               ZW_RF_PA_ON: The RF in the Tx mode, the modualtor PA is turned on
                               ZW_RF_PA_OFF: the Rf in the Tx mode, the modulator PA is turned off
                               ZW_RF_RX_MODE: The RF switch from Tx to Rx mode, the demodulator is started.*/
{

}


/*============================   ApplicationInitHW   ========================
**    Initialization of non Z-Wave module hardware
**
**    Side effects:
**       Returning FALSE from this function will force the API into
**       production test mode.
**--------------------------------------------------------------------------*/

BYTE                       /* RET TRUE        */
ApplicationInitHW(
  BYTE bWakeupReason)      /* IN  Nothing     */
{

  /* Setup Button S1 on the development board */
  /* CHANGE THIS - Set up your external hardware here */
  PIN_IN(P24, 1); /*s1 ZDP03A*/
  PIN_IN(P36, 1); /*s1 ZDP03A*/

  wakeupReason = bWakeupReason;
  InitBatteryMonitor(wakeupReason);
  Transport_OnApplicationInitHW(bWakeupReason);
  return(TRUE);
}



/*===========================   ApplicationInitSW   =========================
**    Initialization of the Application Software variables and states
**
**--------------------------------------------------------------------------*/
BYTE                      /*RET  TRUE       */
ApplicationInitSW( void ) /* IN   Nothing   */
{
  /* Init state machine*/
  currentState = STATE_APP_STARTUP;
  wakeupNotificationSend = FALSE;

  /* Do not reinitialize the UART if already initialized for ISD51 in ApplicationInitHW() */
#ifndef ZW_ISD51_DEBUG
  ZW_DEBUG_INIT(1152);
#endif

  ZW_DEBUG_SENSORPIR_SEND_NL();
  ZW_DEBUG_SENSORPIR_SEND_BYTE('A');
  ZW_DEBUG_SENSORPIR_SEND_BYTE('0');

#ifdef WATCHDOG_ENABLED
  ZW_WatchDogEnable();
#endif

  EventSchedularInit();

  /*Check battery level*/
  /*Check if battery Monitor has no state change and last time was battery state ST_BATT_DEAD*/
  if((FALSE == BatterySensorRead(NULL)) && (ST_BATT_DEAD ==BatteryMonitorState()))
  {
    ZW_DEBUG_SEND_STR("DEAD BATT! ");
    ZW_DEBUG_SEND_NL();
    ZW_DEBUG_SEND_STR("monitor ST: ");
    ZW_DEBUG_SEND_NUM(BatteryMonitorState());
    ZW_DEBUG_SEND_NL();
    ZW_DEBUG_SEND_NL();
    /*just power down! woltage to low.*/
    PowerDownNow();
    AppStateManager(EVENT_APP_IS_POWERING_DOWN);
  	return TRUE;
  }

  /* Get this sensors identification on the network */
  LoadConfiguration();

  /*it is not the time to wake completely*/
  if (FALSE == BatteryInit( BATT_MODE_NOT_LISTENING, wakeupReason))
  {
    ZW_DEBUG_SEND_NL();
    ZW_DEBUG_SEND_STR("TIME TO WAKEUP: NO!");
    ZW_DEBUG_SEND_NL();
    ZW_DEBUG_SEND_STR("monitor ST: ");
    ZW_DEBUG_SEND_NUM(BatteryMonitorState());
    ZW_DEBUG_SEND_NL();
    AppStateManager(EVENT_APP_IS_POWERING_DOWN);
    return (TRUE);
  }
  else{
    ZW_DEBUG_SEND_NL();
    ZW_DEBUG_SEND_STR("TIME TO WAKEUP");
    ZW_DEBUG_SEND_NL();
    ZW_DEBUG_SEND_STR("monitor ST: ");
    ZW_DEBUG_SEND_NUM(BatteryMonitorState());
    ZW_DEBUG_SEND_NL();

  }

  /* Initialize association module */
  AssociationInit(FALSE);
  /* Setup AGI group lists*/
  AGI_LifeLineGroupSetup(agiTableLifeLine, (sizeof(agiTableLifeLine)/sizeof(CMD_CLASS_GRP)));
  AGI_ResourceGroupSetup(agiTableRootDeviceGroups, (sizeof(agiTableRootDeviceGroups)/sizeof(AGI_GROUP)), 1);

  /* Init key manager*/
  InitKeyManager(AppStateManager, ZCB_SetPowerDownTimeout);

  InitNotification();
  AddNotification(NOTIFICATION_REPORT_BURGLAR_V3,NOTIFICATION_EVENT_HOME_SECURITY_MOTION_DETECTION_UNKNOWN_LOCATION, NULL, 0);
#ifdef BOOTLOADER_ENABLED
  /* Initialize OTA module */
  OtaInit( ZWAVE_PLUS_TX_OPTIONS, ZCB_OTAStart, ZCB_OTAFinish);
#endif


  Transport_OnApplicationInitSW( &m_AppNIF, &ZCB_SetPowerDownTimeoutWakeUpStateCheck);
  AppStateManager((EVENT_WAKEUP)wakeupReason);

  return(TRUE);
}


/*============================   ApplicationTestPoll   ======================
**    Function description
**      This function is called when the slave enters test mode.
**
**    Side effects:
**       Code will not exit until it is reset
**--------------------------------------------------------------------------*/
void
ApplicationTestPoll(void)
{
}

/*=============================  ApplicationPoll   =========================
**    Application poll function for the Battery operated Bin Sensor
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void                    /*RET  Nothing                  */
ApplicationPoll(void) /* IN  Nothing                  */
{

#ifdef WATCHDOG_ENABLED
  ZW_WatchDogKick(); /* Kick watchdog*/
#endif
  KeyScan();


  /* Check for event in queue*/
  EventSchedular();

}


/*========================   Transport_ApplicationCommandHandler   ====================
**    Handling of a received application commands and requests
**
**
**--------------------------------------------------------------------------*/
void                              /*RET Nothing                  */
Transport_ApplicationCommandHandler(
  BYTE  rxStatus,                 /* IN Frame header info */
  BYTE  sourceNode,               /* IN Command sender Node ID */
  ZW_APPLICATION_TX_BUFFER *pCmd, /* IN Payload from the received frame, the union */
                                  /*    should be used to access the fields */
  BYTE   cmdLength)               /* IN Number of command bytes including the command */
{
  BYTE txOption;

  /* Update txoptions with the low power flag from the received frame */
  txOption = ((rxStatus & RECEIVE_STATUS_LOW_POWER) ? TRANSMIT_OPTION_LOW_POWER : 0)
             | ZWAVE_PLUS_TX_OPTIONS;


  /* Call command class handlers */
  switch (pCmd->ZW_Common.cmdClass)
  {
    case COMMAND_CLASS_VERSION:
      handleCommandClassVersion(txOption, sourceNode, pCmd, cmdLength);
      break;

#ifdef BOOTLOADER_ENABLED
    case COMMAND_CLASS_FIRMWARE_UPDATE_MD_V2:
      handleCommandClassFWUpdate(txOption, sourceNode, pCmd, cmdLength);
      break;
#endif


    case COMMAND_CLASS_ASSOCIATION_GRP_INFO:
      handleCommandClassAssociationGroupInfo( txOption, sourceNode, pCmd, cmdLength);
      break;

    case COMMAND_CLASS_ASSOCIATION:
			handleCommandClassAssociation(txOption, sourceNode, pCmd, cmdLength);
      break;

    case COMMAND_CLASS_POWERLEVEL:
      handleCommandClassPowerLevel(txOption, sourceNode, pCmd, cmdLength);
      break;

    case COMMAND_CLASS_MANUFACTURER_SPECIFIC:
      handleCommandClassManufacturerSpecific(txOption, sourceNode, pCmd, cmdLength);
      break;

    case COMMAND_CLASS_ZWAVEPLUS_INFO:
      handleCommandClassZWavePlusInfo(txOption, sourceNode, pCmd, cmdLength);
      break;

   case COMMAND_CLASS_BATTERY:
      handleCommandClassBattery(txOption, sourceNode, pCmd, cmdLength);
      break;

    case COMMAND_CLASS_NOTIFICATION_V3:
      handleCommandClassNotification(txOption, sourceNode, pCmd, cmdLength);
      break;

    case COMMAND_CLASS_WAKE_UP:
      HandleCommandClassWakeUp(txOption, sourceNode, pCmd, cmdLength);
      break;
  }
}

BYTE handleCommandClassVersionAppl(BYTE cmdClass)
{
  switch (cmdClass)
  {
    case COMMAND_CLASS_VERSION:               return CommandClassVersionVersionGet();
    case COMMAND_CLASS_POWERLEVEL:            return CommandClassPowerLevelVersionGet();
    case COMMAND_CLASS_MANUFACTURER_SPECIFIC: return CommandClassManufacturerVersionGet();
    case COMMAND_CLASS_ASSOCIATION:           return CommandClassAssociationVersionGet();
    case COMMAND_CLASS_ASSOCIATION_GRP_INFO:  return CommandClassAssociationGroupInfoVersionGet();
    case COMMAND_CLASS_DEVICE_RESET_LOCALLY:  return CommandClassDeviceResetLocallyVersionGet();
    case COMMAND_CLASS_ZWAVEPLUS_INFO:        return CommandClassZWavePlusVersion();
    case COMMAND_CLASS_BASIC:                 return CommandClassBasicVersionGet();
    case COMMAND_CLASS_BATTERY:               return CommandClassBatteryVersionGet();
    case COMMAND_CLASS_NOTIFICATION_V3:       return CommandClassNotificationVersionGet();
    case COMMAND_CLASS_WAKE_UP:               return CmdClassWakeupVersion();
#ifdef BOOTLOADER_ENABLED
    case COMMAND_CLASS_FIRMWARE_UPDATE_MD:    return CommandClassFirmwareUpdateMdVersionGet();
#endif
#ifdef SECURITY
    case COMMAND_CLASS_SECURITY:              return CommandClassSecurityVersionGet();
#endif
    default:
     return UNKNOWN_VERSION;
  }
}


/*==========================   ApplicationSlaveUpdate   =======================
**   Inform a slave application that a node information is received.
**   Called from the slave command handler when a node information frame
**   is received and the Z-Wave protocol is not in a state where it is needed.
**
**--------------------------------------------------------------------------*/
void
ApplicationSlaveUpdate(
  BYTE bStatus,     /*IN  Status event */
  BYTE bNodeID,     /*IN  Node id of the node that send node info */
  BYTE* pCmd,       /*IN  Pointer to Application Node information */
  BYTE bLen)       /*IN  Node info length                        */
{
}



/*============================ handleNbrFirmwareVersions ===================
** Function description
** Read number of firmwares in application.
**
**-------------------------------------------------------------------------*/
BYTE
handleNbrFirmwareVersions(void)
{
  return 1; /*CHANGE THIS - firmware 0 version*/
}


/*============================ handleGetFirmwareVersion ====================
** Function description
** Read application firmware version informations
**
**-------------------------------------------------------------------------*/
void
handleGetFirmwareVersion( BYTE bFirmwareNumber, VG_VERSION_REPORT_V2_VG* pVariantgroup)
{
  /*firmware 0 version and sub version*/
  if(bFirmwareNumber == 0)
  {
    pVariantgroup->firmwareVersion = APP_VERSION;
    pVariantgroup->firmwareSubVersion = APP_REVISION;
  }
  else
  {
    /*Just set it to 0 if firmware n is not present*/
    pVariantgroup->firmwareVersion = 0;
    pVariantgroup->firmwareSubVersion = 0;
  }
}


/*============================   LearnCompleted   ========================
**    Callback which is called on learnmode completed
**  Application specific handling of LearnModeCompleted - called from
**  slave_learn.c
**--------------------------------------------------------------------------*/
void
LearnCompleted(BYTE bNodeID)                 /*IN The nodeID assigned*/
{
  ZW_DEBUG_SENSORPIR_SEND_STR("Learn complete ");
  ZW_DEBUG_SENSORPIR_SEND_NUM(bNodeID);
  ZW_DEBUG_SENSORPIR_SEND_NL();

  /*If bNodeID= 0xff.. learn mode failed*/
  if(bNodeID != NODE_BROADCAST)
  {
    /*Success*/
    myNodeID = bNodeID;
    if (myNodeID == 0)
    {
      SetDefaultConfiguration();
    }
    else
    {
      /*if we have been included to the network extened the wakeup time*/
      ZCB_SetPowerDownTimeout(LEARNMODE_POWERDOWNTIMEOUT);
    }
  }
  AppStateManager(EVENT_APP_LEARN_MODE_FINISH);
  if((bNodeID != NODE_BROADCAST) || (myNodeID == 0))
  {
    /* Dont' call transport layer if exclusion failed (nothing to do) */
    Transport_OnLearnCompleted(bNodeID);
  }
}


/*========================   GetMyNodeID   =================================
**    Get the device node ID
**
**   Side effects: none
**--------------------------------------------------------------------------*/
BYTE
GetMyNodeID(void)
{
  return myNodeID;
}


/*============================ AppState ===============================
** Function description
** Return application statemachine state
**
** Side effects:
**
**-------------------------------------------------------------------------*/
STATE_APP
AppState()
{
  return currentState;
}


/*============================ AppStateManager ===============================
** Function description
** This function...
**
** Side effects:
**
**-------------------------------------------------------------------------*/
void
AppStateManager( EVENT_APP ev)
{
  ZW_DEBUG_SENSORPIR_SEND_STR("AppStMan ev ");
  ZW_DEBUG_SENSORPIR_SEND_NUM(ev);
  ZW_DEBUG_SENSORPIR_SEND_STR(" st ");
  ZW_DEBUG_SENSORPIR_SEND_NUM(currentState);
  ZW_DEBUG_SENSORPIR_SEND_NL();


  switch(currentState)
  {

    case STATE_APP_STARTUP:
      if(EVENT_APP_IS_POWERING_DOWN == ev)
      {
        ChangeState(STATE_APP_POWERDOWN);
      }
      ChangeState(STATE_APP_IDLE);
      break;

    case STATE_APP_IDLE:

      if(EVENT_BATT_LOW == ev)
      {
        ZW_DEBUG_SENSORPIR_SEND_STR("Batt low");
        ChangeState(STATE_APP_NOTIFICATION_SEND);
      }

      if(EVENT_KEY_B0_PRESS == ev){}

      if(EVENT_KEY_B0_RELEASE == ev){}

      if(EVENT_KEY_B0_TRIPLE_PRESS == ev)
      {
        ZCB_SetPowerDownTimeout(LEARNMODE_POWERDOWNTIMEOUT);
        if (myNodeID){
          ZW_DEBUG_SENSORPIR_SEND_STR("LEARN_MODE_EXCLUSION");
          StartLearnModeNow(LEARN_MODE_EXCLUSION);
        }
        else{
          ZW_DEBUG_SENSORPIR_SEND_STR("LEARN_MODE_INCLUSION");

          /*Activate Wake-up state to setup security and other stuff.
            This gives 10 secund timeout for each command until controler send command "wake-up no more"*/
          ZCB_WakeUpStateSet(TRUE);

          StartLearnModeNow(LEARN_MODE_INCLUSION);
        }
        ChangeState(STATE_APP_LEARN_MODE);
      }

      if(EVENT_KEY_B0_HELD == ev)
      {
        ZW_DEBUG_SENSORPIR_SEND_STR("** DeviceResetLocally **");
        handleCommandClassDeviceResetLocally(ZCB_DeviceResetLocallyDone);
        ChangeState(STATE_APP_LOCAL_RESET);
      }

      if(EVENT_KEY_B1_PRESS == ev)
      {
        if(MemoryGetByte((WORD) & nvmApplDescriptor.alarmStatus_far))
        {
          AGI_PROFILE agiProfile = {ASSOCIATION_GROUP_INFO_REPORT_PROFILE_NOTIFICATION,
                                    NOTIFICATION_REPORT_BURGLAR_V3};

          NotificationEventTrigger(NOTIFICATION_REPORT_BURGLAR_V3,NOTIFICATION_EVENT_HOME_SECURITY_MOTION_DETECTION_UNKNOWN_LOCATION);
          AGI_NodeIdListInit(&nList, &agiProfile);
          ChangeState(STATE_APP_NOTIFICATION_SEND);
          AddEvent(EVENT_APP_GET_NODELIST);
        }
      }

      if(EVENT_APP_OTA_START == ev)
      {
        ChangeState(STATE_APP_OTA);
      }
      break;

    case STATE_APP_LEARN_MODE:
      if(EVENT_KEY_B0_TRIPLE_PRESS == ev)
      {
        ZW_DEBUG_SENSORPIR_SEND_STR("End LEARNMODE ");
        StartLearnModeNow(LEARN_MODE_DISABLE);
        ChangeState(STATE_APP_IDLE);
      }

      if( EVENT_APP_LEARN_MODE_FINISH == ev)
      {
        ChangeState(STATE_APP_IDLE);
      }
      break;

    case STATE_APP_LOCAL_RESET:
      //device reboot in this state by ZCB_CommandClassDeviceResetLocally
      break;

    case STATE_APP_OTA:
      /*OTA state... do nothing until firmware update is finish*/
      break;

    case STATE_APP_POWERDOWN:
      /* Device is powering down.. wait!*/
      break;

    case STATE_APP_NOTIFICATION_SEND:

      if(EVENT_APP_GET_NODELIST == ev)
      {
        if( TRUE == AGI_NodeIdListGetNext( &nList))
        {
          if(FALSE == AddEvent(EVENT_APP_NEXT_EVENT_JOB))
          {
            ZW_DEBUG_SENSORPIR_SEND_STR("** EVENT_APP_NEXT_EVENT_JOB fail");
            ZW_DEBUG_SENSORPIR_SEND_NUM(eventQueue);
            ZW_DEBUG_SENSORPIR_SEND_NL();
          }
        }
        else
        {
          if(FALSE == AddEvent(EVENT_APP_FINISH_EVENT_JOB))
          {
            ZW_DEBUG_SENSORPIR_SEND_STR("** EVENT_APP_FINISH_EVENT_JOB fail");
            ZW_DEBUG_SENSORPIR_SEND_NUM(eventQueue);
            ZW_DEBUG_SENSORPIR_SEND_NL();
            ChangeState(STATE_APP_IDLE);
          }
        }
      }

      if(EVENT_APP_NEXT_EVENT_JOB == ev)
      {
        if(JOB_STATUS_BUSY == NextUnsolicitedEvent())
        {
          ZCB_JobStatus(TRANSMIT_COMPLETE_FAIL);
        }
      }

      if(EVENT_APP_FINISH_EVENT_JOB == ev)
      {
        ClearLastNotificationAction();
        ChangeState(STATE_APP_IDLE);
      }
      break;
  }
}


/*============================ ChangeState ===============================
** Function description
** Change state
**
**-------------------------------------------------------------------------*/
void
ChangeState( STATE_APP st)
{
 ZW_DEBUG_SENSORPIR_SEND_STR("ChangeState st = ");
 ZW_DEBUG_SENSORPIR_SEND_NUM(currentState);
 ZW_DEBUG_SENSORPIR_SEND_STR(" -> new st = ");
 ZW_DEBUG_SENSORPIR_SEND_NUM(st);
 ZW_DEBUG_SENSORPIR_SEND_NL();

 currentState = st;
}


code const void (code * ZCB_DeviceResetLocallyDone_p)(BYTE txStatus) = &ZCB_DeviceResetLocallyDone;
/*==============================   ZCB_Done  ================================
**
**  Function:  callback function perform reset device
**
**  Side effects: None
**
**--------------------------------------------------------------------------*/
void
ZCB_DeviceResetLocallyDone(BYTE status)
{

  Transport_SetDefault();
  SetDefaultConfiguration();
  ZW_SetDefault();
  ZW_WatchDogEnable(); /*reset asic*/
  for (;;) {}
}


#ifdef BOOTLOADER_ENABLED
/*============================ OTA_Finish ===============================
** Function description
** OTA is finish.
**
** Side effects:
**
**-------------------------------------------------------------------------*/
code const void (code * ZCB_OTAFinish_p)(OTA_STATUS otaStatus) = &ZCB_OTAFinish;
void
ZCB_OTAFinish(OTA_STATUS otaStatus) /*Status on OTA*/
{

  /*Just reboot node to cleanup and start on new FW.*/
  ZW_WatchDogEnable(); /*reset asic*/
  while(1);
}

/*============================ OTA_Start ===============================
** Function description
** Ota_Util calls this function when firmware update is ready to start.
** Return FALSE if OTA should be rejected else TRUE
**
** Side effects:
**
**-------------------------------------------------------------------------*/
code const BOOL (code * ZCB_OTAStart_p)(void) = &ZCB_OTAStart;
BOOL   /*Return FALSE if OTA should be rejected else TRUE*/
ZCB_OTAStart()
{
  BOOL  status = FALSE;
  if (STATE_APP_IDLE == AppState())
  {
    AppStateManager(EVENT_APP_OTA_START);
    status = TRUE;
  }
  return status;
}
#endif


/*========================   handleBasicSetCommand  =========================
**   Application specific Basic Set Command handler
**   return none
**
**   Side effects: none
**--------------------------------------------------------------------------*/
void
handleBasicSetCommand(  BYTE val )
{
  /* Not used, only here because CommandClassBasic requires it */
}


/*========================   getAppBasicReport   ===========================
**    return the On / Off state
**
**   Side effects: none
**--------------------------------------------------------------------------*/
BYTE
getAppBasicReport(void)
{
  /* Used when sending unsolicited basic report */
  /* CHANGE THIS - Fill in your application code here */
  return 0;
}


/*========================   AppPowerDownReady  =========================
**   check if application is ready to go to power down state
**   return true if ready
**
**   Side effects: none
**--------------------------------------------------------------------------*/
BYTE AppPowerDownReady()
{
  BYTE status =FALSE;
  ZW_DEBUG_SENSORPIR_SEND_STR("AppPowerDownReady ");
  if (STATE_APP_IDLE == AppState())
  {
    /*Check battery before shut down*/
    if(TRUE == TimeToSendBattReport(ZWAVE_PLUS_TX_OPTIONS, ZCB_BattReportSentDone))
    {
      ZW_DEBUG_SENSORPIR_SEND_STR("EVENT_BATT_LOW");
      AddEvent(EVENT_BATT_LOW);
      /*Not ready to power of*/
      status = FALSE;
    }
    else if(((EVENT_WAKEUP_RESET == wakeupReason) ||
             (EVENT_WAKEUP_WUT == wakeupReason)) &&
             (FALSE == wakeupNotificationSend))
    {
      ZW_DEBUG_SENSORPIR_SEND_STR("NOTIFIC***");
      wakeupNotificationSend = TRUE;
      WakeUpNotification();
    }
    else{
      status = TRUE;
    }
  }
  ZW_DEBUG_SENSORPIR_SEND_NUM(status);
  ZW_DEBUG_SENSORPIR_SEND_NL();
  return status;
}



/*============================ NextUnsolicitedEvent ===============================
** Function description
** This function...
**
** Side effects:
**
**-------------------------------------------------------------------------*/
JOB_STATUS NextUnsolicitedEvent(void)
{
  ZW_DEBUG_SENSORPIR_SEND_STR("NextUnsolicitedEvent ");

  if((COMMAND_CLASS_BASIC == nList.pCurrentCmdGrp->cmdClass) && (BASIC_SET == nList.pCurrentCmdGrp->cmd))
  {
    ZW_DEBUG_SENSORPIR_SEND_STR("BASIC_SET node ");
    ZW_DEBUG_SENSORPIR_SEND_NUM(*nList.pCurrentNode);
    ZW_DEBUG_SENSORPIR_SEND_NL();
    return CmdClassBasicSetSend( ZWAVE_PLUS_TX_OPTIONS,
                                    *nList.pCurrentNode, 0xFF,
                                    ZCB_JobStatus);
  }
  else if((COMMAND_CLASS_NOTIFICATION_V3 == nList.pCurrentCmdGrp->cmdClass) && (NOTIFICATION_REPORT_V3 == nList.pCurrentCmdGrp->cmd))
  {
    BYTE notificationType, notificationEvent;
    if( TRUE == ReadLastNotificationAction(&notificationType, &notificationEvent))
    {
      ZW_DEBUG_SENSORPIR_SEND_STR("NOTIFICATION_REPORT node ");
      ZW_DEBUG_SENSORPIR_SEND_NUM(*nList.pCurrentNode);
      ZW_DEBUG_SENSORPIR_SEND_NL();
      return CmdClassNotificationReport( *nList.pCurrentNode,
                                         notificationType, notificationEvent,
                                         ZCB_JobStatus);
    }
  }
  else if((COMMAND_CLASS_BATTERY == nList.pCurrentCmdGrp->cmdClass) && (BATTERY_REPORT == nList.pCurrentCmdGrp->cmd))
  {
    ZW_DEBUG_SENSORPIR_SEND_STR("NOTIFICATION_REPORT node ");
    ZW_DEBUG_SENSORPIR_SEND_NUM(*nList.pCurrentNode);
    ZW_DEBUG_SENSORPIR_SEND_NL();
    return CmdClassBatteryReport( ZWAVE_PLUS_TX_OPTIONS, *nList.pCurrentNode, 0xff, ZCB_JobStatus);
  }
  else
  {
    ZW_DEBUG_SENSORPIR_SEND_STR("Error class ");
    ZW_DEBUG_SENSORPIR_SEND_NUM(nList.pCurrentCmdGrp->cmdClass);
    ZW_DEBUG_SENSORPIR_SEND_STR(" cmd ");
    ZW_DEBUG_SENSORPIR_SEND_NUM(nList.pCurrentCmdGrp->cmd);
  }
  return JOB_STATUS_BUSY;
}


code const void (code * ZCB_JobStatus_p)(BYTE txStatus) = &ZCB_JobStatus;
/*============================ ZCB_JobStatus  ===============================
** Function description
** This function...
**
** Side effects:
**
**-------------------------------------------------------------------------*/
void ZCB_JobStatus(BYTE bStatus)
{
  /*application do not take care of bStatus!*/
    AddEvent(EVENT_APP_GET_NODELIST);

}



code const void (code * ZCB_BattReportSentDone_p)(BYTE) = &ZCB_BattReportSentDone;
/*============================   ZCB_BattReportSentDone   ======================
**    Call back function used when sending battery report
**    Side effects:none
**--------------------------------------------------------------------------*/
void
ZCB_BattReportSentDone(BYTE txStatus)
{
  /*we only set the lowBattReportSent FALSE if we didn't got an ACK from the CSC node*/
  if ((TRANSMIT_COMPLETE_OK != txStatus) || (AssociationGetLifeLineNodeID() == 0xFF))
  {
    SetLowBattReport(FALSE);
  }
  if(FALSE == AddEvent(EVENT_APP_FINISH_EVENT_JOB))
  {
    ZW_DEBUG_SENSORPIR_SEND_STR("** something went wrong");
    ZW_DEBUG_SENSORPIR_SEND_NL();
  }
}


/*============================   SetDefaultConfiguration   ======================
**    Function resets configuration to default values.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void                   /*RET  Nothing       */
SetDefaultConfiguration(void)
{
  AssociationInit(TRUE);
  MemoryPutByte((WORD)&nvmApplDescriptor.alarmStatus_far, 0xFF);
  SetDefaultBatteryConfiguration(DEFAULT_SLEEP_TIME);
  MemoryPutByte((WORD)&nvmApplDescriptor.EEOFFSET_MAGIC_far, APPL_MAGIC_VALUE);
  CmdClassWakeUpNotificationMemorySetDefault();
}


/*============================   LoadConfiguration   ======================
**    This function loads the application settings from EEPROM.
**    If no settings are found, default values are used and saved.
**    Side effects:
**
**--------------------------------------------------------------------------*/
void                   /* RET  Nothing      */
LoadConfiguration(void)
{
  /*load the application configuration*/
  MemoryGetID(NULL, &myNodeID);
  ManufacturerSpecificDeviceIDInit();
  SetWakeUpConfiguration(WAKEUP_PAR_DEFAULT_SLEEP_TIME, DEFAULT_SLEEP_TIME);
  SetWakeUpConfiguration(WAKEUP_PAR_MAX_SLEEP_TIME, MAX_SLEEP_TIME);
  SetWakeUpConfiguration(WAKEUP_PAR_MIN_SLEEP_TIME, MIN_SLEEP_TIME);
  SetWakeUpConfiguration(WAKEUP_PAR_SLEEP_STEP, STEP_SLEEP_TIME);

  /* Check to see, if any valid configuration is stored in the EEPROM */
  if (MemoryGetByte((WORD)&nvmApplDescriptor.EEOFFSET_MAGIC_far) == APPL_MAGIC_VALUE)
  {
    loadStatusPowerLevel(ZCB_StopPowerDownTimer, ZCB_StartPowerDownTimer);
    /* There is a configuration stored, so load it */
    LoadBatteryConfiguration();
  }
  else
  {

    /* Initialize transport layer NVM */
    Transport_SetDefault();
    /* Reset protocol */
    ZW_SetDefault();
    loadInitStatusPowerLevel(ZCB_StopPowerDownTimer, ZCB_StartPowerDownTimer);
    /* Apparently there is no valid configuration in EEPROM, so load */
    /* default values and save them to EEPROM. */
    SetDefaultConfiguration();
  }
    /*Set PowerDown timeout */
    //ZCB_SetPowerDownTimeout(SEC_2_POWERDOWNTIMEOUT);
}

/*============================ AddEvent ===============================
** Function description
** Add event to queue. If return FALSE is queue full!
**
**-------------------------------------------------------------------------*/
BOOL
AddEvent(EVENT_APP ev)
{
  if(EVENT_EMPTY == eventQueue)
  {
    ZW_DEBUG_SENSORPIR_SEND_STR("AddEvent ev = ");
    ZW_DEBUG_SENSORPIR_SEND_NUM(ev);
    ZW_DEBUG_SENSORPIR_SEND_NL();
    eventQueue = ev;
    return TRUE;
  }
  else
  {

    ZW_DEBUG_SENSORPIR_SEND_STR("AddEvent FAILED: ev = ");
    ZW_DEBUG_SENSORPIR_SEND_NUM(ev);
    ZW_DEBUG_SENSORPIR_SEND_NL();
  }

  return FALSE;
}

/*============================ EventSchedularInit ===============================
** Function description
** Init queue
**
** Side effects:
**
**-------------------------------------------------------------------------*/
void
EventSchedularInit()
{
  eventQueue = EVENT_EMPTY;
}
/*============================ EventSchedular ===============================
** Function description
** Event handler... can only handle one event
**
**-------------------------------------------------------------------------*/
void
EventSchedular(void)
{
  if(EVENT_EMPTY != eventQueue)
  {
    BYTE tempEventQ = eventQueue;
    /*Empty queue before calling AppStateManager. AppStateManager can add new
      event in the queue*/
    eventQueue = EVENT_EMPTY;
    ZW_DEBUG_SENSORPIR_SEND_STR("EventSchedular ev = ");
    ZW_DEBUG_SENSORPIR_SEND_NUM(tempEventQ);
    ZW_DEBUG_SENSORPIR_SEND_NL();
    AppStateManager(tempEventQ);
  }
}
