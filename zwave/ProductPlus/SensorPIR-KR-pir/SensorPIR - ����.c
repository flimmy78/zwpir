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
#include <nvm_util.h>

/*IO control*/
#include <io_zdp03a.h>

#include <ZW_task.h>
#include <ev_man.h>
#include <ZW_timer_api.h>

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
#include <CommandClassBasic.h>

#include <CommandClassBattery.h>
#include <CommandClassNotification.h>

#include <CommandClassMultiChan.h>
#include <CommandClassMultiChanAssociation.h>
#include <CommandClassSupervision.h>
#include <notification.h>

#include <battery_plus.h>
#include <battery_monitor.h>

typedef enum _EVENT_APP_ {
  EVENT_EMPTY = DEFINE_EVENT_APP_NBR,
  EVENT_APP_INIT,
  EVENT_APP_REFRESH_MMI,
  EVENT_APP_NEXT_EVENT_JOB,
  EVENT_APP_FINISH_EVENT_JOB,
  EVENT_APP_BATT_LOW,
  EVENT_APP_IS_POWERING_DOWN,
  EVENT_APP_BASIC_STOP_JOB,
  EVENT_APP_BASIC_START_JOB,
  EVENT_APP_NOTIFICATION_START_JOB,
  EVENT_APP_NOTIFICATION_STOP_JOB,
  EVENT_APP_START_TIMER_EVENTJOB_STOP
} EVENT_APP;

typedef enum _STATE_APP_ {
  STATE_APP_STARTUP,
  STATE_APP_IDLE,
  STATE_APP_LEARN_MODE,
  STATE_APP_WATCHDOG_RESET,
  STATE_APP_OTA,
  STATE_APP_TRANSMIT_DATA,
  STATE_APP_POWERDOWN,
  STATE_APP_BASIC_SET_TIMEOUT_WAIT
} STATE_APP;

#define DEFAULT_SLEEP_TIME 300
#define MIN_SLEEP_TIME     60
#define MAX_SLEEP_TIME     86400 // 24 hours
#define STEP_SLEEP_TIME    60

static code BYTE cmdClassListNonSecureNotIncluded[] = {
  COMMAND_CLASS_ZWAVEPLUS_INFO,
  COMMAND_CLASS_ASSOCIATION,
  COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2,
  COMMAND_CLASS_ASSOCIATION_GRP_INFO,
  COMMAND_CLASS_TRANSPORT_SERVICE_V2,
  COMMAND_CLASS_VERSION,
  COMMAND_CLASS_MANUFACTURER_SPECIFIC,
  COMMAND_CLASS_DEVICE_RESET_LOCALLY,
  COMMAND_CLASS_POWERLEVEL,
  COMMAND_CLASS_BATTERY,
  COMMAND_CLASS_SECURITY_2,
  COMMAND_CLASS_NOTIFICATION_V3,
  COMMAND_CLASS_WAKE_UP,
  COMMAND_CLASS_SUPERVISION
#ifdef BOOTLOADER_ENABLED
  ,COMMAND_CLASS_FIRMWARE_UPDATE_MD_V2
#endif
};

static code BYTE cmdClassListNonSecureIncludedSecure[] = {
  COMMAND_CLASS_ZWAVEPLUS_INFO,
  COMMAND_CLASS_TRANSPORT_SERVICE_V2,
  COMMAND_CLASS_SECURITY_2
};

static code BYTE cmdClassListSecure[] = {
  COMMAND_CLASS_VERSION,
  COMMAND_CLASS_ASSOCIATION,
  COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2,
  COMMAND_CLASS_ASSOCIATION_GRP_INFO,
  COMMAND_CLASS_MANUFACTURER_SPECIFIC,
  COMMAND_CLASS_DEVICE_RESET_LOCALLY,
  COMMAND_CLASS_POWERLEVEL,
  COMMAND_CLASS_BATTERY,
  COMMAND_CLASS_NOTIFICATION_V3,
  COMMAND_CLASS_WAKE_UP,
  COMMAND_CLASS_SUPERVISION
#ifdef BOOTLOADER_ENABLED
  ,COMMAND_CLASS_FIRMWARE_UPDATE_MD_V2
#endif
};

