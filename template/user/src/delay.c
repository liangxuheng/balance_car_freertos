/**
  ******************************************************************************
  * @file    delay.c
  * @author  铁头山羊
  * @version V 1.0.0
  * @date    2022年8月30日
  * @brief   延迟函数源文件
  ******************************************************************************
  */

#include "delay.h"
#include "timer.h"

__IO uint64_t ulTicks;

/**
 * @brief  初始化延时定时器（TIM3，1MHz 计数）
 * @param  timer_x  定时器句柄
 * @retval None
 */
void Delay_Init(timer_handler timer_x)
{
	timer_init(timer_x);
}

/**
 * @brief  TIM3 更新中断：毫秒计数器自增
 * @retval None
 */
void TIM3_IRQHandler (void)
{
	if(TIM_GetITStatus(TIM3,TIM_IT_Update)==SET)
	{
		ulTicks++;
		TIM_ClearITPendingBit(TIM3,TIM_IT_Update);
	}
}

/**
 * @brief  获取系统 tick（ms）
 * @retval tick 计数
 */
uint64_t GetTick(void)
{
	return ulTicks;
}

/**
 * @brief  获取微秒级时间戳
 * @retval 微秒计数
 */
uint64_t GetUs(void)
{
	uint64_t last_tick=0;
	uint64_t final_tick=0;
	do{
		last_tick=ulTicks;
		final_tick=ulTicks*1000u+TIM_GetCounter(TIM3);
	}while(last_tick!=ulTicks);
	return final_tick;
}


/**
 * @brief  微秒级忙等延时
 * @param  us  延时微秒数
 * @retval None
 */
void DelayUs(uint64_t us)
{
	uint64_t expire=GetUs()+us+1;
	while(GetUs()<expire);
}
