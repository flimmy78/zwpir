#include "stm8l10x.h"
#include "stm8l10x_gpio.h"

/* Macro */

#define QUICK_TEST 1
#if QUICK_TEST
#define AWU_TIMEOUT_SEC								((30)/15)
#define RPT_TIMEOUT										((15*60)/15)
#define HAS_PERSON_TO_NO_PERSON_TIME	((2*60)/15)
#else
#define AWU_TIMEOUT_SEC								(30)
#define RPT_TIMEOUT										(15*60)
#define HAS_PERSON_TO_NO_PERSON_TIME	(2*60)
#endif

#define BUAD	115200

/* Enum */
enum  {
	S_NO_PERSON		= 0,
	S_HAS_PERSON	= 1,
};
enum {
	E_KEY					= 0x01,
	E_PIR					= 0x02,
	E_AWU_TIMEOUT	= 0x04,
};

/* Global Variable */
static u8 state					= S_NO_PERSON;
static u8 event					= 0;
static u8 timcnt				= 0;
static u8 timcnt_has2no	= 0;

#if 0
#define KEY_GPIO_PORT GPIOC
#define KEY_GPIO_PIN	GPIO_Pin_4
#elif 0
#define KEY_GPIO_PORT GPIOD
#define KEY_GPIO_PIN	GPIO_Pin_0
#else
#define KEY_GPIO_PORT GPIOB
#define KEY_GPIO_PIN	GPIO_Pin_1
#define KEY_EXTI_PIN	EXTI_Pin_1
#define KEY_EXTI_STS	EXTI_IT_Pin1
#endif

#define PIR_GPIO_PORT GPIOB
#define PIR_GPIO_PIN	GPIO_Pin_0
#define PIR_EXTI_PIN	EXTI_Pin_0
#define PIR_EXTI_STS	EXTI_IT_Pin0

#define WAK_GPIO_PORT GPIOB
#define WAK_GPIO_PIN	GPIO_Pin_7

#define	MSG_GPIO_PORT GPIOB
#define MSG_GPIO_PIN1	GPIO_Pin_6
#define MSG_GPIO_PIN2	GPIO_Pin_5

#if QUICK_TEST
#define TEST_GPIO_PORT GPIOB
#define TEST_GPIO_PIN GPIO_Pin_4
#endif