APP_NODE_INFORMATION m_AppNIF = {
  cmdClassListNonSecureNotIncluded, sizeof(cmdClassListNonSecureNotIncluded),
  cmdClassListNonSecureIncludedSecure, sizeof(cmdClassListNonSecureIncludedSecure),
  cmdClassListSecure, sizeof(cmdClassListSecure),
  DEVICE_OPTIONS_MASK, GENERIC_TYPE, SPECIFIC_TYPE
};

const char GroupName[]   = "Lifeline";

CMD_CLASS_GRP  agiTableLifeLine[] = {AGITABLE_LIFELINE_GROUP};

AGI_GROUP agiTableRootDeviceGroups[] = {AGITABLE_ROOTDEVICE_GROUPS};

static const AGI_PROFILE lifelineProfile = {
    ASSOCIATION_GROUP_INFO_REPORT_PROFILE_GENERAL,
    ASSOCIATION_GROUP_INFO_REPORT_PROFILE_GENERAL_LIFELINE
};

BYTE myNodeID = 0;

static STATE_APP currentState = STATE_APP_IDLE;

SW_WAKEUP wakeupReason;

BOOL wakeupNotificationSend = FALSE;

static bTimerHandle_t eventJobsTimerHandle = 0xFF;

static BYTE suppportedEvents = NOTIFICATION_EVENT_HOME_SECURITY_MOTION_DETECTION_UNKNOWN_LOCATION;

static BYTE basicValue = 0x00;

#ifdef APP_SUPPORTS_CLIENT_SIDE_AUTHENTICATION
s_SecurityS2InclusionCSAPublicDSK_t sCSAResponse = { 0, 0, 0, 0};
#endif /* APP_SUPPORTS_CLIENT_SIDE_AUTHENTICATION */

void ZCB_BattReportSentDone(TRANSMISSION_RESULT * pTransmissionResult);
void ZCB_DeviceResetLocallyDone(TRANSMISSION_RESULT * pTransmissionResult);
void ZCB_ResetDelay(void);
STATE_APP GetAppState();
void AppStateManager( EVENT_APP event);
void ChangeState( STATE_APP newState);
void ZCB_JobStatus(TRANSMISSION_RESULT * pTransmissionResult);
void SetDefaultConfiguration(void);
void LoadConfiguration(void);

#ifdef BOOTLOADER_ENABLED
void ZCB_OTAFinish(OTA_STATUS otaStatus);
BOOL ZCB_OTAStart();
#endif

void ZCB_EventJobsTimer(void);

void
ApplicationRfNotify(BYTE rfState) {
  UNUSED(rfState);
}

BYTE ApplicationInitHW(SW_WAKEUP bWakeupReason) {
  wakeupReason = bWakeupReason;
  
  ZDP03A_InitHW(ZCB_EventSchedulerEventAdd, &ZCB_SetPowerDownTimeoutWakeUpStateCheck);
  SetPinIn(ZDP03A_KEY_1,TRUE);
  SetPinIn(ZDP03A_KEY_2,TRUE);
  
  SetPinOut(ZDP03A_LED_D1); /**< Learn mode indication*/
  Led(ZDP03A_LED_D1,OFF);

  InitBatteryMonitor(wakeupReason);
  Transport_OnApplicationInitHW(bWakeupReason);
  
  return(TRUE);
}

