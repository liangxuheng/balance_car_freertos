/**
 ******************************************************************************
 * @file    app_encoder.c
 * @brief   编码器测速：正交解码模式，计算实际角速度
 ******************************************************************************
 */

#include "app_encoder.h"
#include "app_encoder_desc.h"
#include "delay.h"
#include <stddef.h>
#include <string.h>

typedef enum{
	FORWARD_ROTATION,
	REVERSE_ROTATION,
	FORWARD_TO_REVERSE,
	REVERSE_TO_FORWARD,
}encoder_sign_t;

static volatile int64_t encoder_cnt_r=0;
static volatile int64_t encoder_cnt_l=0;
static volatile encoder_sign_t sign_left=FORWARD_ROTATION;
static volatile encoder_sign_t sign_right=FORWARD_ROTATION;
static volatile uint64_t tick_cur_l=0, tick_last_l=0;
static volatile uint64_t tick_cur_r=0, tick_last_r=0;

static void app_encoder_gpio_init(encoder_gpio_handler);
static void app_encoder_exti_init(encoder_exti_handler
	,encoder_gpio_handler);
static void app_encoder_nvic_init(encoder_nvic_handler);

/**
 * @brief  初始化编码器：GPIO、EXTI、NVIC
 * @param  encoder_x  编码器句柄
 * @retval None
 */
void app_encoder_init(encoder_handler encoder_x)
{
	if(encoder_x==NULL)
	{
		return;
	}
	app_encoder_gpio_init(encoder_x->__gpio_1);
	app_encoder_gpio_init(encoder_x->__gpio_2);
	app_encoder_exti_init(encoder_x->__exti,encoder_x->__gpio_1);
	if(encoder_x->__is_interrupt)
	{
		app_encoder_nvic_init(encoder_x->__nvic);
	}
}

static float encoder_get_pos_ctl(encoder_t forward)
{
	//这个返回的结果是轮胎的转过的角度,不再是编码器的计数值
	if(forward==EN_LEFT)
	{
		return encoder_cnt_l/22.0f/(30613.0f/1500.0f)*360.0f;
	}
	else if(forward==EN_RIGHT)
	{
		return encoder_cnt_r/22.0f/(30613.0f/1500.0f)*360.0f;
	}
	else 
	{
		return 0;
	}
}


/**
 * @brief  获取轮胎转过的角度（度）
 * @param  encoder_x  编码器句柄
 * @retval 角度(度)
 */
float encoder_get_pos(encoder_handler encoder_x)
{
	if(encoder_x==NULL)
	{
		return 0;
	}
	return encoder_get_pos_ctl(encoder_x->forward);
}

