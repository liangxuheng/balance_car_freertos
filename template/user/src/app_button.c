/**
 ******************************************************************************
 * @file    app_button.c
 * @brief   按键应用层：注册短按/长按回调
 ******************************************************************************
 */

#include "app_button.h"
#include "my_button_desc.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stddef.h>
#include <stdbool.h>

SemaphoreHandle_t button_scan=NULL;
static void app_button_task(void*p);

/**
 * @brief  初始化按键应用：创建信号量和扫描任务
 * @param  button_x       按键句柄
 * @param  button_usr_read 按键电平读取回调
 * @param  ClickCb         按键点击回调
 * @retval None
 */
void app_button_init(my_button_handler button_x
	,uint8_t (*button_usr_read)(void*),void (*ClickCb)(void*,uint8_t clicks))
{
	if(button_x==NULL||button_usr_read==NULL||ClickCb==NULL)
	{
		return;
	}
	my_button_init(button_x,button_usr_read);
	my_button_set_clickcb(button_x,ClickCb);
	button_scan=xSemaphoreCreateBinary();
	configASSERT(button_scan);
	xTaskCreate(app_button_task,"app_button_task",configMINIMAL_STACK_SIZE
	,button_x,tskIDLE_PRIORITY+2,NULL);
}

/**
 * @brief  按键处理：等待信号量后执行消抖扫描
 * @param  button_x  按键句柄
 * @retval None
 */
static void app_button_proc(my_button_handler button_x)
{
	xSemaphoreTake(button_scan,portMAX_DELAY);
	if(button_x==NULL)
	{
		return;
	}
	my_button_proc(button_x);
}

/**
 * @brief  按键扫描任务：循环处理按键状态机
 * @param  p  按键句柄
 * @retval None
 */
static void app_button_task(void*p)
{
	my_button_handler button_x=(my_button_handler)p;
	while(true)
	{
		app_button_proc(button_x);
	}
}