BYTE ApplicationInitSW(ZW_NVM_STATUS nvmStatus) {
  UNUSED(nvmStatus);

  currentState = STATE_APP_STARTUP;
  wakeupNotificationSend = FALSE;

#ifndef ZW_ISD51_DEBUG
  ZW_DEBUG_INIT(1152);
#endif
  ZW_DEBUG_SENSORPIR_SEND_NL();
  ZW_DEBUG_SENSORPIR_SEND_STR("AppInitSW ");
  ZW_DEBUG_SENSORPIR_SEND_NUM((BYTE)wakeupReason);

#ifdef WATCHDOG_ENABLED
  ZW_WatchDogEnable();
#endif
  EventSchedulerInit(AppStateManager);

  if((FALSE == BatterySensorRead(NULL)) && (ST_BATT_DEAD ==BatteryMonitorState()))  {
    ZW_DEBUG_SENSORPIR_SEND_NL();
    ZW_DEBUG_SENSORPIR_SEND_STR("DEAD BATT!");
    ZW_DEBUG_SENSORPIR_SEND_NL();
    ZW_DEBUG_SENSORPIR_SEND_NL();

    PowerDownNow();
    AppStateManager(EVENT_APP_IS_POWERING_DOWN);
    return TRUE;
  }

  LoadConfiguration();

  if (FALSE == BatteryInit( BATT_MODE_NOT_LISTENING, wakeupReason)) {
    ZW_DEBUG_SENSORPIR_SEND_NL();
    ZW_DEBUG_SENSORPIR_SEND_STR("Go to sleep!");
    ZW_DEBUG_SENSORPIR_SEND_NL();
    ZW_DEBUG_SENSORPIR_SEND_NL();
    ChangeState(STATE_APP_POWERDOWN);
    return(TRUE);
  }

  AGI_Init();
  AGI_LifeLineGroupSetup(agiTableLifeLine, (sizeof(agiTableLifeLine)/sizeof(CMD_CLASS_GRP)), GroupName, ENDPOINT_ROOT );
  AGI_ResourceGroupSetup(agiTableRootDeviceGroups, (sizeof(agiTableRootDeviceGroups)/sizeof(AGI_GROUP)), ENDPOINT_ROOT);


  InitNotification(); {
    AddNotification(
        &lifelineProfile,
        NOTIFICATION_TYPE_HOME_SECURITY,
        &suppportedEvents,
        1,
        FALSE,
        0);
  }
#ifdef BOOTLOADER_ENABLED
  OtaInit(ZCB_OTAStart, NULL,ZCB_OTAFinish);
#endif
  
  Transport_OnApplicationInitSW( &m_AppNIF, &ZCB_SetPowerDownTimeoutWakeUpStateCheck);

  ZCB_EventSchedulerEventAdd((EVENT_WAKEUP)wakeupReason);

  return(TRUE);
}

void ApplicationTestPoll(void) {
}

void ApplicationPoll(void) {

#ifdef WATCHDOG_ENABLED
  ZW_WatchDogKick();
#endif

  TaskApplicationPoll();
}

received_frame_status_t Transport_ApplicationCommandHandlerEx(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_APPLICATION_TX_BUFFER *pCmd,
  BYTE cmdLength) {
  
  received_frame_status_t frame_status = RECEIVED_FRAME_STATUS_NO_SUPPORT;
  ZW_DEBUG_SENSORPIR_SEND_NL();
  ZW_DEBUG_SENSORPIR_SEND_STR("TAppH");
  ZW_DEBUG_SENSORPIR_SEND_NUM(pCmd->ZW_Common.cmdClass);

  switch (pCmd->ZW_Common.cmdClass) {
    case COMMAND_CLASS_VERSION:
      frame_status = handleCommandClassVersion(rxOpt, pCmd, cmdLength);
      break;

#ifdef BOOTLOADER_ENABLED
    case COMMAND_CLASS_FIRMWARE_UPDATE_MD_V2:
      frame_status = handleCommandClassFWUpdate(rxOpt, pCmd, cmdLength);
      break;
#endif
      
    case COMMAND_CLASS_ASSOCIATION_GRP_INFO:
      frame_status = handleCommandClassAssociationGroupInfo( rxOpt, pCmd, cmdLength);
      break;

    case COMMAND_CLASS_ASSOCIATION:
			frame_status = handleCommandClassAssociation(rxOpt, pCmd, cmdLength);
      break;

    case COMMAND_CLASS_POWERLEVEL:
      frame_status = handleCommandClassPowerLevel(rxOpt, pCmd, cmdLength);
      break;

    case COMMAND_CLASS_MANUFACTURER_SPECIFIC:
      frame_status = handleCommandClassManufacturerSpecific(rxOpt, pCmd, cmdLength);
      break;

    case COMMAND_CLASS_ZWAVEPLUS_INFO:
      frame_status = handleCommandClassZWavePlusInfo(rxOpt, pCmd, cmdLength);
      break;

   case COMMAND_CLASS_BATTERY:
      frame_status = handleCommandClassBattery(rxOpt, pCmd, cmdLength);
      break;

    case COMMAND_CLASS_NOTIFICATION_V3:
      frame_status = handleCommandClassNotification(rxOpt, pCmd, cmdLength);
      break;

    case COMMAND_CLASS_WAKE_UP:
      HandleCommandClassWakeUp(rxOpt, pCmd, cmdLength);
      break;

    case COMMAND_CLASS_SUPERVISION:
      frame_status = handleCommandClassSupervision(rxOpt, pCmd, cmdLength);
      break;

    case COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2:
      frame_status = handleCommandClassMultiChannelAssociation(rxOpt, pCmd, cmdLength);
      break;
  }
  return frame_status;
}

