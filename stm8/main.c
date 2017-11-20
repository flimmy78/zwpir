#include "stm8l10x.h"
#include "stm8l10x_gpio.h"

/* Macro */

enum {
	NO_PERSON = 0,
	HAS_PERSON = 1,
};

/* Global Variable */
static u8 key_pressed;
static u8 pir;

static u8 min_timeout_2		= 0;
static u8 min_timeout_15	= 0;

static u8 pir_old_cnt = 0;
static u8 pir_old;

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

/* pb6 from pir & pb5  from key */


/* delay */
static void mdelay(__IO uint16_t nCount) {
	while (nCount != 0) {
		nCount--;
	}
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

	//30S定时唤醒
  AWU_Init(AWU_Timebase_30s);
}
static void sleep_time_enable() {
  AWU_Cmd(ENABLE);
}
static void sleep_time_disabe() {
  AWU_Cmd(DISABLE);
}
static void sleep() {
	halt();
}

/* ext int */
static void enable_int() {
	enableInterrupts();
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
static void pir_close() {
}


/* uart */
static void uart_init() {
	CLK_PeripheralClockConfig (CLK_Peripheral_USART,ENABLE); //enable ext clock
  GPIO_Init(GPIOC,GPIO_Pin_3,GPIO_Mode_Out_PP_High_Fast);
  GPIO_Init(GPIOC,GPIO_Pin_2,GPIO_Mode_In_PU_No_IT);
	USART_Init(115200,USART_WordLength_8D,USART_StopBits_1,USART_Parity_No,USART_Mode_Tx|USART_Mode_Rx);
	USART_ITConfig (USART_IT_RXNE,ENABLE);
	USART_Cmd (ENABLE);
}

static void uart_sendstr(u8 *str) {
	while(*str!=0){
		USART_SendData8(*str);

		while(!USART_GetFlagStatus(USART_FLAG_TXE));

		str++;
	}
}

/* post msg */
static void msg_init() {
	GPIO_Init(MSG_GPIO_PORT, MSG_GPIO_PIN1, GPIO_Mode_Out_PP_Low_Slow);
	GPIO_Init(MSG_GPIO_PORT, MSG_GPIO_PIN2, GPIO_Mode_Out_PP_Low_Slow);
	GPIO_Init(WAK_GPIO_PORT, WAK_GPIO_PIN,	GPIO_Mode_Out_PP_Low_Slow);
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

	mdelay(10);

	//PB7 = 1;
	GPIO_SetBits(WAK_GPIO_PORT, WAK_GPIO_PIN);

	mdelay(20);
}


/* main */
int main() {
	u8 flag = 0;

	ext_init();

	uart_init();
	//tim_init();

	key_init();
	pir_init();

	sleep_init();

	msg_init();

	enable_int();


	uart_sendstr("loop..\r\n");

	while (1) {
		if (pir_old == NO_PERSON) { // before no person
			if (pir == HAS_PERSON) {			// now has person
				flag = 1;

				pir_old = pir;
				min_timeout_15	= 0;
				min_timeout_2		= 0;
			} else {											// now no person
				if (min_timeout_15) {						// > 15 min
					flag = 1;

					min_timeout_15 = 0;
				} else {												// < 15 min
					;
				}
			}
		} else {										// before has person
			if (pir == HAS_PERSON) {			// now has person
				if (min_timeout_15) {						// > 15 min
					flag = 1;

					min_timeout_15 = 0;
					min_timeout_2  = 0;
				} else {												// < 15 min
					pir_old_cnt++;
					pir_close();
				}
			} else {											// now no person
				if (min_timeout_2) {					// > 2 min
					flag = 1;

					pir_old = pir;
					min_timeout_2		= 0;
					min_timeout_15	= 0;
				} else {											// < 2 min
					; 
				}
			}
		}

		if (key_pressed) {
			uart_sendstr("key \r\n");
			msg_post(1, 0);
			key_pressed = 0;
		}

		if (flag) {
			uart_sendstr("pir \r\n");
			msg_post(0, pir);
			pir_old = pir;
		}

		sleep();
	}
}





INTERRUPT_HANDLER(EXTI0_IRQHandler, 8) {   /* pir */
	//pir = EXTI_GetITStatus(PIR_EXTI_STS);
	pir = 1;
	EXTI_ClearITPendingBit (PIR_EXTI_STS);
}  

INTERRUPT_HANDLER(EXTI1_IRQHandler, 9) {  /* key */
	//key_pressed = EXTI_GetITStatus(KEY_EXTI_STS);
	key_pressed = 1;
	EXTI_ClearITPendingBit (KEY_EXTI_STS);
}  

INTERRUPT_HANDLER(AWU_IRQHandler,4) {				/* awu */
	AWU_GetFlagStatus();
	min_timeout_2		= 0;
	min_timeout_15	= 0;
}

INTERRUPT_HANDLER(USART_RX_IRQHandler, 28) {	/* tx */
	USART_ClearITPendingBit ();
	USART_SendData8 (USART_ReceiveData8());
}
