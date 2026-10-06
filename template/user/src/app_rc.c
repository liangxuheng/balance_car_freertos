/**
 ******************************************************************************
 * @file    app_rc.c
 * @brief   遥控接收模块：串口接收 "move <turn> <speed>" 命令，解析目标速度
 * @author  平衡车项目
 *
 * @details 三缓冲区设计（中断安全）：
 *            intBuf   - ISR 直接写入，禁止在任务中访问
 *            transBuf - ISR 完成一行后拷贝，任务读取后清零
 *            procBuf  - 任务上下文解析
 *          50ms 无新命令自动停车（看门狗逻辑）。
 *
 ******************************************************************************
 */

#include "app_rc.h"
#include "app_rc_desc.h"
#include "my_usart.h"
#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define CMD_MAX_LEN 64

struct rc_buf_TypeDef{
	char intBuf[CMD_MAX_LEN]; // 专门用于中断程序的缓冲区
	char transBuf[CMD_MAX_LEN]; // 在中断程序和进程函数间转运数据的缓冲区
  char procBuf[CMD_MAX_LEN]; // 专门用于进程函数的缓冲区
	volatile bool lineReceivedFlag;
	uint16_t intBufCursor;
};
typedef struct rc_buf_TypeDef* rc_buf_handler;

 struct rc_buf_TypeDef rc_buf;

static rc_buf_handler rc_buf_handler_1=&rc_buf;

static void rc_buf_init(rc_buf_handler rc_x);

static volatile float target_vel=0.0f;
static volatile float target_turn=0.0f;

static void app_rc_task(void*p);

/**
 * @brief  初始化遥控串口、NVIC 中断和接收缓冲区
 * @param  app_rc_x  遥控句柄
 * @retval None
 */
void app_rc_init(app_rc_handler app_rc_x)
{
	if(app_rc_x==NULL)
	{
		return;
	}
	usart_init(app_rc_x->__usart);
	usart_enable_rx_it(app_rc_x->__usart, app_rc_x->__usart_nvic);
	rc_buf_init(rc_buf_handler_1);
	xTaskCreate(app_rc_task,"app_rc_task",configMINIMAL_STACK_SIZE,app_rc_x
	,tskIDLE_PRIORITY+2,NULL);
}

/**
 * @brief  初始化遥控接收缓冲区
 * @param  rc_x  缓冲区句柄
 * @retval None
 */
static void rc_buf_init(rc_buf_handler rc_x)
{
	if(rc_x==NULL)
	{
		return;
	}
	if(rc_x->intBuf)
	{
		memset(rc_x->intBuf,0,sizeof(rc_x->intBuf));
	}
	if(rc_x->procBuf)
	{
		memset(rc_x->procBuf,0,sizeof(rc_x->procBuf));
	}
	if(rc_x->transBuf)
	{
		memset(rc_x->transBuf,0,sizeof(rc_x->transBuf));
	}
	rc_x->intBufCursor=0;
	rc_x->lineReceivedFlag=false;
}

/**
 * @brief  解析遥控命令：move speed omega，50ms 无命令自动停车
 * @param  app_rc_x  遥控句柄
 * @retval None
 */
void app_rc_proc(app_rc_handler app_rc_x)
{
	if(app_rc_x==NULL||app_rc_x->__pid_turn==NULL
		||app_rc_x->__pid_velocity==NULL)
	{
		return;
	}
	static TickType_t last_tick=0;
	TickType_t cur_tick=xTaskGetTickCount();
	if(rc_buf_handler_1->lineReceivedFlag)
	{
		last_tick=cur_tick;
		strcpy(rc_buf_handler_1->procBuf,rc_buf_handler_1->transBuf);
		memset(rc_buf_handler_1->transBuf,0,sizeof(rc_buf_handler_1->transBuf));
		rc_buf_handler_1->lineReceivedFlag=false;
		if(strncasecmp(rc_buf_handler_1->procBuf,"move ",strlen("move "))==0)
		{
			int speed=0,omega_speed=0;
			//Keil 的 MicroLIB 不支持`%[a-zA-Z]` 这种 scanset 格式符
			if(sscanf(rc_buf_handler_1->procBuf+strlen("move "),"%d %d",&omega_speed
				,&speed)==2)
			{
				target_vel=-speed*0.01f*0.7f;
				target_turn=-omega_speed*0.01f*15.0f;
			}
			else 
			{
				target_vel=0.0f;
				target_turn=0.0f;
			}
		}
		else 
		{
			target_vel=0.0f;
			target_turn=0.0f;
		}
	}
	else if(cur_tick-last_tick>=50)
	{
		last_tick=cur_tick;
		target_vel=0.0f;
		target_turn=0.0f;
	}
}

/**
 * @brief  USART3 接收中断：逐字符存入 intBuf，收到 \n 后转存 transBuf
 * @retval None
 */
void USART3_IRQHandler(void)
{
	if(USART_GetITStatus(USART3,USART_IT_RXNE)==SET)
	{
		char ch=USART_ReceiveData(USART3);
		if(rc_buf_handler_1->intBufCursor<=CMD_MAX_LEN-1)
		{
			if(ch!='\n')
			{
				if(rc_buf_handler_1->intBufCursor<CMD_MAX_LEN-1)
				{
					rc_buf_handler_1->intBuf[rc_buf_handler_1->intBufCursor++]=ch;
				}
				else
				{
					memset(rc_buf_handler_1->intBuf,0,sizeof(rc_buf_handler_1->intBuf));
					rc_buf_handler_1->intBufCursor=0;
					rc_buf_handler_1->intBuf[rc_buf_handler_1->intBufCursor++]=ch;
				}
			}
			else
			{
				rc_buf_handler_1->intBuf[rc_buf_handler_1->intBufCursor++]=0;
				strncpy(rc_buf_handler_1->transBuf
				,rc_buf_handler_1->intBuf,rc_buf_handler_1->intBufCursor);
				rc_buf_handler_1->intBufCursor=0;
				rc_buf_handler_1->lineReceivedFlag=true;
//				memset(rc_buf_handler_1->transBuf,0
//				,sizeof(rc_buf_handler_1->transBuf));
			}
		}
		USART_ClearITPendingBit(USART3,USART_IT_RXNE);
	}
}

/**
 * @brief  获取当前目标速度和转向角速度
 * @param  vel   输出：目标速度
 * @param  turn  输出：目标转向角速度
 * @retval None
 */
void app_rc_target_get(float *vel,float *turn)
{
	if(vel)
	{
		*vel=target_vel;
	}
	if(turn)
	{
		*turn=target_turn;
	}
}

/**
 * @brief  遥控任务：每 2ms 解析一次命令
 * @param  p  遥控句柄
 * @retval None
 */
static void app_rc_task(void*p)
{
	TickType_t last_time=xTaskGetTickCount();
	while(true)
	{
		app_rc_proc(p);
		vTaskDelayUntil(&last_time,pdMS_TO_TICKS(2));
	}
}