BYTE handleCommandClassVersionAppl(BYTE cmdClass) {
  BYTE commandClassVersion = UNKNOWN_VERSION;

  switch (cmdClass) {
    case COMMAND_CLASS_VERSION:
     commandClassVersion = CommandClassVersionVersionGet();
      break;

#ifdef BOOTLOADER_ENABLED
    case COMMAND_CLASS_FIRMWARE_UPDATE_MD:
      commandClassVersion = CommandClassFirmwareUpdateMdVersionGet();
      break;
#endif

    case COMMAND_CLASS_POWERLEVEL:
     commandClassVersion = CommandClassPowerLevelVersionGet();
      break;

    case COMMAND_CLASS_MANUFACTURER_SPECIFIC:
     commandClassVersion = CommandClassManufacturerVersionGet();
      break;

    case COMMAND_CLASS_ASSOCIATION:
     commandClassVersion = CommandClassAssociationVersionGet();
      break;

    case COMMAND_CLASS_ASSOCIATION_GRP_INFO:
     commandClassVersion = CommandClassAssociationGroupInfoVersionGet();
      break;

    case COMMAND_CLASS_DEVICE_RESET_LOCALLY:
     commandClassVersion = CommandClassDeviceResetLocallyVersionGet();
      break;

    case COMMAND_CLASS_ZWAVEPLUS_INFO:
     commandClassVersion = CommandClassZWavePlusVersion();
      break;

    case COMMAND_CLASS_BATTERY:
      commandClassVersion = CommandClassBatteryVersionGet();
      break;

    case COMMAND_CLASS_NOTIFICATION_V3:
      commandClassVersion = CommandClassNotificationVersionGet();
      break;

    case COMMAND_CLASS_WAKE_UP:
      commandClassVersion = CmdClassWakeupVersion();
      break;

    case COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2:
      commandClassVersion = CmdClassMultiChannelAssociationVersion();
      break;

    case COMMAND_CLASS_SUPERVISION:
      commandClassVersion = CommandClassSupervisionVersionGet();
      break;

      /*
       * If both S0 & S2 are supported, ZW_Transport_CommandClassVersionGet can be used in default
       * instead of handling each of the CCs. Please see the other Z-Wave Plus apps for an example.
       *
       * In this case, S0 is not supported. Hence we must not return the version of S0 CC. Using
       * ZW_Transport_CommandClassVersionGet will return the version.
       */

    case COMMAND_CLASS_SECURITY_2:
      commandClassVersion = SECURITY_2_VERSION;
      break;

    case COMMAND_CLASS_TRANSPORT_SERVICE_V2:
      commandClassVersion = TRANSPORT_SERVICE_VERSION_V2;
      break;

    default:
     commandClassVersion = UNKNOWN_VERSION;
  }
  return commandClassVersion;
}

void ApplicationSlaveUpdate(
  BYTE bStatus,
  BYTE bNodeID,
  BYTE* pCmd,
  BYTE bLen) {
  UNUSED(bStatus);
  UNUSED(bNodeID);
  UNUSED(pCmd);
  UNUSED(bLen);
}

void LearnCompleted(BYTE bNodeID) {

  if(bNodeID != NODE_BROADCAST) {
    
    myNodeID = bNodeID;
    
    if (myNodeID == 0) {
      SetDefaultConfiguration();
    } else {

      ActivateBattNotificationTrigger();
      
      ZCB_WakeUpStateSet(TRUE);
    }
  }
  
  ZCB_EventSchedulerEventAdd((EVENT_APP) EVENT_SYSTEM_LEARNMODE_FINISH);
  Transport_OnLearnCompleted(bNodeID);
}

BYTE GetMyNodeID(void) {
  return myNodeID;
}

STATE_APP GetAppState(void) {
  return currentState;
}