static float encoder_get_speed_ctl(encoder_handler encoder_x)
{
	if(encoder_x->forward==EN_LEFT)
	{
		__disable_irq();
		encoder_sign_t sign_left_temp=sign_left;
		uint64_t tick_cur_l_temp=tick_cur_l;
		uint64_t tick_last_l_temp=tick_last_l;
		__enable_irq();
		float T=0;
		uint64_t now=GetUs();
		if(tick_cur_l_temp-tick_last_l_temp>now-tick_cur_l_temp)
		{
			T=tick_cur_l_temp-tick_last_l_temp;
		}
		else {
			T=now-tick_cur_l_temp;
		}
		if(sign_left_temp==FORWARD_ROTATION)
		{
//			return 1.0f/((T)*1.0e-6f)/22.0f/(30613.0f/1500.0f)*360.0f;
				return 1.0f/((T)*1.0e-6f)/22.0f/(30613.0f/1500.0f)*6.2831853f;
		}
		else if(sign_left_temp==REVERSE_ROTATION)
		{
//			return -1.0f/((T)*1.0e-6f)/22.0f/(30613.0f/1500.0f)*360.0f;
				return -1.0f/((T)*1.0e-6f)/22.0f/(30613.0f/1500.0f)*6.2831853f;
		}
		else if(sign_left_temp==REVERSE_TO_FORWARD||sign_left_temp==FORWARD_TO_REVERSE)
		{
			return 0;
		}
		else {
			return 0;
		}
	}
	else if(encoder_x->forward==EN_RIGHT)
	{
		__disable_irq();
		encoder_sign_t sign_right_temp=sign_right;
		uint64_t tick_cur_r_temp=tick_cur_r;
		uint64_t tick_last_r_temp=tick_last_r;
		__enable_irq();
		float T=0;
		uint64_t now=GetUs();
		if(tick_cur_r_temp-tick_last_r_temp>now-tick_cur_r_temp)
		{
			T=tick_cur_r_temp-tick_last_r_temp;
		}
		else {
			T=now-tick_cur_r_temp;
		}
		if(sign_right_temp==FORWARD_ROTATION)
		{
//			return 1.0f/((T)*1.0e-6f)/22.0f/(30613.0f/1500.0f)*360.0f;
				return 1.0f/((T)*1.0e-6f)/22.0f/(30613.0f/1500.0f)*6.2831853f;
		}
		else if(sign_right_temp==REVERSE_ROTATION)
		{
//			return -1.0f/((T)*1.0e-6f)/22.0f/(30613.0f/1500.0f)*360.0f;
				return -1.0f/((T)*1.0e-6f)/22.0f/(30613.0f/1500.0f)*6.2831853f;
		}
		else if(sign_right_temp==REVERSE_TO_FORWARD||sign_right_temp==FORWARD_TO_REVERSE)
		{
			return 0;
		}
		else {
			return 0;
		}
	}
	else 
	{
		return 0;
	}
}

/**
 * @brief  获取轮胎角速度（rad/s）
 * @param  encoder_x  编码器句柄
 * @retval 角速度(rad/s)
 */
float encoder_get_speed(encoder_handler encoder_x)
{
	if(encoder_x==NULL)
	{
		return 0;
	}
	return encoder_get_speed_ctl(encoder_x);
}


static void app_encoder_gpio_init(encoder_gpio_handler gpio_x){
	if(gpio_x==NULL)
	{
		return;
	}
	if(gpio_x->__GPIOx==GPIOB&&(gpio_x->__GPIO_Pin==GPIO_Pin_3
		||gpio_x->__GPIO_Pin==GPIO_Pin_4))
	{
		//因为pb3和pb4默认是jtag的调试引脚,因此要关闭
		GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
	}
	GPIO_InitTypeDef GPIO_InStructer;
	GPIO_StructInit(&GPIO_InStructer);
	GPIO_InStructer.GPIO_Mode=gpio_x->__GPIO_Mode;
	GPIO_InStructer.GPIO_Pin=gpio_x->__GPIO_Pin;
	GPIO_InStructer.GPIO_Speed=gpio_x->__GPIO_Speed;
	GPIO_Init(gpio_x->__GPIOx,&GPIO_InStructer);
}

static void app_auto_exti_pinsource(encoder_gpio_handler gpio_x)
{
	uint8_t port_source=0;
	uint8_t i=0;
 
	if(gpio_x->__GPIOx == NULL)
	{
		return;
	}
 
	/* #1. 端口 → 端口源 */
	if(gpio_x->__GPIOx == GPIOA)      port_source = GPIO_PortSourceGPIOA;
	else if(gpio_x->__GPIOx == GPIOB) port_source = GPIO_PortSourceGPIOB;
	else if(gpio_x->__GPIOx == GPIOC) port_source = GPIO_PortSourceGPIOC;
	else if(gpio_x->__GPIOx == GPIOD) port_source = GPIO_PortSourceGPIOD;
	else if(gpio_x->__GPIOx == GPIOE) port_source = GPIO_PortSourceGPIOE;
	else return;                     /* 不是可用的 GPIO 端口 */
 
	/* #2. 引脚掩码 → 引脚源编号（0 ~ 15） */
	/* GPIO_Pin_N 是 (1<<N)，而 GPIO_PinSourceN 的数值就是 N， */
	/* 所以数出掩码里第几个 bit 置位即可，不用写 16 分支的 switch */
	for(i = 0; i < 16; i++)
	{
		if(gpio_x->__GPIO_Pin & (uint16_t)(1 << i))
		{
			GPIO_EXTILineConfig(port_source, (uint8_t)i);
		}
	}
}


