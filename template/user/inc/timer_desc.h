#ifndef __TIMER_DESC_H_
#define __TIMER_DESC_H_
#include <stdint.h>
#include "stm32f10x.h"
struct timer_struct;
typedef struct timer_struct* timer_handler;
struct timer_base{
	//定时器编号
	TIM_TypeDef*__tim_x;
	//定时器分频系数
	uint16_t __TIM_Prescaler;
	//定时器计数模式
	uint16_t __TIM_CounterMode;
	//ARR寄存器的计数值
	uint16_t __TIM_Period;
	//分频系数(与死区生成相关)
	uint16_t __TIM_ClockDivision;
	//RCR寄存器的计数值
	uint8_t __TIM_RepetitionCounter; 
};
struct timer_oc{
	//定时器输出比较的模式
	uint16_t __TIM_OCMode;
	//是否开启主通道的输出
	uint16_t __TIM_OutputState;
	//是否开启互补通道的输出
	uint16_t __TIM_OutputNState;
	//ccr寄存器的值
	uint16_t __TIM_Pulse;
	//主通道的极性
	uint16_t __TIM_OCPolarity;
	//互补通道的极性
	uint16_t __TIM_OCNPolarity;
	//主通道的空闲电平
	uint16_t __TIM_OCIdleState;
	//互补通道的空闲电平
	uint16_t __TIM_OCNIdleState;
};
//时钟中断结构体
struct timer_nvic{
	uint8_t nvic_irqn;
};
struct timer_struct{
	struct timer_base* __timer_base;
	//从模式触发信号
	uint16_t __timer_trgo_source;
	struct timer_oc* __timer_oc;
	struct timer_nvic* __nvic;
};
#endif
