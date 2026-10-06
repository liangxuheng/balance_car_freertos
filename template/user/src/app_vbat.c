/**
 ******************************************************************************
 * @file    app_vbat.c
 * @brief   电池电压监测：ADC 采样 + 低电压报警 LED
 ******************************************************************************
 */

#include "app_vbat.h"
#include "timer.h"
#include "adc.h"
#include "bat_led.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include <stdint.h>
#include <stddef.h>
#include "delay.h"

static TickType_t lastTime = 0; // 上次LED切换的时间
static uint8_t  stage = 0; // LED当前的状态，0-熄灭，1-点亮
static float vbat=0.0f;
SemaphoreHandle_t semaphore_update_vbat=NULL;

static void vbat_task(void*p);

typedef struct {
	adc_handler adc_x;
	bat_led_init_handler led_init_handler;
}args_t;

/**
 * @brief  初始化电池监控：定时器+ADC+LED，创建电压采样任务
 * @param  timer_x   定时器句柄
 * @param  adc_x     ADC 句柄
 * @param  led_init_handler  LED 初始化配置
 * @retval None
 */
void app_vbat_init(timer_handler timer_x,adc_handler adc_x,bat_led_init_handler led_init_handler)
{
	timer_init(timer_x);
	adc_init(adc_x);
//	bat_led_init();
	bat_led_init_s(led_init_handler);
	semaphore_update_vbat=xSemaphoreCreateBinary();
	configASSERT(semaphore_update_vbat);
	vbat=vbat_get(adc_x);
	static args_t args={0};
	args.adc_x=adc_x;
	args.led_init_handler=led_init_handler;
	xTaskCreate(vbat_task,"vbat_task",configMINIMAL_STACK_SIZE,&args
	,tskIDLE_PRIORITY+2,NULL);
}

/**
 * @brief  获取当前电池电压
 * @param  adc_x  ADC 句柄（未使用）
 * @retval 电池电压(V)
 */
float app_vbat_get(adc_handler adc_x)
{
	return vbat;
}

//电源指示灯监控任务
static void app_vbat_proc(bat_led_init_handler led_init_handler,adc_handler adc_x)
{
	if(led_init_handler==NULL||adc_x==NULL)
	{
		return;
	}
	TickType_t now_tick=xTaskGetTickCount();
	xSemaphoreTake(semaphore_update_vbat,portMAX_DELAY);
	vbat=vbat_get(adc_x);
	if(vbat>7.9f)
	{
		bat_led_set(bat_led_get_handler(led_init_handler,LED_LOW),true);
		bat_led_set(bat_led_get_handler(led_init_handler,LED_MID),true);
		bat_led_set(bat_led_get_handler(led_init_handler,LED_TOP),true);
	}
	else if(vbat>7.4f)
	{
		bat_led_set(bat_led_get_handler(led_init_handler,LED_LOW),true);
		bat_led_set(bat_led_get_handler(led_init_handler,LED_MID),true);
		bat_led_set(bat_led_get_handler(led_init_handler,LED_TOP),false);
	}
	else if(vbat>7)
	{
		bat_led_set(bat_led_get_handler(led_init_handler,LED_LOW),true);
		bat_led_set(bat_led_get_handler(led_init_handler,LED_MID),false);
		bat_led_set(bat_led_get_handler(led_init_handler,LED_TOP),false);
	}
	else if(vbat>6.5)
	{
		bat_led_set(bat_led_get_handler(led_init_handler,LED_LOW),false);
		bat_led_set(bat_led_get_handler(led_init_handler,LED_MID),false);
		bat_led_set(bat_led_get_handler(led_init_handler,LED_TOP),false);
	}
	else 
	{
		if(now_tick-lastTime>pdMS_TO_TICKS(200))
		{
			switch(stage)
			{
				case 0:
				{
					bat_led_set(bat_led_get_handler(led_init_handler,LED_LOW),false);
					bat_led_set(bat_led_get_handler(led_init_handler,LED_MID),false);
					bat_led_set(bat_led_get_handler(led_init_handler,LED_TOP),false);
					stage=1;
					break;
				}
				case 1:{
						bat_led_set(bat_led_get_handler(led_init_handler,LED_LOW),true);
						bat_led_set(bat_led_get_handler(led_init_handler,LED_MID),true);
						bat_led_set(bat_led_get_handler(led_init_handler,LED_TOP),true);
						stage=0;
						break;
				}
				default:{
					break;
				}
			}
			lastTime=now_tick;
		}
	}
}

static void vbat_task(void*p)
{
	args_t* args=(args_t*)p;
	while(true)
	{
		app_vbat_proc(args->led_init_handler,args->adc_x);
	}
}
