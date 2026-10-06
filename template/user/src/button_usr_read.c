/**
 ******************************************************************************
 * @file    button_usr_read.c
 * @brief   用户按键 GPIO 读取回调
 ******************************************************************************
 */

#include "button_usr_read.h"
#include "stm32f10x.h"
#include "my_button.h"
#include "my_button_desc.h"
#include <stddef.h>

/**
 * @brief  按键电平读取回调：读取 GPIO 状态
 * @param  button  按键句柄
 * @retval 0=未按下, 1=按下
 */
uint8_t button_usr_read(void*button)
{
	if(button==NULL)
	{
		return 0;
	}
	my_button_handler button_handler=(my_button_handler)button;
	return GPIO_ReadInputDataBit(button_handler->__GPIOx
		,button_handler->__GPIO_Pin);
}