void AppStateManager(EVENT_APP event) {
  
  ZW_DEBUG_SENSORPIR_SEND_NL();
  ZW_DEBUG_SENSORPIR_SEND_STR("AppStMan ev ");
  ZW_DEBUG_SENSORPIR_SEND_NUM(event);
  ZW_DEBUG_SENSORPIR_SEND_STR(" st ");
  ZW_DEBUG_SENSORPIR_SEND_NUM(currentState);

  if(EVENT_SYSTEM_WATCHDOG_RESET == event) {
      ChangeState(STATE_APP_WATCHDOG_RESET);
  }

  switch(currentState)  {

    case STATE_APP_STARTUP:
      ChangeState(STATE_APP_IDLE);
      ZCB_EventSchedulerEventAdd(EVENT_APP_REFRESH_MMI);
      break;

    case STATE_APP_IDLE:
      if(EVENT_APP_REFRESH_MMI == event)
      {
        Led(ZDP03A_LED_D1,OFF);
      }


      if((EVENT_KEY_B1_TRIPLE_PRESS == event) ||(EVENT_SYSTEM_LEARNMODE_START == event))
      {
        ZCB_SetPowerDownTimeout(LEARNMODE_POWERDOWNTIMEOUT);
        if (myNodeID){
          ZW_DEBUG_SENSORPIR_SEND_STR("LEARN_MODE_EXCLUSION");
          StartLearnModeNow(LEARN_MODE_EXCLUSION_NWE);
        }
        else{
          ZW_DEBUG_SENSORPIR_SEND_STR("LEARN_MODE_INCLUSION");
          StartLearnModeNow(LEARN_MODE_INCLUSION);
        }
        ChangeState(STATE_APP_LEARN_MODE);
      }

      if ((EVENT_KEY_B1_HELD_10_SEC == event) || (EVENT_SYSTEM_RESET == event))
      {
        /*
         * Since this application is a routing slave, it'll use the internal NVM also known as the
         * MTP. The MTP is getting flushed 300 ms after the latest write which means we'll have to
         * wait some time before resetting the device.
         */
        MemoryPutByte((WORD)&EEOFFSET_MAGIC_far, 1 + APPL_MAGIC_VALUE);
        ZW_TIMER_START(ZCB_ResetDelay, 50, 1); // 50 * 10 = 500 ms  to be sure.
      }

      if(EVENT_KEY_B2_PRESS == event)
      {
        ChangeState(STATE_APP_TRANSMIT_DATA);

        if(FALSE == ZCB_EventSchedulerEventAdd(EVENT_APP_NEXT_EVENT_JOB))
        {
          ZW_DEBUG_SENSORPIR_SEND_STR("** EVENT_APP_NEXT_EVENT_JOB fail");
          ZW_DEBUG_SENSORPIR_SEND_NL();
        }
        /*Add event's on job-queue*/
        ZCB_EventEnqueue(EVENT_APP_BASIC_START_JOB);
        ZCB_EventEnqueue(EVENT_APP_NOTIFICATION_START_JOB);
        ZCB_EventEnqueue(EVENT_APP_START_TIMER_EVENTJOB_STOP);
      }



      if(EVENT_SYSTEM_OTA_START == event)
      {
        ChangeState(STATE_APP_OTA);
      }


      break;

    case STATE_APP_LEARN_MODE:
      if(EVENT_APP_REFRESH_MMI == event)
      {
        Led(ZDP03A_LED_D1,ON);
      }

      if((EVENT_KEY_B1_TRIPLE_PRESS == event)||(EVENT_SYSTEM_LEARNMODE_END == event))
      {
        StartLearnModeNow(LEARN_MODE_DISABLE);
        ChangeState(STATE_APP_IDLE);
      }

      if(EVENT_SYSTEM_LEARNMODE_FINISH == event)
      {
        ChangeState(STATE_APP_IDLE);
      }
      break;

    case STATE_APP_WATCHDOG_RESET:
      if(EVENT_APP_REFRESH_MMI == event){}

      ZW_DEBUG_SENSORPIR_SEND_STR("STATE_APP_WATCHDOG_RESET");
      ZW_DEBUG_SENSORPIR_SEND_NL();
      ZW_WatchDogEnable(); /*reset asic*/
      for (;;) {}
      break;

    case STATE_APP_OTA:
      if(EVENT_APP_REFRESH_MMI == event){}
      /*OTA state... do nothing until firmware update is finish*/
      break;

    case STATE_APP_POWERDOWN:
      /* Device is powering down.. wait!*/
      break;

    case STATE_APP_TRANSMIT_DATA:

      if(EVENT_KEY_B2_PRESS == event)
      {
        if (0xFF != eventJobsTimerHandle)
        {
          ZW_TimerLongRestart(eventJobsTimerHandle);
        }
      }

      if(EVENT_APP_NEXT_EVENT_JOB == event)
      {
        BYTE event;
        /*check job-queue*/
        if(TRUE == ZCB_EventDequeue(&event))
        {
          /*Let the event scheduler fire the event on state event machine*/
          ZCB_EventSchedulerEventAdd(event);
        }
        else{
          ZCB_EventSchedulerEventAdd(EVENT_APP_FINISH_EVENT_JOB);
        }
      }

      if(EVENT_APP_BASIC_START_JOB == event)
      {
        if(JOB_STATUS_SUCCESS != CmdClassBasicSetSend( &agiTableRootDeviceGroups[0].profile, ENDPOINT_ROOT, BASIC_SET_TRIGGER_VALUE, ZCB_JobStatus))
        {
          basicValue = BASIC_SET_TRIGGER_VALUE;
          /*Kick next job*/
          ZCB_EventSchedulerEventAdd(EVENT_APP_NEXT_EVENT_JOB);
        }
      }

      if(EVENT_APP_BASIC_STOP_JOB == event)
      {
        if(JOB_STATUS_SUCCESS != CmdClassBasicSetSend( &agiTableRootDeviceGroups[0].profile, ENDPOINT_ROOT, 0, ZCB_JobStatus))
        {
          basicValue = 0;
          /*Kick next job*/
          ZCB_EventSchedulerEventAdd(EVENT_APP_NEXT_EVENT_JOB);
        }
      }


      if(EVENT_APP_NOTIFICATION_START_JOB == event)
      {
        ZW_DEBUG_SENSORPIR_SEND_STR("\r\nEVENT_APP_NOTIFICATION_START_JOB");
        NotificationEventTrigger(&lifelineProfile,
                                 suppportedEvents,
                                 NULL, 0,
                                 ENDPOINT_ROOT);
        if(JOB_STATUS_SUCCESS !=  UnsolicitedNotificationAction(&lifelineProfile, ENDPOINT_ROOT, ZCB_JobStatus))
        {
          /*Kick next job*/
          ZCB_EventSchedulerEventAdd(EVENT_APP_NEXT_EVENT_JOB);
        }
      }

      if(EVENT_APP_NOTIFICATION_STOP_JOB == event)
      {
        ZW_DEBUG_SENSORPIR_SEND_STR("\r\nEVENT_APP_NOTIFICATION_STOP_JOB");
        NotificationEventTrigger(&lifelineProfile,
                                 0,
                                 &suppportedEvents, 1,
                                 ENDPOINT_ROOT);
        if(JOB_STATUS_SUCCESS !=  UnsolicitedNotificationAction(&lifelineProfile, ENDPOINT_ROOT, ZCB_JobStatus))
        {
          /*Kick next job*/
          ZCB_EventSchedulerEventAdd(EVENT_APP_NEXT_EVENT_JOB);
        }
      }

      if( EVENT_APP_START_TIMER_EVENTJOB_STOP== event)
      {
         eventJobsTimerHandle = ZW_TimerLongStart(ZCB_EventJobsTimer, BASIC_SET_TIMEOUT, 1);
      }

      if(EVENT_APP_BATT_LOW == event)
      {
        if (JOB_STATUS_SUCCESS != SendBattReport( ZCB_BattReportSentDone ))
        {
          ActivateBattNotificationTrigger();
          ZCB_EventSchedulerEventAdd(EVENT_APP_NEXT_EVENT_JOB);
        }
      }

      if(EVENT_APP_FINISH_EVENT_JOB == event)
      {
          ZW_DEBUG_SENSORPIR_SEND_NL();
          ZW_DEBUG_SENSORPIR_SEND_STR("#EVENT_APP_FINISH_EVENT_JOB");
          ZW_DEBUG_SENSORPIR_SEND_NL();
        if (0xFF == eventJobsTimerHandle)
        {
           ChangeState(STATE_APP_IDLE);
        }
      }
      break;
  }
}
PCB(ZCB_DeviceResetLocallyDone)(TRANSMISSION_RESULT * pTransmissionResult) {
  if (TRANSMISSION_RESULT_FINISHED == pTransmissionResult->isFinished) {
    ZW_DEBUG_SENSORPIR_SEND_NL();
    ZW_DEBUG_SENSORPIR_SEND_STR("DRLD");

    ZCB_EventSchedulerEventAdd((EVENT_APP) EVENT_SYSTEM_WATCHDOG_RESET);
  }
}

