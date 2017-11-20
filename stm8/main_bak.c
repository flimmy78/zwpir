


/* Global Variable */
static u8 key_pressed;
static u8 pir;

static u8 min_timeout_2		= 0;
static u8 min_timeout_15	= 0;

static u8 has_person = 0;
static u8 pir_old;



int main() {
	u8 flag = 0;

	key_init();
	pir_init();
	tim_init();

	while (1) {
		if (pir_old == NO_PERSON) { // 先前无人
			if (pir == HAS_PERSON) {			// 现在有人
				flag = 1;

				pir_old = pir;
				min_timeout_15	= 0;
				min_timeout_2		= 0;
			} else {											// 现在无人
				if (min_timeout_15) {						// 超过 15分钟
					flag = 1;

					min_timeout_15 = 0;
				} else {												// 没超过 15分钟
					;
				}
			}
		} else {										// 先前有人
			if (pir == HAS_PERSON) {			// 现在有人
				if (min_timeout_15) {						// 超过 15分钟
					flag = 1;

					min_timeout_15 = 0;
					min_timeout_2  = 0;
				} else {												// 没超过 15分钟
					has_person_cnt++;
					pir_close();
				}
			} else {											// 现在无人
				if (min_timeout_2) {					// 超过2分钟
					flag = 1;

					pir_old = pir;
					min_timeout_2		= 0;
					min_timeout_15	= 0;
				} else {											// 没超过2分钟
					; 
				}
			}
		}

		if (key_pressed) {
			post_msg(1, 0);
			key_pressed = 0;
		}

		if (flag) {
			post_msg(0, pir);
			pir_old = pir;
		}

		sleep();
	}
}

void post_msg(u8 x, u8 y) {

	PB6 = x;
	PB5 = y;
	
	PB7 = 0;
	mdelay(10);
	PB7 = 1;
	mdelay(20);
}


/* 按键间断 */
INTERRUPT_HANDLER(EXTI2_IRQHandler, 10) {  
	key_pressed = 1;

	//清除中断标志位  
}  

/* 红外人感-外部中断  */
INTERRUPT_HANDLER(EXTI2_IRQHandler, 10) {  
	pir = 1;
	//清除中断标志位  
}  


/* 红外人感-中断记数器 */
/*
INTERRUPT_HANDLER(TIM2_UPD_OVF_TRG_BRK_IRQHandler, 19) {  
  TIM2_ClearITPendingBit(TIM2_IT_Update);   
}  
*/

/* 定时器中断 */
INTERRUPT_HANDLER(TIM2_UPD_OVF_TRG_BRK_IRQHandler, 19) {  
  TIM2_ClearITPendingBit(TIM2_IT_Update);   
}  




