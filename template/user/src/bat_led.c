/**
 ******************************************************************************
 * @file    bat_led.c
 * @brief   电池电量指示灯驱动
 ******************************************************************************
 */

#include "bat_led.h"
#include "bat_led_desc.h"
#include <stddef.h>

/**
 * @brief  初始化单个电池指示灯 GPIO
 * @param  bat_led_x  LED 句柄
 * @retval None
 */
void bat_led_init(bat_led_handler bat_led_x)
{
	if(bat_led_x==NULL)
	{
		return;
	}
	GPIO_InitTypeDef GPIO_InStructer;
	GPIO_StructInit(&GPIO_InStructer);
	GPIO_InStructer.GPIO_Mode=bat_led_x->__GPIO_Mode;
	GPIO_InStructer.GPIO_Pin=bat_led_x->__GPIO_Pin;
	GPIO_InStructer.GPIO_Speed=bat_led_x->__GPIO_Speed;
	GPIO_Init(bat_led_x->__GPIO_X,&GPIO_InStructer);
}

/**
 * @brief  批量初始化三个电池指示灯
 * @param  led_init_handler  LED 配置（含 LOW/MID/TOP）
 * @retval None
 */
void bat_led_init_s(bat_led_init_handler led_init_handler)
{
	if(led_init_handler==NULL)
	{
		return;
	}
	bat_led_init(led_init_handler->__led_low);
	bat_led_init(led_init_handler->__led_mid);
	bat_led_init(led_init_handler->__led_top);
}

/**
 * @brief  获取指定档位的 LED 句柄
 * @param  led_init_handler  LED 配置
 * @param  state            LED_LOW/MID/TOP
 * @retval LED 句柄
 */
bat_led_handler bat_led_get_handler(bat_led_init_handler led_init_handler,led_stat_t state)
{
	if(led_init_handler==NULL)
	{
		return NULL;
	}
	if(state==LED_TOP)
	{
		return led_init_handler->__led_top;
	}
	else if(state==LED_LOW)
	{
		return led_init_handler->__led_low;
	}
	else if(state==LED_MID)
	{
		return led_init_handler->__led_mid;
	}
	else 
	{
		return NULL;
	}
}

/**
 * @brief  设置 LED 亮灭
 * @param  led_x  LED 句柄
 * @param  is_on  true=亮, false=灭
 * @retval None
 */
void bat_led_set(bat_led_handler led_x,bool is_on)
{
	if(!led_x)
	{
		return;
	}
	GPIO_WriteBit(led_x->__GPIO_X,led_x->__GPIO_Pin,is_on?Bit_SET:Bit_RESET);
}

