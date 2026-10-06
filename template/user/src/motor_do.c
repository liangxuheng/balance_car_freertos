/**
 ******************************************************************************
 * @file    motor_do.c
 * @brief   电机驱动任务：通过队列接收命令，1ms 周期执行 PID 输出
 * @author  平衡车项目
 *
 * @details 命令队列解耦：上层模块（控制环/按键）只往队列发命令，
 *          真正的 PWM 输出在 motor_do_task 里统一执行，避免多线程
 *          同时写电机寄存器。每 1ms 调用 app_motor_proc 更新占空比。
 *
 ******************************************************************************
 */

#include "motor_do.h"
#include "motor_do_desc.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "app_motor.h"
#include "pid.h"

static QueueHandle_t cmd_queue=NULL;

static void motor_do_task(void*p)
{
	motor_handler motor_x=(motor_handler)p;
	TickType_t last_tick=xTaskGetTickCount();
	while(true)
	{
		struct motor_do_TypeDef motor_do_temp={0};
		while(xQueueReceive(cmd_queue,&motor_do_temp,0)==pdPASS)
		{
			switch(motor_do_temp.type_t)
			{
				case MOTOR_DO_CMD:
				{
					app_motor_cmd(motor_do_temp.motor_do_cmd.motor_x,motor_do_temp.motor_do_cmd.cmd);
					break;
				}
				case MOTOR_DO_SET:
				{
					app_motor_set_omega(motor_do_temp.motor_do_set.motor_x
					,MOTOR_BOTH,motor_do_temp.motor_do_set.value);
					break;
				}
				default:
				{
					break;
				}
			}
		}
		app_motor_proc(motor_x);
		vTaskDelayUntil(&last_tick,pdMS_TO_TICKS(1));
	}
}

void motor_do_init(motor_handler motor_x)
{
	if(motor_x==NULL)
	{
		return;
	}
	cmd_queue=xQueueCreate(24,sizeof(struct motor_do_TypeDef));
	configASSERT(cmd_queue);
	xTaskCreate(motor_do_task,"motor_do_task",configMINIMAL_STACK_SIZE*2,motor_x
	,tskIDLE_PRIORITY+4,NULL);
}

void motor_do_set_do_cmd(motor_handler motor_x,bool on)
{
	if(motor_x==NULL)
	{
		return;
	}
	struct motor_do_TypeDef motor_do_temp={0};
	motor_do_temp.type_t=MOTOR_DO_CMD;
	motor_do_temp.motor_do_cmd.motor_x=motor_x;
	motor_do_temp.motor_do_cmd.cmd=on;
	xQueueSend(cmd_queue,&motor_do_temp,0);
}

void motor_do_set_do_set(motor_handler motor_x,float value)
{
	if(motor_x==NULL)
	{
		return;
	}
	struct motor_do_TypeDef motor_do_temp={0};
	motor_do_temp.type_t=MOTOR_DO_SET;
	motor_do_temp.motor_do_set.motor_x=motor_x;
	motor_do_temp.motor_do_set.value=value;
	xQueueSend(cmd_queue,&motor_do_temp,0);
}
