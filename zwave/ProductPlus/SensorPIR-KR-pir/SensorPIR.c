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

#include <led_control.h>
#include <ZW_power_api.h>
#include <ZW_nvr_api.h>
#include <appl_timer_api.h>
#include <ZW_conbufio.h>

/////////////////////////////////////////////////////////////////////////////////////
// Debug Macro
//,ZW_DEBUG_SENSORPIR,ZW_DEBUG,ZM5202,ZW_DEBUG_BATT
//, ZM5202,ZW_DEBUG

/////////////////////////////////////////////////////////////////////////////////////
// Class Command
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

static APP_NODE_INFORMATION m_AppNIF = {
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

/////////////////////////////////////////////////////////////////////////////////////
// Task Def
typedef char (*TASK_FUNC)(void *);
typedef BYTE (*TASK_BRRK)(void *);
typedef BYTE (*TASK_TIME)(void *);
typedef struct _stTask {
	BYTE	type;
	BYTE	status;

	TASK_FUNC				func;
	TASK_BRRK				brrk;
	TASK_TIME				time;

	BYTE	param;
}stTask_t;

enum {
	TASK_LED			= 0,
	TASK_LEARN		= 1,
	TASK_RESET		= 2,
	TASK_MOTION		= 3,
	TASK_BATTERY	= 4,
	TASK_NONE			= 99,
};

enum {
	TS_NONE				= 0,
	TS_IDLE				= 99,

	/* led	*/
	TS_LED_ON			= 1,
	TS_LED_OFF		= 3,
	TS_LED_TOGGLE = 4,
	TS_LED_BLINK	= 5,

	/* learn */
	TS_LEARN_START			= 11,
	TS_LEARN_STARING		= 12,
	TS_LEARN_START_DONE = 13,

	/* reset */
	TS_RESET						= 21,
	TS_RESETING					= 22,
	TS_RESET_DONE				= 23,
	
	/* motion */
	TS_MOTION						= 31,
	TS_MOTIONING				= 32,
	TS_MOTION_DONE			= 33,

	/* battery */
	TS_BATTERY					= 41,
	TS_BATTERING				= 42,
	TS_BATTERY_DONE			= 43,
};

/* task in */
typedef BYTE (*FTASKIN)(BYTE *s);

/* App Env */
typedef struct _AppEnv {
	WORD HomeID;
	BYTE NodeID;
	BYTE RfFailCnt;
}stAppEnv_t;

/////////////////////////////////////////////////////////////////////////////////////
// Function
void mySleepTimerFunc(void); 
void mySetPowerDownTimeoutWakeUpStateCheck(BYTE timeout);

/* conf */
void conf_load(void);
void conf_set(WORD off, void *value, WORD len);
void conf_get(WORD off, void *value, WORD len);


/* misc */
BYTE misc_node_included();
void misc_nodeid_save();
void misc_nodeid_load();
BYTE misc_nodeid_get();
void misc_nodeid_set(BYTE id);

BYTE misc_rf_failcnt();
void misc_rf_failcnt_save();
void misc_rf_failcnt_load();
void misc_rf_failcnt_inc();
void misc_rf_failcnt_clr();


void misc_zw_init();

void misc_zw_send_motion();
void misc_zw_send_motion_done(char status);

void misc_zw_send_battery();
void misc_zw_send_battery_done(char status);

void misc_zw_learn();
void misc_zw_learn_done(char status);

void misc_zw_reset();
void misc_zw_reset_done(char status);

void misc_msg_wait_timeout();

/* task */
void task_set(BYTE t, BYTE s);
BYTE task_get_cnt();
BYTE task_must_wake();
BYTE task_has_rf_task();
BYTE task_get_min_task_time();
void task_in(void);
void task_do(void);

/* button */
BYTE btn_pressed();


/* check in */
static BYTE check_btn(BYTE *s);
static BYTE check_pir(BYTE *s);
static BYTE check_battery(BYTE *s);

/* functions */
static char func_led(void *);
static char func_learn(void *);
static char func_reset(void *);
static char func_motion(void *);
static char func_battery(void *);

static BYTE brrk_led(void *); 
static BYTE brrk_learn(void *); 
static BYTE brrk_reset(void *); 
static BYTE brrk_motion(void *); 
static BYTE brrk_battery(void *); 

static BYTE time_led(void *); 
static BYTE time_learn(void *); 
static BYTE time_reset(void *); 
static BYTE time_motion(void *); 
static BYTE time_battery(void *); 
/////////////////////////////////////////////////////////////////////////////////////
// Global Variables
static BYTE				mySleepTimer = 0xff;
static BYTE				myLearnTimer = 0xff;
static stAppEnv_t	myEnv = {
	0x00,
	0x00,
};
static stTask_t tasks[] = {
	/* task          status   func					brrk					time */
	{TASK_LED,			TS_NONE,	func_led,			brrk_led,			time_led			, 0},
	{TASK_LEARN,		TS_NONE,	func_learn,		brrk_learn,		time_learn		, 0},
	{TASK_RESET,		TS_NONE,	func_reset,		brrk_reset,		time_reset		, 0},
	{TASK_MOTION,		TS_NONE,	func_motion,	brrk_motion,	time_motion		, 0},
	{TASK_BATTERY,  TS_NONE,	func_battery,	brrk_battery,	time_battery	, 0},
};
static FTASKIN tasks_in[] = {
	check_btn, 
	check_pir,
	check_battery,
};

static NVR_FLASH_STRUCT nvs;
//static t_nvmApplDescriptor nvmappl;
//static t_nvmDescriptor nvmdesc;

static const char *sleep_mode_str[] = {
	"IDLE_MODE",
	"",
	"STOP_MODE",
	"WUT_MODE",
	"WUT_FAST_MODE",
	"FREQUENTILY_LISTENING_MODE",
};
static const char *sleep_mask_str[] = {
	"",
	"",
	"INT_MASK_EXT1",
};

static BYTE wakeupReason;
static const char *wakeup_reason_str[] = {
	"WAKEUP_RESET",					//0   /* Woken up by reset or external int */
	"WAKEUP_WUT",						//1   /* Woken up by the WUT timer */
	"WAKEUP_SENSOR",				//2   /* Woken up by a wakeup beam */
	"WAKEUP_WATCHDOG",			//3
	"WAKEUP_EXT_INT",				//4
	"WAKEUP_POR",						//5
	"WAKEUP_USB_SUSPEND",		//6
};

static BYTE				myPowerTimer = 0xff;
static BYTE myRfTimeout = 0;
static BYTE				myMsgWaitTimer = 0xff;
static BYTE	myMsgTimeout = 0;

static	BYTE v24 = 1;
static	BYTE v36 = 1;
static	BYTE	 v = 0x03;
static	BYTE	mo = 0;
static  BYTE batt = 0;

/* FE CMD1 CMD2 LEN DATA CHK */
enum {
  S_WAIT_HEAD,
  S_WAIT_CMD1,
  S_WAIT_CMD2,
  S_WAIT_LEN,
  S_WAIT_DATA,
  S_WAIT_CHECK,
};
#define MAX_FRAME_LEN 128
static BYTE frame[MAX_FRAME_LEN - 5];
static BYTE sts = 0;
static BYTE flen = 0;
static BYTE len = 0;
static BYTE rlen = 0;
static BYTE sum = 0;
static void SerialPollReset();
static BYTE SerialPoll();
static void SerialSendFrame(BYTE cmd1, BYTE cmd2, BYTE *da, BYTE len);
static void SerialSendStr(BYTE *str);
static void SerialSendNum(BYTE num);
static void SerialSendNl();
/* 
  name        	src       req            				res
  st wake zwave stm8      int1            			(0x01|0x80) 0x55
	include ok																		(0x02|0x80) 0x55
	exclude ok																		(0x03|0x80) 0x55

  btn press			stm8 		 0x41 0x55       				(0x41|0x80) 0x55
  has person    stm8     0x42 0x55       				(0x42|0x80) 0x55
  no  person    stm8     0x43 0x55       				(0x43|0x80) 0x55
*/	

#define SF_VERSION "1.0.0"


static BYTE suppportedEvents = NOTIFICATION_EVENT_HOME_SECURITY_MOTION_DETECTION_UNKNOWN_LOCATION;

#if 0
#define MY_DEBUG_SEND_STR(x)			SerialSendStr(x)
#define MY_DEBUG_SEND_NUM(x)			SerialSendNum(x)
#define MY_DEBUG_SEND_NL()		SerialSendNl()
#else 
#define MY_DEBUG_SEND_STR(x)			
#define MY_DEBUG_SEND_NUM(x)			
#define MY_DEBUG_SEND_NL()		
#endif


/////////////////////////////////////////////////////////////////////////////////////
// Initilize 
BYTE  ApplicationInitHW(BYTE bWakeupReason) {
	wakeupReason = bWakeupReason;

	//LedControlInit();
	//LedOff(2);

  InitBatteryMonitor(wakeupReason);

	//v24 = !!PIN_GET(P24);
	//v36 = !!PIN_GET(P36);
	//v		= (v36 << 1) | v24;
	v  = 0x03;

	Transport_OnApplicationInitHW(bWakeupReason);
  return(TRUE);
}

void misc_msg_wait_timeout() {
	ApplTimerStop(&myMsgWaitTimer);
	myMsgTimeout = 0;
}

BYTE ApplicationInitSW(ZW_NVM_STATUS nvmStatus) {
  UNUSED(nvmStatus);

#ifndef ZW_ISD51_DEBUG
  ZW_DEBUG_INIT(1152);
#endif

#ifdef WATCHDOG_ENABLED
  ZW_WatchDogEnable();
#endif

	//ZW_UART0_zm5202_mode_enable(TRUE);
  ZW_InitSerialIf(1152);
  ZW_FinishSerialIf();
	SerialPollReset();
	//MY_DEBUG_SEND_STR("Wakeup\r\n");
	if (wakeupReason == ZW_WAKEUP_EXT_INT) {
		SerialSendFrame(0x01|0x80, 0x55, 0, 0);
	}
	/*
	ZW_SerialPutByte(0xFE);
	ZW_SerialFlush();
	ZW_SerialPutByte(0x01);
	ZW_SerialFlush();
	ZW_SerialPutByte(0x55);
	ZW_SerialFlush();
	ZW_SerialPutByte(0x00);
	ZW_SerialFlush();
	ZW_SerialPutByte(0x54);
	ZW_SerialFlush();
	*/
	
	MY_DEBUG_SEND_STR("\r\nWakeup:");
	MY_DEBUG_SEND_STR(wakeup_reason_str[wakeupReason]);
	MY_DEBUG_SEND_STR("\r\n");
	MY_DEBUG_SEND_STR("SoftWare Version "SF_VERSION"\r\n");

	ApplTimerInit();

	conf_load();

	misc_zw_init();

	mySleepTimer		=  ZW_TIMER_START(mySleepTimerFunc, 1 ,TIMER_FOREVER);  // 10 ms * 1

	
	myMsgTimeout = 1;
	myMsgWaitTimer	=	 ApplTimerStart(misc_msg_wait_timeout, 1, 1);

	//Transport_OnApplicationInitSW( &m_AppNIF, &ZCB_SetPowerDownTimeoutWakeUpStateCheck);	
	Transport_OnApplicationInitSW( &m_AppNIF, &mySetPowerDownTimeoutWakeUpStateCheck);	
  return(TRUE);
}

/////////////////////////////////////////////////////////////////////////////////////
// Poll
void ApplicationPoll(void) {
	
#ifdef WATCHDOG_ENABLED
  ZW_WatchDogKick(); 

#endif	

	//while (ZW_SerialCheck()) {
	//	BYTE x =  ZW_SerialGetByte();
		//ZW_SerialPutByte(x);
	//	ZW_SerialFlush();  
	//	LedToggle(2);
	//}

	//MY_DEBUG_SEND_STR("ApplicationPoll\r\n");	
	//MY_DEBUG_SEND_STR("WakeUp\r\n");
	
	if (SerialPoll()) {
		switch (frame[1]) {
			case 0x41:
				v = 0x02;
			MY_DEBUG_SEND_STR("Btn Pressed\r\n");
				break;
			case 0x42:
				v = 0x01;
			MY_DEBUG_SEND_STR("Has Person\r\n");
				break;
			case 0x43:
				v = 0x00;
			MY_DEBUG_SEND_STR("No Person\r\n");
				break;
			default:
				break;
		}
		ApplTimerStop(&myMsgWaitTimer);
		myMsgTimeout = 0;
	}

	BatterySensorRead(NULL);
	batt = BatteryMonitorState();
	batt = (4-(batt%4))*25;
	mo	 = batt;
	
	task_in();
	task_do();
}
/////////////////////////////////////////////////////////////////////////////////////
// Complete More
void LearnCompleted(BYTE bNodeID) {
	stTask_t *t = NULL;
	MY_DEBUG_SEND_STR("LearnCompleted\r\n");

  MY_DEBUG_SEND_STR("Learn complete ");
  MY_DEBUG_SEND_NUM(bNodeID);
  MY_DEBUG_SEND_NL();

	//ApplTimerStop(&myLearnTimer);
	t = &tasks[TASK_LEARN];
	if (t->status == TS_LEARN_STARING) {
		t->status = TS_LEARN_START_DONE;
	} 
	t = &tasks[TASK_RESET];
	if (t->status == TS_RESETING) {
		t->status = TS_RESET_DONE;
	}

	if (bNodeID == NODE_BROADCAST) {
		return;
	}

	myEnv.NodeID = bNodeID;

	if (bNodeID == 0x00) {
		//SetDefaultConfiguration();
	} else {
	}

  Transport_OnLearnCompleted(bNodeID);
}



/////////////////////////////////////////////////////////////////////////////////////
// ZWave Cmd, RF Relatives
received_frame_status_t Transport_ApplicationCommandHandlerEx(
		RECEIVE_OPTIONS_TYPE_EX *rxOpt,
		ZW_APPLICATION_TX_BUFFER *pCmd,
		BYTE cmdLength) {
	received_frame_status_t frame_status = RECEIVED_FRAME_STATUS_NO_SUPPORT;

	MY_DEBUG_SEND_STR("Transport_ApplicationCommandHandlerEx\r\n");	

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

	MY_DEBUG_SEND_STR("handleCommandClassVersionAppl\r\n");	
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


BYTE handleNbrFirmwareVersions(void) {
	MY_DEBUG_SEND_STR("handleNbrFirmwareVersions\r\n");	
  return 1; 
}

void handleGetFirmwareVersion( BYTE bFirmwareNumber, VG_VERSION_REPORT_V2_VG* pVariantgroup) {
	MY_DEBUG_SEND_STR("handleGetFirmwareVersion\r\n");	

  if(bFirmwareNumber == 0) {
    pVariantgroup->firmwareVersion = APP_VERSION;
    pVariantgroup->firmwareSubVersion = APP_REVISION;
  } else {
    /*Just set it to 0 if firmware n is not present*/
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

void handleBasicSetCommand(  BYTE val, BYTE endpoint ) {
	MY_DEBUG_SEND_STR("handleBasicSetCommand\r\n");

}

BYTE getAppBasicReport(BYTE endpoint) {
	MY_DEBUG_SEND_STR("getAppBasicReport\r\n");

  return 0;
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


BYTE GetMyNodeID(void) {
	MY_DEBUG_SEND_STR("GetMyNodeID\r\n");
  return myEnv.NodeID;
}

void ApplicationSecurityEvent(s_application_security_event_data_t *securityEvent) {
	switch (securityEvent->event) {
#ifdef APP_SUPPORTS_CLIENT_SIDE_AUTHENTICATION
		case E_APPLICATION_SECURITY_EVENT_S2_INCLUSION_REQUEST_DSK_CSA:
			ZW_SetSecurityS2InclusionPublicDSK_CSA(&sCSAResponse);
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



/////////////////////////////////////////////////////////////////////////////////////
// Sleep 
PCB(mySleepTimerFunc)(void) {

	BYTE mode;
	BYTE mask;
	BYTE timeout;

	if (myRfTimeout != 0 || myMsgTimeout != 0) {
		//MY_DEBUG_SEND_STR("mySleepTimerFunc Failed, has rf Task.\r\n");
		return;
	}

	if (task_get_cnt() > 0 && task_must_wake()) {
		//has task to do, can do sleep
		//MY_DEBUG_SEND_STR("mySleepTimerFunc Failed, Task not permit!\r\n");
		return;
	}
	if (!misc_node_included()) {
		if (task_has_rf_task()) {
			//mode = ZW_FREQUENTLY_LISTENING_MODE;
			//MY_DEBUG_SEND_STR("mySleepTimerFunc Has Rf task(not include), Can't Sleep\r\n");
			return;	/* can't sleep */
		} else {
			mode = ZW_STOP_MODE;
			/* test for no button from remote */
			//mode = ZW_WUT_MODE;
		}
	} else {
		if (task_has_rf_task()) {
			//mode = ZW_FREQUENTLY_LISTENING_MODE;
			//MY_DEBUG_SEND_STR("mySleepTimerFunc Has Rf task(include), Can't Sleep\r\n");
			return; /* can't sleep */
		} else {
			mode = ZW_WUT_MODE;
		}
	}
	mask = ZW_INT_MASK_EXT1;
	
	timeout = task_get_min_task_time();
	if (misc_node_included() && misc_rf_failcnt() > 30) {
		timeout = 0xff - 1;
	}
	if (timeout == 0) {
		//MY_DEBUG_SEND_STR("mySleepTimerFunc Timeout 0, Not Sleep\r\n");
		return;
	}

	ZW_SetWutTimeout(timeout);
	if (!ZW_SetSleepMode(mode,mask,0)) {
		//MY_DEBUG_SEND_STR("sleep failed\r\n");
		return;
	} 

	ZW_TIMER_CANCEL(mySleepTimerFunc);
	mySleepTimer = 0xff;
	//LedOn(2);


	MY_DEBUG_SEND_STR("Go to sleep with mode:");
	//MY_DEBUG_SEND_NUM(mode);
	MY_DEBUG_SEND_STR(sleep_mode_str[mode]);
	MY_DEBUG_SEND_STR(",mask:");
	//MY_DEBUG_SEND_NUM(mask);
	MY_DEBUG_SEND_STR(sleep_mask_str[mask]);
	MY_DEBUG_SEND_STR(",timeout:");
	MY_DEBUG_SEND_NUM(timeout);
	MY_DEBUG_SEND_STR("\r\n");
	MY_DEBUG_SEND_STR("Sleep\r\n");
}


void misc_zw_setpowerdown_timeout() {
	ApplTimerStop(&myPowerTimer);
	myRfTimeout = 0;
}
PCB(mySetPowerDownTimeoutWakeUpStateCheck)(BYTE timeout) {
	MY_DEBUG_SEND_STR("SetPowerDownTimeoutWakeupStateCheck:");
	MY_DEBUG_SEND_NUM(timeout);
	MY_DEBUG_SEND_STR("\r\n");

	ApplTimerStop(&myPowerTimer);
	if (timeout > 2) {
		timeout = 2;
	}
	myPowerTimer = ApplTimerStart(misc_zw_setpowerdown_timeout, timeout, 1);
	myRfTimeout = !!timeout;
}

/////////////////////////////////////////////////////////////////////////////////////
// flash
void conf_set(WORD off, void *value, WORD len) {
	ZW_MemoryPutBuffer(off,  (BYTE*)value, len);
}
void conf_get(WORD off,  void *value, WORD len) {
	MemoryGetBuffer(off,  (BYTE*)value, len);
}
void conf_load(void) {
	BYTE i;

	ZW_NVRGetValue(0, sizeof(NVR_FLASH_STRUCT), &nvs);

	MY_DEBUG_SEND_STR("Revision:");
	MY_DEBUG_SEND_NUM(nvs.bRevision);
	MY_DEBUG_SEND_STR(",SAWBandWidth:");
	MY_DEBUG_SEND_NUM(nvs.bSAWBandwidth);
	MY_DEBUG_SEND_STR(",UUID:");
	for (i = 0; i < 8; i++) {
		MY_DEBUG_SEND_NUM(nvs.abUUID[i]);
	}
	MY_DEBUG_SEND_STR("\r\n");
	ManufacturerSpecificDeviceIDInit();

	//MemoryGetBuffer((WORD)&nvmApplDescriptor, &nvmappl, sizeof(nvmappl));
	//MemoryGetBuffer((WORD)&nvmDescriptor, &nvmdesc, sizeof(nvmdesc));

	misc_rf_failcnt_load();
	misc_nodeid_load();

	MY_DEBUG_SEND_STR("nodeid:");
	MY_DEBUG_SEND_NUM(myEnv.NodeID);
	MY_DEBUG_SEND_STR(",RfFailCnt:");
	MY_DEBUG_SEND_NUM(myEnv.RfFailCnt);
	MY_DEBUG_SEND_STR("\r\n");
}

/////////////////////////////////////////////////////////////////////////////////////
// misk
BYTE misc_node_included() {
	return (myEnv.NodeID != 0x00);
}
void misc_nodeid_save() {
	//
}
void misc_nodeid_load() {
	MemoryGetID(NULL, &myEnv.NodeID);
}
BYTE misc_nodeid_get() {
	return (myEnv.NodeID);
}
void misc_nodeid_set(BYTE id) {
	myEnv.NodeID = id;
	misc_nodeid_save();
}

BYTE misc_rf_failcnt() {
	return (myEnv.RfFailCnt);
}
void misc_rf_failcnt_save() {
	//conf_set((WORD)&nvmApplDescriptor.RfFailCnt, &myEnv.RfFailCnt, 1);
}
void misc_rf_failcnt_load() {
	//conf_get((WORD)&nvmApplDescriptor.RfFailCnt, &myEnv.RfFailCnt, 1);
}
void misc_rf_failcnt_inc() {
	if (myEnv.RfFailCnt < 0xff) {
		myEnv.RfFailCnt++;
	}
	misc_rf_failcnt_save();
}
void misc_rf_failcnt_clr() {
	myEnv.RfFailCnt = 0;
	misc_rf_failcnt_save();
}
void misc_zw_send_motion_completed(BYTE bStatus) {
  /*application do not take care of bStatus!*/
  //AddEvent(EVENT_APP_GET_NODELIST);
	stTask_t *t = &tasks[TASK_MOTION];
	MY_DEBUG_SEND_STR("misc_zw_send_motion_completed\r\n");
	t->status = TS_MOTION_DONE;
}
void misc_zw_send_motion() {
	BYTE notificationType		= NOTIFICATION_REPORT_BURGLAR_V3;
	BYTE notificationEvent	= NOTIFICATION_EVENT_HOME_SECURITY_MOTION_DETECTION_UNKNOWN_LOCATION;
	JOB_STATUS ret = 0;

	MY_DEBUG_SEND_STR("misc_zw_send_motion:");
	MY_DEBUG_SEND_NUM(mo);
	MY_DEBUG_SEND_STR(",");
	MY_DEBUG_SEND_NUM(notificationType);
	MY_DEBUG_SEND_STR(",");
	MY_DEBUG_SEND_NUM(notificationEvent);
	MY_DEBUG_SEND_STR("\r\n");

	NotificationEventTrigger(&lifelineProfile,
			suppportedEvents,
			&mo, 1,
			ENDPOINT_ROOT);
  ret = CmdClassNotificationReport(&lifelineProfile, 0x00, notificationType, notificationEvent,misc_zw_send_motion_completed);

	MY_DEBUG_SEND_STR("misc_zw_send_motion ret:");
	MY_DEBUG_SEND_NUM(ret);
	MY_DEBUG_SEND_STR("\r\n");
}
void misc_zw_send_motion_done(char status) {
	MY_DEBUG_SEND_STR("misc_zw_send_motion_done\r\n");
	ApplTimerStop(&myPowerTimer);
	myRfTimeout = 0;
}

void misc_zw_send_battery() {
	MY_DEBUG_SEND_STR("misc_zw_send_battery\r\n");
	/* TODO */
}
void misc_zw_send_battery_done(char status) {
	MY_DEBUG_SEND_STR("misc_zw_send_battery_done\r\n");
}
void misc_zw_learn_timeout() {
	stTask_t *t = &tasks[TASK_LEARN];

	ApplTimerStop(&myLearnTimer);
	StartLearnModeNow(LEARN_MODE_DISABLE);

	if (t->status == TS_LEARN_STARING) {
		t->status = TS_LEARN_START_DONE;
	} 
}
void misc_zw_learn() {
	MY_DEBUG_SEND_STR("misc_zw_learn\r\n");
  StartLearnModeNow(LEARN_MODE_INCLUSION);

	//myLearnTimer = ApplTimerStart(misc_zw_learn_timeout, 3, 1);
}
void misc_zw_learn_done(char status) {
	MY_DEBUG_SEND_STR("misc_zw_learn_done(nodeid:");
	MY_DEBUG_SEND_NUM(myEnv.NodeID);
	MY_DEBUG_SEND_STR(")\r\n");
	
	if (myEnv.NodeID != 0x00) {
		SerialSendFrame(0x02 | 0x80, 0x55, 0, 0);
	} else {
		SerialSendFrame(0x03 | 0x80, 0x55, 0, 0);
	}
}

void misc_zw_reset_timeout() {
	stTask_t *t = &tasks[TASK_RESET];

	ApplTimerStop(&myLearnTimer);
	StartLearnModeNow(LEARN_MODE_DISABLE);

	if (t->status == TS_RESETING) {
		t->status = TS_RESET_DONE;
	} 
}
void misc_zw_reset() {
	MY_DEBUG_SEND_STR("misc_zw_reset\r\n");
  StartLearnModeNow(LEARN_MODE_EXCLUSION);

	//myLearnTimer = ApplTimerStart(misc_zw_reset_timeout, 3, 1);
}
void misc_zw_reset_done(char status) {
	MY_DEBUG_SEND_STR("misc_zw_reset_done(nodeid:");
	MY_DEBUG_SEND_NUM(myEnv.NodeID);
	MY_DEBUG_SEND_STR(")\r\n");

	if (myEnv.NodeID != 0x00) {
		SerialSendFrame(0x02 | 0x80, 0x55, 0, 0);
	} else {
		SerialSendFrame(0x03 | 0x80, 0x55, 0, 0);
	}
}


void misc_zw_set_default() {
  AssociationInit(TRUE);

  //MemoryPutByte((WORD)&nvmApplDescriptor.alarmStatus_far, 0xFF);

  //SetDefaultBatteryConfiguration(DEFAULT_SLEEP_TIME);

  //MemoryPutByte((WORD)&nvmApplDescriptor.EEOFFSET_MAGIC_far, APPL_MAGIC_VALUE);

	/* notifaction dest */
  //CmdClassWakeUpNotificationMemorySetDefault();
}
void misc_zw_init() {

	if (misc_node_included()) {
	} else {
    Transport_SetDefault();
		misc_zw_set_default();
	}

	AssociationInit(FALSE);

	AGI_Init();
	AGI_LifeLineGroupSetup(agiTableLifeLine, (sizeof(agiTableLifeLine)/sizeof(CMD_CLASS_GRP)), GroupName, ENDPOINT_ROOT );
	AGI_ResourceGroupSetup(agiTableRootDeviceGroups, (sizeof(agiTableRootDeviceGroups)/sizeof(AGI_GROUP)), ENDPOINT_ROOT);

  InitNotification();
	AddNotification(
			&lifelineProfile,
			NOTIFICATION_TYPE_HOME_SECURITY,
			&suppportedEvents,
			1,
			FALSE,
			0);
}

/////////////////////////////////////////////////////////////////////////////////////
// task
void task_set(BYTE t, BYTE s) {
	if (t == TASK_NONE) {
		return;
	}
	tasks[t].status = s;
}
BYTE task_get_cnt() {
	BYTE i		= 0;
	BYTE cnt	= 0;
	for (i = 0; i < sizeof(tasks)/sizeof(tasks[0]); i++) {
		stTask_t *t = &tasks[i];
		if (t->status != TASK_NONE) {
			cnt++;
		}
	}
	return cnt;
}
BYTE task_must_wake() {
	BYTE i		= 0;
	for (i = 0; i < sizeof(tasks)/sizeof(tasks[0]); i++) {
		stTask_t *t = &tasks[i];
		if (t->status == TS_NONE || t->status == TS_IDLE) {
			continue;
		}

		if (t->time != NULL && t->time(t) == 0) {
			return !0;
		}
	}
	return 0;
}
BYTE task_has_rf_task() {
	BYTE i		= 0;
	for (i = 0; i < sizeof(tasks)/sizeof(tasks[0]); i++) {
		stTask_t *t = &tasks[i];
		if (t->status == TS_NONE || t->status == TS_IDLE) {
			continue;
		}

		if (t->type == TASK_LEARN ||
				t->type == TASK_RESET ||
				t->type == TASK_MOTION ||
				t->type == TASK_BATTERY) {
			return !0;
		}
	}
	return 0;
}

BYTE task_get_min_task_time() {
	BYTE i		= 0;
	BYTE min  = 0xff;

	for (i = 0; i < sizeof(tasks)/sizeof(tasks[0]); i++) {
		stTask_t *t = &tasks[i];
		if (t->status == TS_NONE) { // can't dec min
			continue;
		}
		if (t->status == TS_IDLE) { // can't dec min
			continue;
		}
		
		if (t->time == NULL) {
			continue;
		}
		if (t->time(t) < min) {
			min = t->time(t);
		}
	}

	return min;
}
void task_in() {
	BYTE i = 0;
	for (i = 0; i < sizeof(tasks_in)/sizeof(tasks_in[0]); i++) {
		BYTE t = 0;
		BYTE s = 0;
		FTASKIN func = tasks_in[i];
		if (func == NULL) {
			continue;
		}

		t = func(&s);
		if (t == TASK_NONE) {
			continue;
		}

		task_set(t, s);
	}
}
void task_do() {
	BYTE i		= 0;	
	char ret	= 0;
	for (i = 0; i < sizeof(tasks)/sizeof(tasks[0]); i++) {
		stTask_t *t = &tasks[i];
		if (t->status == TS_IDLE) {
			t->status = TS_NONE;
		}

		if (t->status == TS_NONE) {
			continue;
		}

		if (t->func == NULL) {
			continue;
		}

		ret = t->func(t);
		if (ret != 0) {
			MY_DEBUG_SEND_STR("task run failed with :");
			MY_DEBUG_SEND_NUM(ret);
			MY_DEBUG_SEND_STR("\r\n");
			continue;
		}

		if (t->brrk == NULL) {
			continue;
		}
		if (t->brrk(t)) {
			break;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////
// btn pressed
BYTE btn_pressed() {
#if 0
	static int x = 1;
#endif

	//BYTE v24 = !!PIN_GET(P24);
	//BYTE v36 = !!PIN_GET(P36);
	//BYTE v   = (v24 << 1) | v36;

	/*
	if (v != 0x03) {
		MY_DEBUG_SEND_STR("Btn:");
		MY_DEBUG_SEND_NUM(v);
		MY_DEBUG_SEND_STR("\r\n");
	}
	*/

	if (v == 0x02) {
		MY_DEBUG_SEND_STR("Key Pressed:");
		MY_DEBUG_SEND_NUM(v);
		MY_DEBUG_SEND_STR("\r\n");
		v = 0x03;
		return !0;
	}

#if 0 /* test from remote no button to be pressed */
	if (x == 1) {
		x = 0;
		return 1;
	}
	return 0;
#else
	return 0;
#endif
}

// pir pressed
BYTE pir_triger() {
	if (v == 0x00) {
		MY_DEBUG_SEND_STR("No Person Signal:");
		MY_DEBUG_SEND_NUM(v);
		MY_DEBUG_SEND_STR("\r\n");
		v = 0x03;
		return 0x01;
	} else if (v == 0x01) {
		MY_DEBUG_SEND_STR("Has Person Signal:");
		MY_DEBUG_SEND_NUM(v);
		MY_DEBUG_SEND_STR("\r\n");
		v = 0x03;
		return 0x02;
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////
// check in 
static BYTE check_btn(BYTE *s) {
	if (btn_pressed()) {
		if (!misc_node_included()) {
			stTask_t *t = &tasks[TASK_LEARN];
			if (t->status == TS_NONE) { 
				*s = TS_LEARN_START;
				return TASK_LEARN;
			}
		} else {
			stTask_t *t = &tasks[TASK_RESET];
			if (t->status == TS_NONE) {
				*s = TS_RESET;
				return TASK_RESET;
			}
		}
	}

	return TASK_NONE;
}
static BYTE check_pir(BYTE *s) {
	BYTE pir = pir_triger();


	if (pir) {
		stTask_t *t = &tasks[TASK_MOTION];
		if (pir == 0x01) {
			mo |= 0x0;
		} else if (pir == 0x02) {
			mo |= 0x80;
		}

		if (!misc_node_included()) {
			MY_DEBUG_SEND_STR("Not Inlcuded, Do't deal Motion Signal:");
			MY_DEBUG_SEND_NUM(mo);
			MY_DEBUG_SEND_STR("\r\n");
			return TASK_NONE;
		}

		if (t->status == TS_NONE) {
			*s = TS_MOTION;
			return TASK_MOTION;
		}
	}
	return TASK_NONE;
}
static BYTE check_battery(BYTE *s) {
	return TASK_NONE;
}

/////////////////////////////////////////////////////////////////////////////////////
// functions
static char func_led(void *arg) {
	stTask_t *t = (stTask_t*)arg;
	char ret = 0;
	switch (t->status) {
		case TS_LED_ON:
			MY_DEBUG_SEND_STR("led on\r\n");
			LED_ON(2);
			t->status = TS_IDLE;
		break;
		case TS_LED_OFF:
			MY_DEBUG_SEND_STR("led off\r\n");
			LED_OFF(2);
			t->status = TS_IDLE;
		break;
		case TS_LED_TOGGLE:
			MY_DEBUG_SEND_STR("led toggle\r\n");
			LED_TOGGLE(2);
			t->status = TS_IDLE;
		break;
		case TS_LED_BLINK:
			MY_DEBUG_SEND_STR("led blink\r\n");
			LED_TOGGLE(2);
		break;
		case TS_IDLE:
		break;
		
		default:
		break;
	}
	return ret;
}
static BYTE brrk_led(void *arg) {
	//stTask_t *t = (stTask_t*)arg;
	return 0;
}
static BYTE time_led(void *arg) {
	stTask_t *t = (stTask_t*)arg;
	if (t->status == TS_LED_BLINK) {
		return  t->param;
	}
	return 0xff;
}



static char func_learn(void *arg) {
	stTask_t *t = (stTask_t*)arg;
	char ret = 0;

	switch (t->status) {
		case TS_LEARN_START:
			misc_zw_learn();
			t->status = TS_LEARN_STARING;
		break;
		case TS_LEARN_STARING:
			/* wait learn complete or stop */
		break;
		case TS_LEARN_START_DONE:
			misc_zw_learn_done(0);
			t->status = TS_IDLE;
		break;
		case TS_IDLE:
		break;

		default:
		break;
	}
	return ret;

}
static BYTE brrk_learn(void *arg) {
	stTask_t *t = (stTask_t*)arg;
	if (t->status == TS_LEARN_STARING) {
		return !0;
	}
	return 0;
}
static BYTE time_learn(void *arg) {
	stTask_t *t = (stTask_t*)arg;
	if (t->status == TS_IDLE ||
			t->status == TS_NONE) {
		return 0xff;
	}
	return 0;
}


static char func_reset(void *arg) {
	stTask_t *t = (stTask_t*)arg;
	char ret = 0;

	switch (t->status) {
		case TS_RESET:
			misc_zw_reset();
			t->status = TS_RESETING;
		break;

		case TS_RESETING:
			/* wait reset ok or failed */
		break;

		case TS_RESET_DONE:
			misc_zw_reset_done(0);
			t->status = TS_IDLE;
		break;

		case TS_IDLE:
		break;

		default:
		break;
	}
	return ret;

}
static BYTE brrk_reset(void *arg) {
	stTask_t *t = (stTask_t*)arg;
	if (t->status == TS_RESETING) {
		return !0;
	}
	return 0;
}
static BYTE time_reset(void *arg) {
	stTask_t *t = (stTask_t*)arg;
	if (t->status == TS_IDLE ||
			t->status == TS_NONE) {
		return 0xff;
	}
	return 0;
}


static char func_motion(void *arg) {
	stTask_t *t = (stTask_t*)arg;
	char ret = 0;

	switch (t->status) {
		case TS_MOTION:
			misc_zw_send_motion();
			t->status = TS_MOTIONING;
		break;
		case TS_MOTIONING:
			/* wait motioning ack */
			//t->status = TS_MOTION_DONE;
		break;
		case TS_MOTION_DONE:
			misc_zw_send_motion_done(0);
			t->status = TS_IDLE;
		break;

		case TS_IDLE:
		break;

		default:
		break;
	}
	return ret;

}
static BYTE brrk_motion(void *arg) {
	stTask_t *t = (stTask_t*)arg;
	if (t->status == TS_MOTIONING) {
		return !0;
	}
	return 0;
}
static BYTE time_motion(void *arg) {
	stTask_t *t = (stTask_t*)arg;
	if (t->status == TS_IDLE ||
			t->status == TS_NONE) {
		return 0xff;
	}
	return 0;
}


static char func_battery(void *arg) {
	stTask_t *t = (stTask_t*)arg;

	char ret = 0;
	switch (t->status) {
		case TS_BATTERY:
			misc_zw_send_battery();
			t->status = TS_BATTERING;
		break;
		case TS_BATTERING:
			/* wait battering ack */
		break;

		case TS_BATTERY_DONE:
			misc_zw_send_battery_done(0);
			t->status = TS_IDLE;
		break;

		case TS_IDLE:
		break;

		default:
		break;
	}
	return ret;
}
static BYTE brrk_battery(void *arg) {
	stTask_t *t = (stTask_t*)arg;
	if (t->status == TS_BATTERING) {
		return !0;
	}
	return 0;
}
static BYTE time_battery(void *arg) {
	stTask_t *t = (stTask_t*)arg;
	if (t->status == TS_IDLE ||
			t->status == TS_NONE) {
		return 0xff;
	}
	return 0;
}


/////////////////////////////////////////////////////////////////////////////////////
// Unused Callback
void ApplicationRfNotify(BYTE rfState) {
 UNUSED(rfState);
}
void ApplicationTestPoll(void) {
}
void ApplicationSlaveUpdate(BYTE bStatus, BYTE bNodeID, BYTE* pCmd, BYTE bLen) {
	//MY_DEBUG_SEND_STR("ApplicationSlaveUpdate\r\n");
}
BYTE AppPowerDownReady() {
	//MY_DEBUG_SEND_STR("AppPowerDownReady\r\n");
	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////////////
//Serial Poll
static void SerialPollReset() {
  sts = S_WAIT_HEAD;
  len = rlen = flen = sum = 0;
}
static BYTE SerialPoll() {	
		BYTE cnt = MAX_FRAME_LEN;
		while (ZW_SerialCheck() && cnt > 0) {
				BYTE b =  ZW_SerialGetByte();
				cnt--;
        switch (sts) {
        case S_WAIT_HEAD:
          if (b == 0xfe) {
            sts = S_WAIT_CMD1;
            frame[flen++] = b;
          }
          return 0;

        case S_WAIT_CMD1:
          frame[flen++] = b;
          sum ^= b;
          sts = S_WAIT_CMD2;
          break;
          
        case S_WAIT_CMD2:
          frame[flen++] = b;
          sum ^= b;
          sts = S_WAIT_LEN;
          break;
          
        case S_WAIT_LEN:
          frame[flen++] = b;
          sum ^= b;
          len = b;
          
          if (len > MAX_FRAME_LEN) {
            SerialPollReset();
          } else {
            if (len > 0) {
              sts = S_WAIT_DATA;
            } else {
              sts = S_WAIT_CHECK;
            }
          }
          break;
        case S_WAIT_DATA:
          frame[flen++] = b;
          sum ^= b;
          rlen++;
          if (rlen == len) {
            sts = S_WAIT_CHECK;
          }
          break;
        case S_WAIT_CHECK:
          frame[flen++] = b;
          if (sum == b) {
						SerialPollReset();
						SerialSendFrame(frame[1] | 0x80, 0x55, 0, 0);
						return 1;
          }
          SerialPollReset();
          break;
        default:
          SerialPollReset();
          break;
        }
		}
		return 0;
}
	
static void SerialSendFrame(BYTE cmd1, BYTE cmd2, BYTE *da, BYTE len) {
	BYTE sum = 0;
  BYTE x = 0;
  BYTE i = 0;
  
  x = 0xFE;
	ZW_SerialPutByte(x);
	ZW_SerialFlush();
  
  x = cmd1;
	ZW_SerialPutByte(x);
	ZW_SerialFlush();
  sum ^= x;
  
  x = cmd2;
	ZW_SerialPutByte(x);
	ZW_SerialFlush();
  sum ^= x;
	
	x = len;
	ZW_SerialPutByte(x);
	ZW_SerialFlush();
  sum ^= x;
  
  for (i = 0; i < len; i++) {
    x = da[i];
		ZW_SerialPutByte(x);
		ZW_SerialFlush();
    sum ^= x;
  }
  
  x = sum;
	ZW_SerialPutByte(x);
	ZW_SerialFlush();
  sum ^= x;
}

static void SerialSendStr(BYTE *str) {

	while (*str != 0) {
		ZW_SerialPutByte(*str);
		ZW_SerialFlush();  
		str++;
	}

}
static void SerialSendHex(BYTE h) {
	if (h >= 0 && h <= 9) {
		ZW_SerialPutByte(h + '0');
		ZW_SerialFlush();  
	} else {
		ZW_SerialPutByte(h - 10 + 'A');
		ZW_SerialFlush();  
	}
}
static void SerialSendNum(BYTE num) {
	BYTE b1 = (num>>4)&0x0f;
	BYTE b2 = (num>>0)&0x0f;
	SerialSendHex(b1);
	SerialSendHex(b2);
}
static void SerialSendNl() {
	ZW_SerialPutByte('\r');
	ZW_SerialFlush();  
	ZW_SerialPutByte('\n');
	ZW_SerialFlush();  
}

