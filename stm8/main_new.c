#include "stm8l10x.h"
#include "stm8l10x_gpio.h"

//////////////////////////////////////////////////////////////
// Pin Configs
#define KEY_GPIO_PORT GPIOB
#define KEY_GPIO_PIN	GPIO_Pin_1
#define KEY_EXTI_PIN	EXTI_Pin_1
#define KEY_EXTI_STS	EXTI_IT_Pin1

#define PIR_GPIO_PORT GPIOB
#define PIR_GPIO_PIN	GPIO_Pin_0
#define PIR_EXTI_PIN	EXTI_Pin_0
#define PIR_EXTI_STS	EXTI_IT_Pin0

#define WAK_GPIO_PORT GPIOB
#define WAK_GPIO_PIN	GPIO_Pin_7

#define	MSG_GPIO_PORT GPIOB
#define MSG_GPIO_PIN1	GPIO_Pin_6
#define MSG_GPIO_PIN2	GPIO_Pin_5


#define LED_GPIO_PORT GPIOA
#define LED_GPIO_PIN	GPIO_Pin_2


//////////////////////////////////////////////////////////////
// Macro
#if 0
#define AWU_TIMEOUT_SEC								((30)/15)
#define RPT_TIMEOUT										((15*60)/15)
#define HAS_PERSON_TO_NO_PERSON_TIME	((2*60)/15)
#elif 0
#define AWU_TIMEOUT_SEC								(60)
#define RPT_TIMEOUT										(15*60)
#define HAS_PERSON_TO_NO_PERSON_TIME	(2*60)
#else 
#define AWU_TIMEOUT_SEC								((30))
#define RPT_TIMEOUT										((15*60))
#define HAS_PERSON_TO_NO_PERSON_TIME	((2*60))
#endif


#define BUAD	        115200

#define MAX_SLEEP_TIME 60

#define RELOAD_VALUE 255
//////////////////////////////////////////////////////////////
// Enum
enum {
	E_KEY								= 0x01,
	E_PIR								= 0x02,
	E_AWU_TIMEOUT				= 0x04,
	E_UART_DATA					= 0x08,
	E_TIM4_TIMEOUT			= 0x10,
};

enum {
	MSG_WAKEUP					= 0x01, 
	MSG_QUERY_INCLUDE		= 0x41,
	MSG_INCLUDE					= 0x42,
	MSG_EXCLUDE					= 0x43,
	MSG_POST_PIR				= 0x44,
};

enum {
	TASK_QUERY_INCLUDE	= 0,
	TASK_INCLUDE				= 1,
	TASK_EXCLUDE				= 2,
	TASK_POST_PIR				= 3,
	TASK_MAIN_LOGIC			= 4,
};
enum {
	S_IDLE												= 0x0,
	S_WAKEUP											= 0x1,
	S_WAKEUPING										= 0x2,
	S_WAKEUP_FAILED								= 0x3,

	S_QUERY_INCLUDE_QUERY					= 0x10,
	S_QUERY_INCLUDE_QUERING				= 0x11,
	S_QUERY_INCLUDE_QUERY_FAILED	= 0x12,
	S_QUERY_INCLUDE_QUERY_SUCCESS = 0x13,

	S_INCLUDE_INCLUDE							= 0x21,
	S_INCLUDE_INCLUDING						= 0x22,
	S_INCLUDE_INCLUDE_FAILED			= 0x23,
	S_INCLUDE_INCLUDE_SUCCESS			= 0x24,

	S_EXCLUDE_EXCLUDE							= 0x30,
	S_EXCLUDE_EXCLUDING						= 0x31,
	S_EXCLUDE_EXCLUDE_FAILED			= 0x32,
	S_EXCLUDE_EXCLUDE_SUCCESS			= 0x33,

	S_POST_PIR_POST								= 0x40,
	S_POST_PIR_POSTING						= 0x41,
	S_POST_PIR_POST_FAILED				= 0x42,
	S_POST_PIR_POST_SUCCESS				= 0x43,


	S_MAIN_QUERY									= 0x90,
	S_MAIN_QUERYING								= 0x91,
	S_MAIN_QUERY_FAILED						= 0x92,
	S_MAIN_INCLUDED								= 0x93,
	S_MAIN_NOT_INCLUDED						= 0x94,
	S_MAIN_INCLUDING							= 0x95,
	S_MAIN_EXCLUDING							= 0x96,
	S_MAIN_POST_PIRING						= 0x97,
};


//////////////////////////////////////////////////////////////
//Task 
typedef int (*TASKFUNC)(void *arg);
typedef int (*TASKTIME)(void *arg);
typedef struct stTask {
	u8				id;
	TASKFUNC	func;
	TASKTIME	stime;
	u8				state;
	void			*arg;
}stTask_t;

