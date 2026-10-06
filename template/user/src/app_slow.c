/**
 ******************************************************************************
 * @file    app_slow.c
 * @brief   慢周期任务：用 FreeRTOS 软件定时器触发电池采样和按键扫描
 * @author  平衡车项目
 *
 * @details 200ms 电池采样 + 20ms 按键扫描。定时器回调只发信号量，
 *          实际工作在 work_queue 里执行，避免占用定时器服务任务。
 *
 ******************************************************************************
 */

#include "app_slow.h"
#include "app_vbat.h"
#include "app_button.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "work_queue.h"
#include "semphr.h"
#include <stddef.h>

extern SemaphoreHandle_t button_scan;
extern SemaphoreHandle_t semaphore_update_vbat;

typedef void (*callback_t)(void);

/**
 * @brief  定时器回调中转：在 work_queue 上下文执行实际函数
 * @param  p  函数指针
 * @retval None
 */
static void time_execute(void *p)
{
	callback_t func = (callback_t)p;
	func();
}

/**
 * @brief  FreeRTOS 软件定时器回调：把工作投递到 work_queue
 * @param  time  定时器句柄
 * @retval None
 */
static void os_time_callback(TimerHandle_t time)
{
	callback_t func = (callback_t)pvTimerGetTimerID(time);
	work_queue_push(time_execute, func);
}

/**
 * @brief  电池采样信号量释放包装
 * @retval None
 */
static void bat_work_warp(void)
{
	xSemaphoreGive(semaphore_update_vbat);
}

/**
 * @brief  按键扫描信号量释放包装
 * @retval None
 */
static void button_work_warp(void)
{
	xSemaphoreGive(button_scan);
}

static TimerHandle_t s_bat_handler = NULL;
static TimerHandle_t s_button_handler = NULL;

/**
 * @brief  创建并启动电池采样(200ms)和按键扫描(20ms)软件定时器
 * @retval None
 */
void slow_tasks_init(void)
{
	s_bat_handler = xTimerCreate("bat_timer", pdMS_TO_TICKS(200), pdTRUE
		, (void *)bat_work_warp, os_time_callback);
	s_button_handler = xTimerCreate("button_timer", pdMS_TO_TICKS(20), pdTRUE
		, (void *)button_work_warp, os_time_callback);
	xTimerStart(s_bat_handler, 0);
	xTimerStart(s_button_handler, 0);
}