PCB(ZCB_ResetDelay)(void) {
  AGI_PROFILE lifelineProfile = {
      ASSOCIATION_GROUP_INFO_REPORT_PROFILE_GENERAL,
      ASSOCIATION_GROUP_INFO_REPORT_PROFILE_GENERAL_LIFELINE
  };
  ZW_DEBUG_SENSORPIR_SEND_NL();
  ZW_DEBUG_SENSORPIR_SEND_STR("Call locally reset");

  handleCommandClassDeviceResetLocally(&lifelineProfile, ZCB_DeviceResetLocallyDone);

  ZW_DEBUG_SENSORPIR_SEND_NL();
  ZW_DEBUG_SENSORPIR_SEND_STR("Delay");
}


BYTE handleNbrFirmwareVersions(void) {
  return 1; /*CHANGE THIS - firmware 0 version*/
}

void handleGetFirmwareVersion(
  BYTE bFirmwareNumber,
  VG_VERSION_REPORT_V2_VG *pVariantgroup) {

  if(bFirmwareNumber == 0) {
    pVariantgroup->firmwareVersion = APP_VERSION;
    pVariantgroup->firmwareSubVersion = APP_REVISION;
  } else {
    pVariantgroup->firmwareVersion = 0;
    pVariantgroup->firmwareSubVersion = 0;
  }
}