//////////////////////////////////////////////////////////////
//funcs
static void udelay_0p68(__IO uint16_t nCount);
static void mdelay(__IO uint16_t ms);

static void  clock_init(CLK_MasterPrescaler_TypeDef div);

static void sleep_init();
static void AWU_Init_60s(void);
static void sleep_60s();
static void sleep(AWU_Timebase_TypeDef ms);

static void ext_init();
static void enable_int();
static void disable_int();

static void key_init();
static void key_enable();
static void key_disable();
static u8 key_get();
static u8 key_disabled();

static void pir_enable();
static void pir_disalbe();
static u8 pir_disabled();
static u8 pir_get();

static void pir_init();

static void uart_init(uint32_t buad);
static void uart_enable();
static void uart_disable();
static void uart_send(u8 *data, u8 len);
static void uart_sendstr(u8 *str);
static void uart_hexPrintf(u8 data);
static void uart_clear();
static u8 uart_get(u8 *byte);
static inline void uart_put(u8 byte);

static void io_init();

static void led_init();
static void led_toggle();
static void led_off();
static void led_on();
static void led_loop();

static void timer4_init();
static void timer4_start(u8 tt, void (*func)(void));
static void timer4_stop();

static void msg_init();
static void msg_frame_send(u8 cmd1, u8 cmd2, u8 *msg, u8 len);
static u8 msg_frame_get(u8 *f);
static void msg_wakeup();
static void msg_query_include();
static void msg_include();
static void msg_exclude();
static void msg_post(u8 pir);
static u8 msg_frame_type(u8 *fm);
static u8 *msg_frame_data(u8 *fm);

static void task_loop();
static u8		task_sleep_time();

static void watch_dog_init();
static void watch_dog_reload();
//////////////////////////////////////////////////////////////
// Global Variable 
static u8 event							= 0;
static u8 pir								= 0;

int func_query_include(void *arg);
int stime_query_include(void *arg);

int func_include(void *arg);
int stime_include(void *arg);

int func_exclude(void *arg);
int stime_exclude(void *arg);

int func_post_pir(void *arg);
int stime_post_pir(void *arg);

int func_main_logic(void *arg);
int stime_main_logic(void *arg);

static stTask_t	tasks[] = {
	{0, func_query_include, stime_query_include,	S_IDLE, 0},
	{1, func_include,				stime_include,				S_IDLE, 0},
	{2, func_exclude,				stime_exclude,				S_IDLE, 0},
	{3, func_post_pir,			stime_post_pir,				S_IDLE, 0},
	{4, func_main_logic,		stime_main_logic,			S_MAIN_QUERY, 0},
};
static u8 fm[32];
static u8 included					= 0xff;

static u8 version						= 0x01;			/* app version */

static int timer4tlt					= 0;
static int timer4cnt					= 0;

