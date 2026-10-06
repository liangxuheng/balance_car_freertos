/**
 ******************************************************************************
 * @file    main.c
 * @brief   平衡车程序入口：创建初始化任务，启动 FreeRTOS 调度器
 * @author  平衡车项目
 *
 * @details 任务优先级分配：
 *            init_task  : 优先级 5（最高，执行完自动删除）
 *            gatekeeper : 优先级 4（状态机/紧急处理）
 *            balance    : 优先级 3（平衡环，1ms 周期）
 *            rc         : 优先级 2（遥控接收解析）
 *            work_queue : 优先级 1（慢操作队列，电池采样等）
 *
 ******************************************************************************
 */

#include "board.h"
#include "button_usr_read.h"
#include "work_queue.h"
#include "app_slow.h"
#include "delay.h"
#include "app_vbat.h"
#include "app_button.h"
#include "app_motor.h"
#include "my_usart.h"
#include "app_mpu6050.h"
#include "app_control.h"
#include "app_rc.h"
#include "motor_do.h"
#include "FreeRTOS.h"
#include "task.h"

static uint8_t state = 0;

/**
 * @brief  用户按键回调：短按切换启停状态
 * @param  button  按键句柄
 * @param  clicks  点击次数（1=短按）
 */
static void button_clicked_cb(void *button, uint8_t clicks)
{
	if (!button || clicks != 1) return;
	state = !state;
	app_control_reset(app_control_1);
	motor_do_set_do_cmd(motor_handler_my, state);
	motor_do_set_do_set(motor_handler_my, 0);
}

/**
 * @brief  初始化任务：板级外设初始化后自动删除
 * @param  p  任务参数（未使用）
 */
static void init_task(void *p)
{
	(void)p;
	board_init();
	Delay_Init(timer_us);
	app_vbat_init(timer_1, adc_1, __bat_led_init);
	app_button_init(my_button_1, button_usr_read, button_clicked_cb);
	app_motor_init(motor_handler_my);
	usart_init(usart_1);
	app_mpu6050_init(mpu6050_1);
	app_control_init(app_control_1);
	app_rc_init(app_rc_1);
	slow_tasks_init();
	work_queue_init();
	motor_do_init(motor_handler_my);
	vTaskDelete(NULL);
}

int main(void)
{
	xTaskCreate(init_task, "init_task", configMINIMAL_STACK_SIZE * 2, NULL
		, tskIDLE_PRIORITY + 4, NULL);
	vTaskStartScheduler();
	while (1)
	{
	}
}