static void app_encoder_exti_init(encoder_exti_handler exti_x
	,encoder_gpio_handler gpio_x)
{
	if(gpio_x==NULL||exti_x==NULL)
	{
		return;
	}
	app_auto_exti_pinsource(gpio_x);
	EXTI_InitTypeDef exti_InStructer;
	EXTI_StructInit(&exti_InStructer);
	exti_InStructer.EXTI_Line=exti_x->__EXTI_Line;
	exti_InStructer.EXTI_LineCmd=exti_x->__EXTI_LineCmd;
	exti_InStructer.EXTI_Mode=exti_x->__EXTI_Mode;
	exti_InStructer.EXTI_Trigger=exti_x->__EXTI_Trigger;
	EXTI_Init(&exti_InStructer);
}

static void app_encoder_nvic_init(encoder_nvic_handler nvic_x)
{
	if(nvic_x==NULL)
	{
		return;
	}
	NVIC_InitTypeDef nvic_InStructer;
	memset(&nvic_InStructer,0,sizeof(NVIC_InitTypeDef));
	nvic_InStructer.NVIC_IRQChannel=nvic_x->__NVIC_IRQChannel;
	nvic_InStructer.NVIC_IRQChannelCmd=ENABLE;
	nvic_InStructer.NVIC_IRQChannelPreemptionPriority=5;
	nvic_InStructer.NVIC_IRQChannelSubPriority=0;
	NVIC_Init(&nvic_InStructer);
}

/**
 * @brief  右编码器 EXTI3 中断：AB 相判断正反转，更新计数值和方向
 * @retval None
 */
void EXTI3_IRQHandler(void)
{
	EXTI_ClearFlag(EXTI_Line3); // 对中断标志位清零
	
	uint8_t a = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_3); // A相的当前电压
	uint8_t b = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_4); // B相的当前电压
	
	tick_last_r=tick_cur_r;
	tick_cur_r=GetUs();
	
	if((a == Bit_SET && b == Bit_RESET) || (a == Bit_RESET && b == Bit_SET)) // 现在轮胎正转
	{
		encoder_cnt_r++;
		
		if(sign_right==REVERSE_ROTATION) // 之前轮胎是反转
		{
			sign_right=REVERSE_TO_FORWARD;
		}
		else
		{
			sign_right=FORWARD_ROTATION;
		}
	}
	else // 现在轮胎是反转
	{
		encoder_cnt_r--;
		
		if(sign_right==FORWARD_ROTATION) // 之前轮胎是正转
		{
			sign_right=FORWARD_TO_REVERSE;
		}
		else
		{
			sign_right=REVERSE_ROTATION;
		}
	}
}

/**
 * @brief  左编码器 EXTI14 中断：AB 相判断正反转，更新计数值和方向
 * @retval None
 */
void EXTI15_10_IRQHandler(void)
{
	if(EXTI_GetFlagStatus(EXTI_Line14) == SET)
	{
		EXTI_ClearFlag(EXTI_Line14); // 对标志位进行清零
		
		uint8_t a = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14); // A相的当前电压
		uint8_t b = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_15); // B相的当前电压
		
		tick_last_l=tick_cur_l;
		tick_cur_l=GetUs();
		
		if((a == Bit_SET && b == Bit_RESET) || (a == Bit_RESET && b == Bit_SET)) // 现在轮胎反转
		{
			encoder_cnt_l--;
			
			if(sign_left==FORWARD_ROTATION) // 之前轮胎是正转
			{
				sign_left=FORWARD_TO_REVERSE;
			}
			else
			{
				sign_left=REVERSE_ROTATION;
			}
		}
		else // 现在轮胎是正转
		{
			encoder_cnt_l++;
			
			if(sign_left==REVERSE_ROTATION) // 之前轮胎是反转，现在轮胎是正转
			{
				sign_left=REVERSE_TO_FORWARD;
			}
			else
			{
				sign_left=FORWARD_ROTATION;
			}
		}
	}
}