/* delay */
static void udelay_0p68(__IO uint16_t nCount) {
	while (nCount != 0) {
		nCount--;
	}
}
static void mdelay(__IO uint16_t ms) {
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

/* ext int */
static void enable_int() {
	enableInterrupts();
}
static void disable_int() {
	disableInterrupts();
}

static void ext_init() {
	EXTI_DeInit();
}

/* key */
static void key_init() {
	GPIO_Init(KEY_GPIO_PORT, KEY_GPIO_PIN, GPIO_Mode_In_PU_IT);
	EXTI_SetPinSensitivity (KEY_EXTI_PIN,EXTI_Trigger_Falling);
}
/* pir */
static void pir_init() {
	GPIO_Init(PIR_GPIO_PORT, PIR_GPIO_PIN, GPIO_Mode_In_PU_IT);
	EXTI_SetPinSensitivity (PIR_EXTI_PIN,EXTI_Trigger_Falling);
}
#if 0
static void pir_close() {
	/* TODO */
}
static u8 pir_get() {
	return !GPIO_ReadInputDataBit(PIR_GPIO_PORT, PIR_GPIO_PIN);
}
static u8 key_get() {
	return !GPIO_ReadInputDataBit(KEY_GPIO_PORT, KEY_GPIO_PIN);
}
#endif


/* uart */
static void uart_init(uint32_t buad) {
	CLK_PeripheralClockConfig (CLK_Peripheral_USART,ENABLE); //enable ext clock
  GPIO_Init(GPIOC,GPIO_Pin_3,GPIO_Mode_Out_PP_High_Fast);
  GPIO_Init(GPIOC,GPIO_Pin_2,GPIO_Mode_In_PU_No_IT);
	USART_Init(buad,USART_WordLength_8D,USART_StopBits_1,USART_Parity_No,USART_Mode_Tx|USART_Mode_Rx);
	USART_ITConfig (USART_IT_RXNE,ENABLE);
	USART_Cmd (ENABLE);
}

static void uart_sendstr(u8 *str) {
	while(*str!=0){
		USART_SendData8(*str);

		while(!USART_GetFlagStatus(USART_FLAG_TXE));

		str++;
	}
	udelay_0p68(20);
}

/* post msg */
static void msg_init() {
	GPIO_Init(MSG_GPIO_PORT, MSG_GPIO_PIN1, GPIO_Mode_Out_PP_High_Slow);
	GPIO_Init(MSG_GPIO_PORT, MSG_GPIO_PIN2, GPIO_Mode_Out_PP_High_Slow);
	GPIO_Init(WAK_GPIO_PORT, WAK_GPIO_PIN,	GPIO_Mode_Out_PP_High_Slow);
}
static void msg_post(u8 x, u8 y) {
	//PB6 = x;
	if (x) {
		GPIO_SetBits(MSG_GPIO_PORT, MSG_GPIO_PIN1);
	} else {
		GPIO_ResetBits(MSG_GPIO_PORT, MSG_GPIO_PIN1);
	}

	//PB5 = y;
	if (y) {
		GPIO_SetBits(MSG_GPIO_PORT, MSG_GPIO_PIN2);
	} else {
		GPIO_ResetBits(MSG_GPIO_PORT, MSG_GPIO_PIN2);
	}
	
	//PB7 = 0;
	GPIO_ResetBits(WAK_GPIO_PORT, WAK_GPIO_PIN);
	mdelay(1);
	//PB7 = 1;
	GPIO_SetBits(WAK_GPIO_PORT, WAK_GPIO_PIN);

	GPIO_SetBits(MSG_GPIO_PORT, MSG_GPIO_PIN1);
	GPIO_SetBits(MSG_GPIO_PORT, MSG_GPIO_PIN2);
}
static void msg_post_key() {
	uart_sendstr("Key Pressed\r\n");
	msg_post(1, 0);
}
static void msg_post_pir(u8 pir) {
	if (pir == 0) {
		uart_sendstr("NO PERSON\r\n");
		msg_post(0, 0);
	} else {
		uart_sendstr("HAS PERSON\r\n");
		msg_post(0, 1);
	}
}


/* test */
#if QUICK_TEST
static void test_init(void) {
	GPIO_Init(TEST_GPIO_PORT, TEST_GPIO_PIN, GPIO_Mode_Out_PP_High_Slow);
}
static void test_msg_led(u8 on) {
	if (on) {
		GPIO_SetBits(TEST_GPIO_PORT, TEST_GPIO_PIN);
	} else {
		GPIO_ResetBits(TEST_GPIO_PORT, TEST_GPIO_PIN);
	}
}
#endif



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



/* main */
int main() {
	u8 flag = 0;

	io_init();

	clock_init(CLK_MasterPrescaler_HSIDiv2);	/* HSI 8M Hz */

	ext_init();

	uart_init(BUAD);

	key_init();
	pir_init();

	sleep_init();

	msg_init();

	enable_int();

#if QUICK_TEST
	test_init();
#endif


	while (1)  {
#if QUICK_TEST
		test_msg_led(1);
#endif

		disable_int();
		AWU_IdleModeEnable();
		uart_init(BUAD);

		if (event & E_KEY) {
			msg_post_key();
			event &= ~E_KEY;
		}

		switch (state) {
			case S_NO_PERSON:
				if (event & E_PIR) {
					flag = 1;

					state = S_HAS_PERSON;

					timcnt = 0;
					timcnt_has2no = 0;
					break;
				}
				if (event & E_AWU_TIMEOUT) {
					timcnt++;
					if (timcnt * AWU_TIMEOUT_SEC >= RPT_TIMEOUT) {
						flag = 1;
						timcnt = 0;
					}
				}
				break;

			case S_HAS_PERSON:
				if (event & E_PIR) {
					timcnt_has2no = 0;
				}
				if (event & E_AWU_TIMEOUT) {
					timcnt_has2no++;
					timcnt++;

					if (timcnt_has2no * AWU_TIMEOUT_SEC >= HAS_PERSON_TO_NO_PERSON_TIME) {
						flag = 1;

						state = S_NO_PERSON;

						timcnt = 0;
						timcnt_has2no = 0;
						break;
					}

					if (timcnt * AWU_TIMEOUT_SEC >= RPT_TIMEOUT) {
						flag = 1;
						timcnt = 0;
					}
				}
				break;
			default:
				break;
		}
		event = 0;

		if (flag) {
			msg_post_pir(state == S_NO_PERSON ? 0 : 1);
			flag = 0;
		}

		enable_int();

#if QUICK_TEST
		test_msg_led(0);
		sleep(AWU_Timebase_2s);
#else
		sleep(AWU_Timebase_30s);
#endif
	}
}



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

INTERRUPT_HANDLER(USART_RX_IRQHandler, 28) {	/* tx */
	USART_ClearITPendingBit ();
	USART_SendData8 (USART_ReceiveData8());
}
