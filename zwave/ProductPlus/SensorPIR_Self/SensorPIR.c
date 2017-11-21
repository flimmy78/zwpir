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
#include <ZW_nvr_api.h>
#include <appl_timer_api.h>

/////////////////////////////////////////////////////////////////////////////////////
// Debug Macro
//,ZW_DEBUG_SENSORPIR,ZW_DEBUG,ZM5202,ZW_DEBUG_BATT


/////////////////////////////////////////////////////////////////////////////////////
// Class Command
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
static myRfTimeout = 0;

#define SF_VERSION "1.0.0"

/////////////////////////////////////////////////////////////////////////////////////
// Initilize 
BYTE  ApplicationInitHW(BYTE bWakeupReason) {
	wakeupReason = bWakeupReason;

	LedControlInit();
	LedOn(2);
	

	Transport_OnApplicationInitHW(bWakeupReason);
  return(TRUE);
}

BYTE ApplicationInitSW( void ) {
#ifndef ZW_ISD51_DEBUG
  ZW_DEBUG_INIT(1152);
#endif
	
	ZW_DEBUG_SEND_STR("Wakeup:");
	ZW_DEBUG_SEND_STR(wakeup_reason_str[wakeupReason]);
	ZW_DEBUG_SEND_STR("\r\n");
	ZW_DEBUG_SEND_STR("SoftWare Version "SF_VERSION"\r\n");

	ApplTimerInit();

	conf_load();

	misc_zw_init();

	mySleepTimer =  ZW_TIMER_START(mySleepTimerFunc, 1 ,TIMER_FOREVER);  // 10 ms * 1

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
	//ZW_DEBUG_SEND_STR("ApplicationPoll\r\n");	

	
	task_in();
	task_do();
}
/////////////////////////////////////////////////////////////////////////////////////
// Learn More
void LearnCompleted(BYTE bNodeID) {
	stTask_t *t = NULL;
	ZW_DEBUG_SEND_STR("LearnCompleted\r\n");

  ZW_DEBUG_SEND_STR("Learn complete ");
  ZW_DEBUG_SEND_NUM(bNodeID);
  ZW_DEBUG_SEND_NL();

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
void Transport_ApplicationCommandHandler(BYTE  rxStatus, BYTE  sourceNode, 
																					ZW_APPLICATION_TX_BUFFER *pCmd, BYTE   cmdLength) {
	ZW_DEBUG_SEND_STR("Transport_ApplicationCommandHandler\r\n");	

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


BYTE GetMyNodeID(void) {
	ZW_DEBUG_SEND_STR("GetMyNodeID\r\n");
  return myEnv.NodeID;
}


/////////////////////////////////////////////////////////////////////////////////////
// Sleep 
PCB(mySleepTimerFunc)(void) {
	BYTE mode;
	BYTE mask;
	BYTE timeout;

	if (myRfTimeout != 0) {
		//ZW_DEBUG_SEND_STR("mySleepTimerFunc Failed, has rf Task.\r\n");
		return;
	}

	if (task_get_cnt() > 0 && task_must_wake()) {
		//has task to do, can do sleep
		//ZW_DEBUG_SEND_STR("mySleepTimerFunc Failed, Task not permit!\r\n");
		return;
	}
	if (!misc_node_included()) {
		if (task_has_rf_task()) {
			//mode = ZW_FREQUENTLY_LISTENING_MODE;
			//ZW_DEBUG_SEND_STR("mySleepTimerFunc Has Rf task(not include), Can't Sleep\r\n");
			return;	/* can't sleep */
		} else {
			mode = ZW_STOP_MODE;
			/* test for no button from remote */
			//mode = ZW_WUT_MODE;
		}
	} else {
		if (task_has_rf_task()) {
			//mode = ZW_FREQUENTLY_LISTENING_MODE;
			//ZW_DEBUG_SEND_STR("mySleepTimerFunc Has Rf task(include), Can't Sleep\r\n");
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
		//ZW_DEBUG_SEND_STR("mySleepTimerFunc Timeout 0, Not Sleep\r\n");
		return;
	}

	ZW_SetWutTimeout(timeout);
	if (!ZW_SetSleepMode(mode,mask,0)) {
		//ZW_DEBUG_SEND_STR("sleep failed\r\n");
		return;
	} 

	ZW_TIMER_CANCEL(mySleepTimerFunc);
	mySleepTimer = 0xff;
	LedOff(2);


	ZW_DEBUG_SEND_STR("Go to sleep with mode:");
	//ZW_DEBUG_SEND_NUM(mode);
	ZW_DEBUG_SEND_STR(sleep_mode_str[mode]);
	ZW_DEBUG_SEND_STR(",mask:");
	//ZW_DEBUG_SEND_NUM(mask);
	ZW_DEBUG_SEND_STR(sleep_mask_str[mask]);
	ZW_DEBUG_SEND_STR(",timeout:");
	ZW_DEBUG_SEND_NUM(timeout);
	ZW_DEBUG_SEND_STR("\r\n");
}


void misc_zw_setpowerdown_timeout() {
	ApplTimerStop(&myPowerTimer);
	myRfTimeout = 0;
}
PCB(mySetPowerDownTimeoutWakeUpStateCheck)(BYTE timeout) {
	ZW_DEBUG_SEND_STR("SetPowerDownTimeoutWakeupStateCheck:");
	ZW_DEBUG_SEND_NUM(timeout);
	ZW_DEBUG_SEND_STR("\r\n");

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

	ZW_DEBUG_SEND_STR("Revision:");
	ZW_DEBUG_SEND_NUM(nvs.bRevision);
	ZW_DEBUG_SEND_STR(",SAWBandWidth:");
	ZW_DEBUG_SEND_NUM(nvs.bSAWBandwidth);
	ZW_DEBUG_SEND_STR(",UUID:");
	for (i = sizeof(nvs.abUUID)-8; i < sizeof(nvs.abUUID); i++) {
		ZW_DEBUG_SEND_NUM(nvs.abUUID[i]);
	}
	ZW_DEBUG_SEND_STR("\r\n");



	//MemoryGetBuffer((WORD)&nvmApplDescriptor, &nvmappl, sizeof(nvmappl));
	//MemoryGetBuffer((WORD)&nvmDescriptor, &nvmdesc, sizeof(nvmdesc));

	misc_rf_failcnt_load();
	misc_nodeid_load();

	ZW_DEBUG_SEND_STR("nodeid:");
	ZW_DEBUG_SEND_NUM(myEnv.NodeID);
	ZW_DEBUG_SEND_STR(",RfFailCnt:");
	ZW_DEBUG_SEND_NUM(myEnv.RfFailCnt);
	ZW_DEBUG_SEND_STR("\r\n");
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
	conf_set((WORD)&nvmApplDescriptor.RfFailCnt, &myEnv.RfFailCnt, 1);
}
void misc_rf_failcnt_load() {
	conf_get((WORD)&nvmApplDescriptor.RfFailCnt, &myEnv.RfFailCnt, 1);
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

void misc_zw_send_motion() {
	ZW_DEBUG_SEND_STR("misc_zw_send_motion(\r\n");
	/* TODO */
}
void misc_zw_send_motion_done(char status) {
}

void misc_zw_send_battery() {
	ZW_DEBUG_SEND_STR("misc_zw_send_battery\r\n");
	/* TODO */
}
void misc_zw_send_battery_done(char status) {
	ZW_DEBUG_SEND_STR("misc_zw_send_battery_done\r\n");
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
	ZW_DEBUG_SEND_STR("misc_zw_learn\r\n");
  StartLearnModeNow(LEARN_MODE_INCLUSION);

	//myLearnTimer = ApplTimerStart(misc_zw_learn_timeout, 3, 1);
}
void misc_zw_learn_done(char status) {
	ZW_DEBUG_SEND_STR("misc_zw_learn_done(nodeid:");
	ZW_DEBUG_SEND_NUM(myEnv.NodeID);
	ZW_DEBUG_SEND_STR(")\r\n");
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
	ZW_DEBUG_SEND_STR("misc_zw_reset\r\n");
  StartLearnModeNow(LEARN_MODE_EXCLUSION);

	//myLearnTimer = ApplTimerStart(misc_zw_reset_timeout, 3, 1);
}
void misc_zw_reset_done(char status) {
	ZW_DEBUG_SEND_STR("misc_zw_reset_done(nodeid:");
	ZW_DEBUG_SEND_NUM(myEnv.NodeID);
	ZW_DEBUG_SEND_STR(")\r\n");
}

void misc_zw_init() {
	/*
	AssociationInit(FALSE);
  AGI_LifeLineGroupSetup(agiTableLifeLine, 
				(sizeof(agiTableLifeLine)/sizeof(CMD_CLASS_GRP)));
  AGI_ResourceGroupSetup(agiTableRootDeviceGroups, 
				(sizeof(agiTableRootDeviceGroups)/sizeof(AGI_GROUP)), 1);
  InitNotification();
  AddNotification(NOTIFICATION_REPORT_BURGLAR_V3,
				NOTIFICATION_EVENT_HOME_SECURITY_MOTION_DETECTION_UNKNOWN_LOCATION,
				NULL, 0);
	*/
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
			ZW_DEBUG_SEND_STR("task run failed with :");
			ZW_DEBUG_SEND_NUM(ret);
			ZW_DEBUG_SEND_STR("\r\n");
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
	static int x = 1;

	BYTE v24 = !!PIN_GET(P24);
	BYTE v36 = !!PIN_GET(P36);
	BYTE v   = (v24 << 1) | v36;

	//ZW_DEBUG_SEND_STR("Btn:");
	//ZW_DEBUG_SEND_NUM(v);
	//ZW_DEBUG_SEND_STR("\r\n");

	if (v == 0x01) {
		return !0;
	}

#if 1 /* test from remote no button to be pressed */
	if (x == 1) {
		x = 0;
		return 1;
	}
	return 0;
#else
	return 0;
#endif
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
			ZW_DEBUG_SEND_STR("led on\r\n");
			LED_ON(2);
			t->status = TS_IDLE;
		break;
		case TS_LED_OFF:
			ZW_DEBUG_SEND_STR("led off\r\n");
			LED_OFF(2);
			t->status = TS_IDLE;
		break;
		case TS_LED_TOGGLE:
			ZW_DEBUG_SEND_STR("led toggle\r\n");
			LED_TOGGLE(2);
			t->status = TS_IDLE;
		break;
		case TS_LED_BLINK:
			ZW_DEBUG_SEND_STR("led blink\r\n");
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
}
void ApplicationTestPoll(void) {
}
void ApplicationSlaveUpdate(BYTE bStatus, BYTE bNodeID, BYTE* pCmd, BYTE bLen) {
	//ZW_DEBUG_SEND_STR("ApplicationSlaveUpdate\r\n");
}
BYTE AppPowerDownReady() {
	//ZW_DEBUG_SEND_STR("AppPowerDownReady\r\n");
	return TRUE;
}