WORD handleFirmWareIdGet( BYTE n) {
  if(n == 0)  {
    return APP_FIRMWARE_ID;
  }
  return 0;
}

PCB(ZCB_JobStatus)(TRANSMISSION_RESULT * pTransmissionResult) {
  if (TRANSMISSION_RESULT_FINISHED == pTransmissionResult->isFinished)  {
    ZCB_EventSchedulerEventAdd(EVENT_APP_NEXT_EVENT_JOB);
  }
}

void SetDefaultConfiguration(void) {
  AssociationInit(TRUE);
  MemoryPutByte((WORD)&EEOFFSET_alarmStatus_far, 0xFF);
  SetDefaultBatteryConfiguration(DEFAULT_SLEEP_TIME);
  MemoryPutByte((WORD)&EEOFFSET_MAGIC_far, APPL_MAGIC_VALUE);
  CmdClassWakeUpNotificationMemorySetDefault();
  ActivateBattNotificationTrigger();
}

void LoadConfiguration(void) {

  MemoryGetID(NULL, &myNodeID);
  ManufacturerSpecificDeviceIDInit();
  SetWakeUpConfiguration(WAKEUP_PAR_DEFAULT_SLEEP_TIME, DEFAULT_SLEEP_TIME);
  SetWakeUpConfiguration(WAKEUP_PAR_MAX_SLEEP_TIME,     MAX_SLEEP_TIME);
  SetWakeUpConfiguration(WAKEUP_PAR_MIN_SLEEP_TIME,     MIN_SLEEP_TIME);
  SetWakeUpConfiguration(WAKEUP_PAR_SLEEP_STEP,         STEP_SLEEP_TIME);

  if (MemoryGetByte((WORD)&EEOFFSET_MAGIC_far) == APPL_MAGIC_VALUE)
  {
    /* Initialize association module */
    AssociationInit(FALSE);

    loadStatusPowerLevel(ZCB_StopPowerDownTimer, ZCB_StartPowerDownTimer);
    /* There is a configuration stored, so load it */
    LoadBatteryConfiguration();
  } else {
    ZW_DEBUG_SENSORPIR_SEND_NL();
    ZW_DEBUG_SENSORPIR_SEND_STR("reset");
    /* Initialize transport layer NVM */
    Transport_SetDefault();
    /* Reset protocol */
    ZW_SetDefault();
    loadInitStatusPowerLevel(ZCB_StopPowerDownTimer, ZCB_StartPowerDownTimer);
    /* Apparently there is no valid configuration in EEPROM, so load */
    /* default values and save them to EEPROM. */
    SetDefaultConfiguration();
  }

}