static int timecnt						= 0;
static int timecnt_has2no			= 0;
//////////////////////////////////////////////////////////////
// main 
int main() {
	u8 tt = 0;

	clock_init(CLK_MasterPrescaler_HSIDiv2);	/* HSI 8M Hz */
	//ext_init();
	disable_int();

	io_init();

	led_init();
	key_init();
	pir_init();

	uart_init(BUAD);
	msg_init();

	sleep_init();
	timer4_init();
	//watch_dog_init();

	enable_int();
	while (1) {
		//watch_dog_reload();
		task_loop();
		tt = task_sleep_time();
		if (tt == 0) {
			/* not sleep */
		} else {
			sleep(AWU_Timebase_2s);
			//sleep_60s();

			if (tasks[TASK_MAIN_LOGIC].state == S_MAIN_QUERY_FAILED) {
				tasks[TASK_MAIN_LOGIC].state = S_MAIN_QUERY;
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////
// functions
/* delay */
static void udelay_0p68(uint16_t nCount) {
	while (nCount != 0) {
		nCount--;
	}
}
static void mdelay(uint16_t ms) {
	uint16_t i = 0;
	uint16_t nCount = 0;
	for (i = 0; i < ms; i++) {
		nCount = 1450;
		while (nCount != 0) {
			nCount--;
		}
	}
}

/* clock */
static void  clock_init(CLK_MasterPrescaler_TypeDef div) {
	CLK_DeInit();
	CLK_MasterPrescalerConfig(div);
}

/* sleep&timer */
static uint32_t   fmaster =0;
static void sleep_init() {
	CLK_PeripheralClockConfig(CLK_Peripheral_AWU,ENABLE);
	CLK_PeripheralClockConfig(CLK_Peripheral_TIM2,ENABLE);

	//使能AWU外设时钟
	AWU_DeInit();
	TIM2_DeInit();

	//连接LSI和TIM2的输入捕获 
	AWU->CSR |= AWU_CSR_MSR; 
	//调用库函数获得LSI频率
	fmaster=TIM2_ComputeLsiClockFreq(2000000U);
	//断开LSI和TIM2输入捕获的连接
	AWU->CSR &= (uint8_t)(~AWU_CSR_MSR);

	//AWU LSI校准，因为AWU是被LSI驱动工作的，标准应该是TIM2输入捕获来测量，精度不高可省略
	AWU_LSICalibrationConfig(fmaster);
}
static void sleep(AWU_Timebase_TypeDef ms) {
	AWU_Init(ms);
	AWU_Cmd(ENABLE);
	halt();
}
static void AWU_Init_60s(void) {
  /* Enable the AWU peripheral */
  AWU->CSR |= AWU_CSR_AWUEN;
  /* Set the TimeBase */
  AWU->TBR &= (uint8_t)(~AWU_TBR_AWUTB);
  AWU->TBR |= 0x0f;
  /* Set the APR divider */
  AWU->APR &= (uint8_t)(~AWU_APR_APR);
  AWU->APR |= 32;
}
static void sleep_60s() {
	AWU_Init_60s();
	AWU_Cmd(ENABLE);
	halt();
}

/* ext int */
static void ext_init() {
	EXTI_DeInit();
}

static void enable_int() {
	enableInterrupts();
}

static void disable_int() {
	disableInterrupts();
}

/* key */
u8 GPIO_Dir_Get(GPIO_TypeDef* GPIOx, uint8_t GPIO_Pin) {
	return (GPIOx->DDR & (1 << GPIO_Pin));
}
 
static void key_init() {
	GPIO_Init(KEY_GPIO_PORT, KEY_GPIO_PIN, GPIO_Mode_In_PU_IT);
	EXTI_SetPinSensitivity (KEY_EXTI_PIN,EXTI_Trigger_Falling);
}
static void key_enable() {
	GPIO_Init(KEY_GPIO_PORT, KEY_GPIO_PIN, GPIO_Mode_In_PU_IT);
	EXTI_SetPinSensitivity (KEY_EXTI_PIN,EXTI_Trigger_Falling);
}
static void key_disable() {
	GPIO_Init(KEY_GPIO_PORT, KEY_GPIO_PIN, GPIO_Mode_Out_PP_Low_Slow);
}
static u8 key_get() {
	return !GPIO_ReadInputDataBit(KEY_GPIO_PORT, KEY_GPIO_PIN);
}
static u8 key_disabled() {
	return GPIO_Dir_Get(KEY_GPIO_PORT, KEY_GPIO_PIN);
}
/* pir */
static void pir_init() {
	GPIO_Init(PIR_GPIO_PORT, PIR_GPIO_PIN, GPIO_Mode_In_PU_IT);
	EXTI_SetPinSensitivity (PIR_EXTI_PIN,EXTI_Trigger_Falling);
}
static void pir_enable() {
	GPIO_Init(PIR_GPIO_PORT, PIR_GPIO_PIN, GPIO_Mode_In_PU_IT);
	EXTI_SetPinSensitivity (PIR_EXTI_PIN,EXTI_Trigger_Falling);
}
static void pir_disalbe() {
	GPIO_Init(PIR_GPIO_PORT, PIR_GPIO_PIN, GPIO_Mode_Out_PP_Low_Slow);
}
static u8 pir_get() {
	return !GPIO_ReadInputDataBit(PIR_GPIO_PORT, PIR_GPIO_PIN);
}
static u8 pir_disabled() {
	return GPIO_Dir_Get(PIR_GPIO_PORT, PIR_GPIO_PIN);
}


/* uart */
static u8 uart_buff[256]		= {0};
static u8 uart_head					= 0;
static u8 uart_tail					= 0;
static void uart_init(uint32_t buad) {
	CLK_PeripheralClockConfig (CLK_Peripheral_USART,ENABLE); //enable ext clock
  GPIO_Init(GPIOC,GPIO_Pin_3,GPIO_Mode_Out_PP_High_Fast);
  GPIO_Init(GPIOC,GPIO_Pin_2,GPIO_Mode_In_PU_No_IT);
	USART_Init(buad,USART_WordLength_8D,USART_StopBits_1,USART_Parity_No,USART_Mode_Tx|USART_Mode_Rx);
	USART_ITConfig (USART_IT_RXNE,ENABLE);
	USART_Cmd(ENABLE);
}
static void uart_enable() {
	USART_Cmd (ENABLE);
}
static void uart_disable() {
	USART_Cmd (DISABLE);
}
static void uart_send(u8 *data, u8 len) {
	u8 i = 0; 
	for (i = 0; i < len; i++) {
		USART_SendData8(data[i]);
		while(!USART_GetFlagStatus(USART_FLAG_TXE));
	}
	udelay_0p68(20);
}
static void uart_sendstr(u8 *str) {
	while(*str!=0){
		USART_SendData8(*str);

		while(!USART_GetFlagStatus(USART_FLAG_TXE));

		str++;
	}
	udelay_0p68(20);
}
static void uart_hexPrintf(u8 data) {
	u8 hex[2];
	u8 sendStr[3];
	u8 i;

	hex[0] = (data >> 4) & 0x0f;
	hex[1] = data & 0x0f;
	for (i = 0;i < 2;i++) {
		if (hex[i] < 10) {
			sendStr[i] = '0' + hex[i]; 
		} else {
			sendStr[i] = 'A' + hex[i] - 10;
		}
	}
	sendStr[2] = 0;
	uart_sendstr(sendStr);
}
static void uart_clear() {
	uart_head = uart_tail = 0;
}
static u8 uart_get(u8 *byte) {
	if (uart_head == uart_tail) {
		return 0;
	}
	*byte = uart_buff[uart_tail];
	uart_tail++;
	return 1;
}
static inline void uart_put(u8 byte) {
	if (uart_head + 1 == uart_tail) {
		uart_tail++;		/* pop the first data */
	}
	uart_buff[uart_head++] = byte;
}

/* io init */
static void io_init() {
	GPIO_Init(GPIOA, 
			GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_6|GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_6,
			GPIO_Mode_Out_PP_Low_Slow
	);
	GPIO_Init(GPIOB, 
			GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_6|GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7, 
			GPIO_Mode_Out_PP_Low_Slow);
	GPIO_Init(GPIOC, 
			GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_6|GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_6, 
			GPIO_Mode_Out_PP_Low_Slow);
	GPIO_Init(GPIOD, 
			GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_6|GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7, 
			GPIO_Mode_Out_PP_Low_Slow);
}

/* led */
static void led_init() {
	GPIO_Init(LED_GPIO_PORT, LED_GPIO_PIN, GPIO_Mode_Out_PP_High_Slow);
}
static void led_toggle() {
	GPIO_ToggleBits(LED_GPIO_PORT, LED_GPIO_PIN);//翻转GPD0输出状态
}
static void led_off() {
  GPIO_SetBits(LED_GPIO_PORT, LED_GPIO_PIN);

} 
static void led_on() {
	GPIO_ResetBits(LED_GPIO_PORT, LED_GPIO_PIN);
}

/* timer4 */
static void (*cb_func)(void) = 0 ;
static void timer4_init() {
	CLK_PeripheralClockConfig (CLK_Peripheral_TIM4,ENABLE);
	TIM4_DeInit();
	TIM4_TimeBaseInit(TIM4_Prescaler_128, 0xff);//16M/8/128=15.625K，0xff=255,255*（1/15.625）=0.01632S，大约61次中断是1S
	TIM4_ITConfig(TIM4_IT_Update, ENABLE);//向上溢出中断使能，中断向量号25
}
static void timer4_start(u8 tt, void (*func)(void)) {
	cb_func = func;
	timer4tlt = tt * 61;
	timer4cnt = 0;
	event &= ~E_TIM4_TIMEOUT;
	TIM4_Cmd(ENABLE);
}
static void timer4_stop() {
	cb_func = 0;
	TIM4_Cmd(DISABLE);
}


/* post msg */
static void msg_init() {
	GPIO_Init(WAK_GPIO_PORT, WAK_GPIO_PIN,	GPIO_Mode_Out_PP_High_Slow);
}
static void msg_frame_send(u8 cmd1, u8 cmd2, u8 *msg, u8 len) {
  u8 sum = 0;
  u8 x = 0;
  u8 i = 0;
  
  x = 0xFE;
  uart_send(&x, 1);
  
  x = cmd1;
  uart_send(&x, 1);
  sum ^= x;
  
  x = cmd2;
  uart_send(&x, 1);
  sum ^= x;
  
  x = len;
  uart_send(&x, 1);
  sum ^= x;
  
  for (i = 0; i < len; i++) {
    x = msg[i];
    uart_send(&x, 1);
    sum ^= x;
  }
  
   x = sum;
   uart_send(&x, 1);
   sum ^= x;
}

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
static u8 frame[MAX_FRAME_LEN - 5];
static u8 sts = 0;
static u8 flen = 0;
static u8 len = 0;
static u8 rlen = 0;
static u8 sum = 0;
static void state_reset() {
  sts = S_WAIT_HEAD;
  len = rlen = flen = sum = 0;
}
static u8 msg_frame_get(u8 *_fm) {
	u8 b;
	while (uart_get(&b)) {
		switch (sts) {
			case S_WAIT_HEAD:
				if (b == 0xfe) {
					sts = S_WAIT_CMD1;
					frame[flen++] = b;
				}
				break;

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
					state_reset();
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
					memcpy(_fm, frame, flen);
					state_reset();
					return 1;
				}
				state_reset();
				break;
			default:
				state_reset();
				break;
		}
	}
	return 0;
}

static void msg_wakeup() {
	GPIO_ResetBits(WAK_GPIO_PORT, WAK_GPIO_PIN);
	mdelay(1);
	GPIO_SetBits(WAK_GPIO_PORT, WAK_GPIO_PIN);
}
static void msg_query_include() {
	msg_frame_send(MSG_QUERY_INCLUDE, 0x55, &version, 1);
}
static void msg_include() {
	msg_frame_send(MSG_INCLUDE, 0x55, 0, 0);
}
static void msg_exclude() {
	msg_frame_send(MSG_EXCLUDE, 0x55, 0, 0);
}
static void msg_post(u8 pir) {
	msg_frame_send(MSG_POST_PIR, 0x55, &pir, 1);
}

static u8 msg_frame_type(u8 *fm) {
	return fm[1]&0xff;
}

static u8 *msg_frame_data(u8 *fm) {
	return fm + 4;
}

static void watch_dog_init() {
	//使能IWDG
	IWDG_Enable();
	//解除写保护  
	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
	//LSI驱动IWDG，LSI 256分频=38000/256
	IWDG_SetPrescaler(IWDG_Prescaler_256);

	/* IWDG timeout = (RELOAD_VALUE + 1) * Prescaler / LSI 
		 = (255 + 1) * 256 / 38 000 
		 = 1723.63 ms */
	IWDG_SetReload((uint8_t)RELOAD_VALUE);

	/* Reload IWDG counter */
	IWDG_ReloadCounter();
}
static void watch_dog_reload() {
	IWDG_ReloadCounter(); 
}

///////////////////////////////////////////////////////////////////////////////////
//interrupt handler 
INTERRUPT_HANDLER(EXTI0_IRQHandler, 8) {   /* pir */
	//pir = EXTI_GetITStatus(PIR_EXTI_STS);
	event |= E_PIR;
	EXTI_ClearITPendingBit (PIR_EXTI_STS);
}  

INTERRUPT_HANDLER(EXTI1_IRQHandler, 9) {  /* key */
	//key_pressed = EXTI_GetITStatus(KEY_EXTI_STS);
	event |= E_KEY;
	EXTI_ClearITPendingBit (KEY_EXTI_STS);
}  

INTERRUPT_HANDLER(AWU_IRQHandler,4) {				/* awu */
	//AWU_GetFlagStatus();
	event |= E_AWU_TIMEOUT;
	AWU_IdleModeEnable();
}

INTERRUPT_HANDLER(TIM4_UPD_OVF_IRQHandler, 25) {
	/* In order to detect unexpected events during development,
		 it is recommended to set a breakpoint on the following instruction.
	 */
	timer4cnt++;
	if (timer4cnt >= timer4tlt) {
		event |= E_TIM4_TIMEOUT;
	}
	if (tasks[TASK_MAIN_LOGIC].state == S_MAIN_INCLUDING ||
			tasks[TASK_MAIN_LOGIC].state == S_MAIN_EXCLUDING) {
		if (timer4cnt % 30 == 0) {
			led_toggle();
		}
	}
	TIM4_ClearITPendingBit(TIM4_IT_Update);
}

INTERRUPT_HANDLER(USART_RX_IRQHandler, 28) {	/* tx */
	//event |= E_UART_DATA;
	uart_put(USART_ReceiveData8());
	USART_ClearITPendingBit ();
}


///////////////////////////////////////////////////////////////////////////////////
//task
void query_include_wakeup_timeout(void) {
	stTask_t *t = &tasks[TASK_QUERY_INCLUDE];
	timer4_stop();
	t->state = S_WAKEUP_FAILED;
}
void query_include_timeout(void) {
	stTask_t *t = &tasks[TASK_QUERY_INCLUDE];
	timer4_stop();
	t->state = S_QUERY_INCLUDE_QUERY_FAILED;
}
int func_query_include(void *arg) {
	stTask_t *t = &tasks[TASK_QUERY_INCLUDE];
	switch (t->state) {
		case S_WAKEUP:
			msg_wakeup();
			t->state = S_WAKEUPING;
			timer4_start(1, query_include_wakeup_timeout);
		break;
		case S_WAKEUPING:
			if (msg_frame_get(fm)) {
				if (msg_frame_type(fm) == MSG_WAKEUP | 0x80) {
					timer4_stop();
					t->state = S_QUERY_INCLUDE_QUERY;
				}
			}
			if (event & E_TIM4_TIMEOUT) {
				if (cb_func != 0) {
					cb_func();
				}
				event &= ~E_TIM4_TIMEOUT;
			}
		break;
		case S_WAKEUP_FAILED:
			t->state = S_IDLE;
			tasks[TASK_MAIN_LOGIC].state = S_MAIN_QUERY_FAILED;
		break;
		case S_QUERY_INCLUDE_QUERY:
			msg_query_include();
			t->state = S_QUERY_INCLUDE_QUERING;
			timer4_start(1, query_include_timeout);
		break;
		case S_QUERY_INCLUDE_QUERING:
			if (msg_frame_get(fm)) {
				if (msg_frame_type(fm) == MSG_QUERY_INCLUDE | 0x80) {
					timer4_stop();
					t->state = S_QUERY_INCLUDE_QUERY_SUCCESS;
					included = !!fm[4];
				}
			}
			if (event & E_TIM4_TIMEOUT) {
				if (cb_func != 0) {
						cb_func();
				}
				event &= ~E_TIM4_TIMEOUT;
			}
		break;
		case S_QUERY_INCLUDE_QUERY_FAILED:
			t->state = S_IDLE;
			tasks[TASK_MAIN_LOGIC].state  = S_MAIN_QUERY_FAILED;
		break;
		case S_QUERY_INCLUDE_QUERY_SUCCESS:
			t->state = S_IDLE;
			if (included) {
				tasks[TASK_MAIN_LOGIC].state  = S_MAIN_INCLUDED;
			event = 0;
			} else {
				tasks[TASK_MAIN_LOGIC].state  = S_MAIN_NOT_INCLUDED;
			event = 0;
			}
		break;
	}
	return 0;
}
int stime_query_include(void *arg) {
	stTask_t *t = &tasks[TASK_QUERY_INCLUDE];
	if (t->state == S_IDLE) {
		return MAX_SLEEP_TIME;
	}
	return 0;
}


void include_wakeup_timeout(void) {
	stTask_t *t = &tasks[TASK_INCLUDE];
	timer4_stop();
	t->state = S_WAKEUP_FAILED;
}

void include_include_timeout(void) {
	stTask_t *t = &tasks[TASK_INCLUDE];
	timer4_stop();
	t->state = S_INCLUDE_INCLUDE_FAILED;
}

int func_include(void *arg) {
	stTask_t *t = &tasks[TASK_INCLUDE];
	switch (t->state) {
		case S_WAKEUP:
			msg_wakeup();
			t->state = S_WAKEUPING;
			timer4_start(1, include_wakeup_timeout);
		break;
		case S_WAKEUPING:
			if (msg_frame_get(fm)) {
				if (msg_frame_type(fm) == MSG_WAKEUP | 0x80) {
					timer4_stop();
					t->state = S_INCLUDE_INCLUDE;
				}
			}
			if (event & E_TIM4_TIMEOUT) {
				if (cb_func != 0) {
						cb_func();
				}
				event &= ~E_TIM4_TIMEOUT;
			}
		break;
		case S_WAKEUP_FAILED:
			t->state = S_IDLE;
			tasks[TASK_MAIN_LOGIC].state = S_MAIN_NOT_INCLUDED;
			event = 0;
		break;
		case S_INCLUDE_INCLUDE:
			msg_include();
			led_on();
			timer4_start(40, include_include_timeout);
			t->state = S_INCLUDE_INCLUDING;
		break;
		case S_INCLUDE_INCLUDING:
			if (msg_frame_get(fm)) {
				if (msg_frame_type(fm) == MSG_INCLUDE | 0x80) {
					timer4_stop();
					included = !!msg_frame_data(fm)[0];
					if (included) {
						t->state = S_INCLUDE_INCLUDE_SUCCESS;
					} else {
						t->state = S_INCLUDE_INCLUDE_FAILED;
					}
				}
			}
			if (event & E_TIM4_TIMEOUT) {
				if (cb_func != 0) {
						cb_func();
				}
				event &= ~E_TIM4_TIMEOUT;
			}
		break;
		case S_INCLUDE_INCLUDE_FAILED:
			t->state = S_IDLE;
			tasks[TASK_MAIN_LOGIC].state = S_MAIN_NOT_INCLUDED;
			led_off();
			event = 0;
		break;
		case S_INCLUDE_INCLUDE_SUCCESS:
			t->state = S_IDLE;
			tasks[TASK_MAIN_LOGIC].state = S_MAIN_INCLUDED;
			led_off();
			event = 0;
		break;
	}
	return 0;
}
int stime_include(void *arg) {
	stTask_t *t = &tasks[TASK_INCLUDE];
	if (t->state == S_IDLE) {
		return MAX_SLEEP_TIME;
	}
	return 0;
}



void exclude_wakeup_timeout(void) {
	stTask_t *t = &tasks[TASK_EXCLUDE];
	timer4_stop();
	t->state = S_WAKEUP_FAILED;
}

void exclude_exclude_timeout(void) {
	stTask_t *t = &tasks[TASK_EXCLUDE];
	timer4_stop();
	t->state = S_EXCLUDE_EXCLUDE_FAILED;
}

int func_exclude(void *arg) {
	stTask_t *t = &tasks[TASK_EXCLUDE];
	switch (t->state) {
		case S_WAKEUP:
			msg_wakeup();
			t->state = S_WAKEUPING;
			timer4_start(1, exclude_wakeup_timeout);
		break;
		case S_WAKEUPING:
			if (msg_frame_get(fm)) {
				if (msg_frame_type(fm) == MSG_WAKEUP | 0x80) {
					timer4_stop();
					t->state = S_EXCLUDE_EXCLUDE;
				}
			}
			if (event & E_TIM4_TIMEOUT) {
				if (cb_func != 0) {
						cb_func();
				}
				event &= ~E_TIM4_TIMEOUT;
			}
		break;
		case S_WAKEUP_FAILED:
			t->state = S_IDLE;
			tasks[TASK_MAIN_LOGIC].state = S_MAIN_INCLUDED;
		break;
		case S_EXCLUDE_EXCLUDE:
			msg_exclude();
			led_on();
			t->state = S_EXCLUDE_EXCLUDING;
			timer4_start(10, exclude_exclude_timeout);
		break;
		case S_EXCLUDE_EXCLUDING:
			if (msg_frame_get(fm)) {
				if (msg_frame_type(fm) == MSG_EXCLUDE | 0x80) {
					timer4_stop();

					included = !!msg_frame_data(fm)[0];
					if (included) {
						t->state = S_EXCLUDE_EXCLUDE_FAILED;
					} else {
						t->state = S_EXCLUDE_EXCLUDE_SUCCESS;
					}
				}
			}
			if (event & E_TIM4_TIMEOUT) {
				if (cb_func != 0) {
						cb_func();
				}
				event &= ~E_TIM4_TIMEOUT;
			}
		break;
		case S_EXCLUDE_EXCLUDE_FAILED:
			t->state = S_IDLE;
			tasks[TASK_MAIN_LOGIC].state = S_MAIN_INCLUDED;
			led_off();
			event = 0;
		break;
		case S_EXCLUDE_EXCLUDE_SUCCESS:
			t->state = S_IDLE;
			tasks[TASK_MAIN_LOGIC].state = S_MAIN_NOT_INCLUDED;
			led_off();
			event = 0;
		break;
	}
	return 0;
}
int stime_exclude(void *arg) {
	stTask_t *t = &tasks[TASK_EXCLUDE];
	if (t->state == S_IDLE) {
		return MAX_SLEEP_TIME;
	}
	return 0;
}


void post_pir_wakeup_timeout(void) {
	stTask_t *t = &tasks[TASK_POST_PIR];
	timer4_stop();
	t->state = S_WAKEUP_FAILED;
}

void post_pir_post_timeout(void) {
	stTask_t *t = &tasks[TASK_POST_PIR];
	timer4_stop();
	t->state = S_POST_PIR_POST_FAILED;
}


int func_post_pir(void *arg) {
	stTask_t *t = &tasks[TASK_POST_PIR];
	switch (t->state) {
		case S_WAKEUP:
			msg_wakeup();
			t->state = S_WAKEUPING;
			timer4_start(1, post_pir_wakeup_timeout);
		break;
		case S_WAKEUPING:
			if (msg_frame_get(fm)) {
				if (msg_frame_type(fm) == MSG_WAKEUP | 0x80) {
					timer4_stop();
					t->state = S_POST_PIR_POST;
				}
			}
			if (event & E_TIM4_TIMEOUT) {
				if (cb_func != 0) {
						cb_func();
				}
				event &= ~E_TIM4_TIMEOUT;
			}
		break;
		case S_WAKEUP_FAILED:
			t->state = S_IDLE;
			event = 0;
			tasks[TASK_MAIN_LOGIC].state = S_MAIN_INCLUDED;
		break;
		case S_POST_PIR_POST:
			msg_post(pir);
			t->state = S_POST_PIR_POSTING;
			timer4_start(1, post_pir_post_timeout);
		break;
		case S_POST_PIR_POSTING:
			if (msg_frame_get(fm)) {
				if (msg_frame_type(fm) == MSG_POST_PIR | 0x80) {
					timer4_stop();
					t->state = S_POST_PIR_POST_SUCCESS;
				}
			}
			if (event & E_TIM4_TIMEOUT) {
				if (cb_func != 0) {
						cb_func();
				}
				event &= ~E_TIM4_TIMEOUT;
			}
		break;
		case S_POST_PIR_POST_FAILED:
			t->state = S_IDLE;
			event = 0;
			tasks[TASK_MAIN_LOGIC].state = S_MAIN_INCLUDED;
		break;
		case S_POST_PIR_POST_SUCCESS:
			t->state = S_IDLE;
			event = 0;
			tasks[TASK_MAIN_LOGIC].state = S_MAIN_INCLUDED;
		break;
	}
	return 0;
}
int stime_post_pir(void *arg) {
	stTask_t *t = &tasks[TASK_POST_PIR];
	if (t->state == S_IDLE) {
		return MAX_SLEEP_TIME;
	}
	return 0;
}

int func_main_logic(void *arg) {
	stTask_t *t = &tasks[TASK_MAIN_LOGIC];
	switch (t->state) {
		case S_MAIN_QUERY:
			//key_disable();
			//pir_disalbe();
			tasks[TASK_QUERY_INCLUDE].state = S_WAKEUP;
			t->state = S_MAIN_QUERYING;
		break;
		case S_MAIN_QUERYING:
			;
		break;
		case S_MAIN_QUERY_FAILED:
			//t->state = S_MAIN_QUERY;
		break;
		case S_MAIN_INCLUDED:
			if (key_disabled()) {
				//key_enable();
			}
			if (pir_disabled()) {
				//pir_enable();
			}
			if (event & E_KEY) {
				if (included) {
					tasks[TASK_EXCLUDE].state = S_WAKEUP;
					t->state = S_MAIN_EXCLUDING;
				} else {
					t->state = S_MAIN_NOT_INCLUDED;
				}
				event = 0;
			} else {
				if (event & E_PIR) {
					if (pir == 0) {
						pir = 1;
						tasks[TASK_POST_PIR].state = S_WAKEUP;
						t->state = S_MAIN_POST_PIRING;

						event &= ~E_AWU_TIMEOUT;
						timecnt = 0;
						timecnt_has2no = 0;
					} else {
						;
					}
					event &= ~E_PIR;
				} 

				if (event & E_AWU_TIMEOUT) {
					if (pir == 0) {
						timecnt++;
						if (timecnt * AWU_TIMEOUT_SEC >= RPT_TIMEOUT) {
							timecnt = 0;
							tasks[TASK_POST_PIR].state = S_WAKEUP;
							t->state = S_MAIN_POST_PIRING;
						} else {
							;
						}
					} else {
						timecnt++;
						timecnt_has2no++;
						if (timecnt_has2no * AWU_TIMEOUT_SEC >= HAS_PERSON_TO_NO_PERSON_TIME) {
							pir = 0;
							tasks[TASK_POST_PIR].state = S_WAKEUP;
							t->state = S_MAIN_POST_PIRING;
							timecnt = 0;
							timecnt_has2no = 0;
						}
						if (timecnt * AWU_TIMEOUT_SEC >= RPT_TIMEOUT) {
							tasks[TASK_POST_PIR].state = S_WAKEUP;
							t->state = S_MAIN_POST_PIRING;
							timecnt = 0;
							timecnt_has2no = 0;
						}
					}
					event &= ~E_AWU_TIMEOUT;
				}
			}
		break;
		case S_MAIN_NOT_INCLUDED:
			if (!pir_disabled()) {
				//pir_disalbe();
			}
			if (key_disabled()) {
				//key_enable();
			}
			if (event & E_KEY) {
				if (included) {
					t->state = S_MAIN_INCLUDED;
				} else {
					tasks[TASK_INCLUDE].state = S_WAKEUP;
					t->state = S_MAIN_INCLUDING;
				}
			}
		break;
		case S_MAIN_EXCLUDING:
			;
		break;
		case S_MAIN_POST_PIRING:
			;
		break;
		case S_MAIN_INCLUDING:
			;
		break;
	}
	return 0;
}


int stime_main_logic(void *arg) {
	stTask_t *t = &tasks[TASK_MAIN_LOGIC];
	switch (t->state) {
		case S_MAIN_QUERY:
			return 0;
		case S_MAIN_QUERYING:
			return 0;
		case S_MAIN_QUERY_FAILED:
			return MAX_SLEEP_TIME;
		case S_MAIN_INCLUDED:
			return MAX_SLEEP_TIME;
		case S_MAIN_EXCLUDING:
		break;
		case S_MAIN_POST_PIRING:
			return 0;
		break;
		case S_MAIN_NOT_INCLUDED:
			return MAX_SLEEP_TIME;
		break;
		case S_MAIN_INCLUDING:
			return 0;
		break;
	}
}

static void task_loop() {
	u8 i = 0;
	for (i = 0; i < sizeof(tasks)/sizeof(tasks[0]); i++) {
		stTask_t *t = &tasks[i];
		if (t->state != S_IDLE) {
			t->func(t);
		}
	}
}
static u8		task_sleep_time() {
	u8 i = 0;
	u8 tt = 60;
	for (i = 0; i < sizeof(tasks)/sizeof(tasks[0]); i++) {
		stTask_t *t = &tasks[i];
		if (t->stime(t) < tt) {
			tt = t->stime(t);
		}
	}
	return tt;
}



