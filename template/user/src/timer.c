/**
 ******************************************************************************
 * @file    timer.c
 * @brief   定时器时基初始化：配置预分频/周期/更新中断
 ******************************************************************************
 */

#include "timer_desc.h"
#include "timer.h"
#include <stddef.h>
#include <string.h>

static void timer_nvic_init(struct timer_nvic*nvic_handler,struct timer_base *timer_x);

/**
 * @brief  初始化定时器：配置时基和更新中断
 * @param  timer_x  定时器句柄
 * @retval None
 */
void timer_init(timer_handler timer_x)
{
	if(timer_x==NULL||timer_x->__timer_base==NULL)
	{
		return;
	}
	TIM_TimeBaseInitTypeDef tim_timerbase_struct;
	TIM_TimeBaseStructInit(&tim_timerbase_struct);
	tim_timerbase_struct.TIM_ClockDivision=timer_x->__timer_base->__TIM_ClockDivision;
	tim_timerbase_struct.TIM_CounterMode=timer_x->__timer_base->__TIM_CounterMode;
	tim_timerbase_struct.TIM_Period=timer_x->__timer_base->__TIM_Period;
	tim_timerbase_struct.TIM_Prescaler=timer_x->__timer_base->__TIM_Prescaler;
	tim_timerbase_struct.TIM_RepetitionCounter=timer_x->__timer_base->__TIM_RepetitionCounter;
	TIM_TimeBaseInit(timer_x->__timer_base->__tim_x,&tim_timerbase_struct);
	TIM_SelectOutputTrigger(timer_x->__timer_base->__tim_x,timer_x->__timer_trgo_source);
	TIM_Cmd(timer_x->__timer_base->__tim_x,ENABLE);
	timer_nvic_init(timer_x->__nvic,timer_x->__timer_base);
}

/**
 * @brief  初始化定时器 NVIC 中断
 * @param  nvic_handler  NVIC 配置
 * @param  timer_x       定时器硬件
 * @retval None
 */
static void timer_nvic_init(struct timer_nvic*nvic_handler,struct timer_base *timer_x)
{
	if(nvic_handler==NULL||timer_x==NULL)
	{
		return;
	}
	NVIC_InitTypeDef nvic_InStruct;
	memset(&nvic_InStruct,0,sizeof(NVIC_InitTypeDef));
	nvic_InStruct.NVIC_IRQChannel=nvic_handler->nvic_irqn;
	nvic_InStruct.NVIC_IRQChannelCmd=ENABLE;
	nvic_InStruct.NVIC_IRQChannelPreemptionPriority=15;
	nvic_InStruct.NVIC_IRQChannelSubPriority=0;
	NVIC_Init(&nvic_InStruct);
	TIM_ClearITPendingBit(timer_x->__tim_x,TIM_IT_Update);
	TIM_ITConfig(timer_x->__tim_x,TIM_IT_Update,ENABLE);
	TIM_SetCounter(timer_x->__tim_x,0);
}