BYTE AppPowerDownReady(void) {
  BYTE status =FALSE;
  ZW_DEBUG_SENSORPIR_SEND_STR("AppPowerDownReady ");
  if (STATE_APP_IDLE == GetAppState())
  {
    ZW_DEBUG_SENSORPIR_SEND_BYTE('a');
    /*Check battery before shut down*/
    if(TRUE == TimeToSendBattReport())
    {
      ZW_DEBUG_SENSORPIR_SEND_BYTE('b');
      ChangeState(STATE_APP_TRANSMIT_DATA);

      if(FALSE == ZCB_EventSchedulerEventAdd(EVENT_APP_NEXT_EVENT_JOB))
      {
        ZW_DEBUG_SENSORPIR_SEND_STR("** EVENT_APP_NEXT_EVENT_JOB fail");
        ZW_DEBUG_SENSORPIR_SEND_NL();
      }
      /*Add event's on job-queue*/
      ZCB_EventEnqueue(EVENT_APP_BATT_LOW);
      /*Not ready to power of*/
      status = FALSE;
    }
    else if(((SW_WAKEUP_RESET == wakeupReason) ||
             (SW_WAKEUP_WUT == wakeupReason)) &&
             (FALSE == wakeupNotificationSend))
    {
      ZW_DEBUG_SENSORPIR_SEND_STR("NOTIFIC***");
      wakeupNotificationSend = TRUE;
      WakeUpNotification();
    }
    else{
      ZW_DEBUG_SENSORPIR_SEND_BYTE('c');
      status = TRUE;
    }
  }
  ZW_DEBUG_SENSORPIR_SEND_NUM(status);
  ZW_DEBUG_SENSORPIR_SEND_NL();
  return status;
}

void handleBasicSetCommand(BYTE val, BYTE endpoint) {
  UNUSED(val);
  UNUSED(endpoint);
}

BYTE getAppBasicReport(BYTE endpoint) {
  UNUSED(endpoint);
  return basicValue;
}

BYTE getAppBasicReportTarget(BYTE endpoint) {
  UNUSED(endpoint);
  return 0;
}

BYTE getAppBasicReportDuration(BYTE endpoint) {
  UNUSED(endpoint);
  /* CHANGE THIS - Fill in your application code here */
  return 0;
}


PCB(ZCB_BattReportSentDone)(TRANSMISSION_RESULT * pTransmissionResult) {
  ZW_DEBUG_SENSORPIR_SEND_NL();
  ZW_DEBUG_SENSORPIR_SEND_NUM(pTransmissionResult->nodeId);
  ZW_DEBUG_SENSORPIR_SEND_NUM(pTransmissionResult->status);
  ZW_DEBUG_SENSORPIR_SEND_NUM(pTransmissionResult->isFinished);
  if (TRANSMIT_COMPLETE_OK != pTransmissionResult->status) {
    // If one of the nodes does not receive the battery status, we activate the trigger once again.
    ActivateBattNotificationTrigger();
  }

  if (TRANSMISSION_RESULT_FINISHED == pTransmissionResult->isFinished)
  {
    ZCB_EventSchedulerEventAdd(EVENT_APP_NEXT_EVENT_JOB);
  }
}

PCB(ZCB_EventJobsTimer)(void) {
  eventJobsTimerHandle = 0xFF;
  ZCB_EventEnqueue(EVENT_APP_NOTIFICATION_STOP_JOB);
  ZCB_EventEnqueue(EVENT_APP_BASIC_STOP_JOB);
  ZCB_EventSchedulerEventAdd(EVENT_APP_NEXT_EVENT_JOB);
}

void ApplicationSecurityEvent(
  s_application_security_event_data_t *securityEvent) {
  switch (securityEvent->event) {
#ifdef APP_SUPPORTS_CLIENT_SIDE_AUTHENTICATION
    case E_APPLICATION_SECURITY_EVENT_S2_INCLUSION_REQUEST_DSK_CSA:  {
        ZW_SetSecurityS2InclusionPublicDSK_CSA(&sCSAResponse);
      }
      break;
#endif /* APP_SUPPORTS_CLIENT_SIDE_AUTHENTICATION */

    default:
      break;
  }
}

BYTE ApplicationSecureKeysRequested(void) {
  return REQUESTED_SECURITY_KEYS;
}

BYTE ApplicationSecureAuthenticationRequested(void) {
  return REQUESTED_SECURITY_AUTHENTICATION;
}

