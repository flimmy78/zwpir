#include "config_app.h"

#include <slave_learn.h>
#include <ZW_slave_api.h>
#ifdef ZW_SLAVE_32
#include <ZW_slave_32_api.h>
#else
#include <ZW_slave_routing_api.h>
#endif 

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

#include <ZW_power_api.h>

#include <battery_plus.h>
#include <battery_monitor.h>

#include <led_control.h>
#include <ZW_power_api.h>

//,ZW_DEBUG_SENSORPIR,ZW_DEBUG,ZM5202,ZW_DEBUG_BATT


static code BYTE cmdClassListNonSecureNotIncluded[] = {
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
static code BYTE cmdClassListNonSecureIncludedSecure[] = {
#ifdef SECURITY
  COMMAND_CLASS_ZWAVEPLUS_INFO,
  COMMAND_CLASS_SECURITY
#else
  NULL
#endif
};
static code BYTE cmdClassListSecure[] = {
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

static APP_NODE_INFORMATION m_AppNIF = {
  cmdClassListNonSecureNotIncluded, sizeof(cmdClassListNonSecureNotIncluded),
  cmdClassListNonSecureIncludedSecure, sizeof(cmdClassListNonSecureIncludedSecure),
  cmdClassListSecure, sizeof(cmdClassListSecure),
  DEVICE_OPTIONS_MASK, GENERIC_TYPE, SPECIFIC_TYPE
};

static BYTE myNodeID = 0;
static BYTE mySleepTimer = 0xff;

void mySleepTimerFunc(void); 


void ApplicationRfNotify(BYTE rfState) {
}

BYTE  ApplicationInitHW(BYTE bWakeupReason) {
	LedControlInit();
	LedOn(2);
	
	//Transport_OnApplicationInitHW(bWakeupReason);
  return(TRUE);
}

BYTE ApplicationInitSW( void ) {
	BYTE ret;
	
#ifndef ZW_ISD51_DEBUG
  ZW_DEBUG_INIT(1152);
#endif
	
	ZW_DEBUG_SEND_STR("Wakeup\r\n");
	
	mySleepTimer =  ZW_TIMER_START(mySleepTimerFunc, 10 ,TIMER_FOREVER);  // 10 ms * 1
	//Transport_OnApplicationInitSW( &m_AppNIF, &ZCB_SetPowerDownTimeoutWakeUpStateCheck);	

	
  return(TRUE);
}

void ApplicationTestPoll(void) {

}

void ApplicationPoll(void) {
	
#ifdef WATCHDOG_ENABLED
  ZW_WatchDogKick(); 
#endif	
	//ZW_DEBUG_SEND_STR("ApplicationPoll\r\n");	
}

void Transport_ApplicationCommandHandler(BYTE  rxStatus, BYTE  sourceNode, ZW_APPLICATION_TX_BUFFER *pCmd, BYTE   cmdLength) {
	ZW_DEBUG_SEND_STR("LearnCompleted\r\n");	
}
	
BYTE handleCommandClassVersionAppl(BYTE cmdClass) {
	ZW_DEBUG_SEND_STR("handleCommandClassVersionAppl\r\n");	
	return UNKNOWN_VERSION;
}


BYTE handleNbrFirmwareVersions(void) {
	ZW_DEBUG_SEND_STR("handleNbrFirmwareVersions\r\n");	
  return 1; 
}

void handleGetFirmwareVersion( BYTE bFirmwareNumber, VG_VERSION_REPORT_V2_VG* pVariantgroup) {
	ZW_DEBUG_SEND_STR("handleGetFirmwareVersion\r\n");	
}

void handleBasicSetCommand(  BYTE val ) {
	ZW_DEBUG_SEND_STR("handleBasicSetCommand\r\n");
}

BYTE getAppBasicReport(void) {
	ZW_DEBUG_SEND_STR("getAppBasicReport\r\n");
  return 0;
}

void ApplicationSlaveUpdate(BYTE bStatus, BYTE bNodeID, BYTE* pCmd, BYTE bLen) {
	ZW_DEBUG_SEND_STR("ApplicationSlaveUpdate\r\n");
}

void LearnCompleted(BYTE bNodeID) {
	ZW_DEBUG_SEND_STR("LearnCompleted\r\n");
}

BYTE GetMyNodeID(void) {
	ZW_DEBUG_SEND_STR("GetMyNodeID\r\n");
  return myNodeID;
}

BYTE AppPowerDownReady() {
	ZW_DEBUG_SEND_STR("AppPowerDownReady\r\n");
	return TRUE;
}

PCB(mySleepTimerFunc)(void) {
	ZW_SetWutTimeout(30);
	ZW_DEBUG_SEND_STR("mySleepTimerFunc\r\n");
	if (!ZW_SetSleepMode(ZW_WUT_MODE,ZW_INT_MASK_EXT1,0)) {
		ZW_DEBUG_SEND_STR("sleep failed\r\n");
	} else {
		ZW_DEBUG_SEND_STR("Go to Sleep!\r\n");
		ZW_TIMER_CANCEL(mySleepTimerFunc);
		mySleepTimer = 0xff;
		LedOff(2);
	}
}

