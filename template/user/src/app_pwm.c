/**
 ******************************************************************************
 * @file    app_pwm.c
 * @brief   电机 PWM 输出：TIM 通道 PWM 占空比控制 + STBY 使能
 ******************************************************************************
 */

#include "app_pwm.h"
#include "pwm_desc.h"
#include <stddef.h>
#include <math.h>

static void app_pwm_stby_init(stby_handler stby_x);
static void app_pwm_motor_init(gpio_in_handler in_x,timer_handler timer_x,gpio_pwm_handler pwm_x);
//static void app_pwm_timer_oc_init(timer_handler timer_x,TIM_OCInitTypeDef*oc_init);
static void app_pwm_in_ctl(gpio_in_handler in_x,int8_t sign);

/**
 * @brief  初始化 PWM 电机驱动：STBY 引脚 + IN 方向引脚 + TIM 输出比较
 * @param  pwm  PWM 句柄
 * @retval None
 */
void app_pwm_init(pwm_handler pwm)
{
	if(pwm==NULL)
	{
		return;
	}
	app_pwm_stby_init(pwm->stby_handler_pwm);
	app_pwm_motor_init(pwm->GPIO_IN,pwm->timer,pwm->GPIO_PWM);
}

/**
 * @brief  设置电机 PWM 占空比和方向
 * @param  pwm    PWM 句柄
 * @param  forward 左右轮选择(LEFT/RIGHT)
 * @param  duty   占空比(-100~100)，符号表示方向
 * @retval None
 */
void app_pwm_motor_set(pwm_handler pwm,forward_t forward,float duty)
{
	if(pwm==NULL||pwm->timer==NULL||pwm->timer->__timer_base==NULL)
	{
		return;
	}
	if(duty>100.0f)
	{
		duty=100.0f;
	}
	else if(duty<-100.0f)
	{
		duty=-100.0f;
	}
	//符号
	int8_t sign=0;
	if(duty>=0&&forward==RIGHT)
	{
		//右边正转
		sign=1;
	}
	else if(duty<0&&forward==RIGHT)
	{
		//右边反转
		sign=-1;
	}
	else if(duty>=0&&forward==LEFT)
	{
		//左边反转
		sign=-1;
	}
	else if(duty<0&&forward==LEFT) 
	{
		//左边正转
		sign=1;
	}
	else 
	{
		return;
	}
	app_pwm_in_ctl(pwm->GPIO_IN,sign);

	duty=fabsf(duty);
	uint32_t ccr=duty/100.0f*pwm->timer->__timer_base->__TIM_Period;
	TIM_SetCompare1(pwm->timer->__timer_base->__tim_x,ccr);
}

static void app_pwm_in_ctl(gpio_in_handler in_x,int8_t sign)
{
	if(in_x==NULL)
	{
		return;
	}
	if(sign>0)
	{
		GPIO_WriteBit(in_x->__GPIOx_1,in_x->__GPIO_Pin_1,Bit_SET);
		GPIO_WriteBit(in_x->__GPIOx_2,in_x->__GPIO_Pin_2,Bit_RESET);
	}
	else 
	{
		GPIO_WriteBit(in_x->__GPIOx_1,in_x->__GPIO_Pin_1,Bit_RESET);
		GPIO_WriteBit(in_x->__GPIOx_2,in_x->__GPIO_Pin_2,Bit_SET);
	}
}


static void app_pwm_stby_init(stby_handler stby_x)
{
	if(stby_x==NULL)
	{
		return;
	}
	GPIO_InitTypeDef GPIO_InStructer;
	GPIO_StructInit(&GPIO_InStructer);
	GPIO_InStructer.GPIO_Mode=stby_x->__GPIO_Mode;
	GPIO_InStructer.GPIO_Pin=stby_x->__GPIO_Pin;
	GPIO_InStructer.GPIO_Speed=stby_x->__GPIO_Speed;
	GPIO_Init(stby_x->__GPIOx,&GPIO_InStructer);
}

static void pwm_stby_ctl(stby_handler stby_x,uint8_t state)
{
	if(stby_x==NULL)
	{
		return;
	}
	if(state)
	{
		GPIO_WriteBit(stby_x->__GPIOx,stby_x->__GPIO_Pin,Bit_SET);
	}
	else 
	{
		GPIO_WriteBit(stby_x->__GPIOx,stby_x->__GPIO_Pin,Bit_RESET);
	}
}

//static void app_pwm_timer_oc_init(timer_handler timer_x,TIM_OCInitTypeDef*oc_init){
//	if(oc_init==NULL)
//	{
//		return;
//	}
//	if(timer_x->__timer_base->__tim_x==TIM1)
//	{
//		TIM_OC1Init(timer_x->__timer_base->__tim_x,oc_init);
//	}
//	else if(timer_x->__timer_base->__tim_x==TIM2)
//	{
//		TIM_OC2Init(timer_x->__timer_base->__tim_x,oc_init);
//	}
//	else if(timer_x->__timer_base->__tim_x==TIM3)
//	{
//		TIM_OC3Init(timer_x->__timer_base->__tim_x,oc_init);
//	}
//	else if(timer_x->__timer_base->__tim_x==TIM4)
//	{
//		TIM_OC4Init(timer_x->__timer_base->__tim_x,oc_init);
//	}
//	else 
//	{
//		return;
//	}
//}

static void app_pwm_motor_init(gpio_in_handler in_x,timer_handler timer_x,gpio_pwm_handler pwm_x)
{
	if(in_x==NULL||timer_x==NULL||timer_x->__timer_base==NULL
	||timer_x->__timer_oc==NULL||pwm_x==NULL)
	{
		return;
	}
	
	//初始化两个in引脚
	GPIO_InitTypeDef GPIO_InStructer;
	GPIO_StructInit(&GPIO_InStructer);
	GPIO_InStructer.GPIO_Mode=in_x->__GPIO_Mode_1;
	GPIO_InStructer.GPIO_Pin=in_x->__GPIO_Pin_1;
	GPIO_InStructer.GPIO_Speed=in_x->__GPIO_Speed_1;
	GPIO_Init(in_x->__GPIOx_1,&GPIO_InStructer);
	GPIO_InStructer.GPIO_Mode=in_x->__GPIO_Mode_2;
	GPIO_InStructer.GPIO_Pin=in_x->__GPIO_Pin_2;
	GPIO_InStructer.GPIO_Speed=in_x->__GPIO_Speed_2;
	GPIO_Init(in_x->__GPIOx_2,&GPIO_InStructer);
	
	//初始化pwm引脚
	GPIO_InStructer.GPIO_Mode=pwm_x->__GPIO_Mode;
	GPIO_InStructer.GPIO_Pin=pwm_x->__GPIO_Pin;
	GPIO_InStructer.GPIO_Speed=pwm_x->__GPIO_Speed;
	GPIO_Init(pwm_x->__GPIOx,&GPIO_InStructer);
	
	//初始化定时器
	/* 3.1 时基单元：参数全部来自 timer_base */
	TIM_TypeDef *tim= timer_x->__timer_base->__tim_x;
	TIM_TimeBaseInitTypeDef tim_base_init;
	TIM_TimeBaseStructInit(&tim_base_init);
	tim_base_init.TIM_CounterMode       = timer_x->__timer_base->__TIM_CounterMode;
	tim_base_init.TIM_Period            = timer_x->__timer_base->__TIM_Period;
	tim_base_init.TIM_Prescaler         = timer_x->__timer_base->__TIM_Prescaler;
	tim_base_init.TIM_ClockDivision     = timer_x->__timer_base->__TIM_ClockDivision;
	tim_base_init.TIM_RepetitionCounter = timer_x->__timer_base->__TIM_RepetitionCounter;
	TIM_TimeBaseInit(tim, &tim_base_init);
 
	/* 3.2 输出比较：把 timer_oc 里的参数搬到 SPL 的结构体 */
	TIM_OCInitTypeDef tim_oc_init;
	TIM_OCStructInit(&tim_oc_init);
	tim_oc_init.TIM_OCMode       = timer_x->__timer_oc->__TIM_OCMode;
	tim_oc_init.TIM_OutputState  = timer_x->__timer_oc->__TIM_OutputState;
	tim_oc_init.TIM_OutputNState = timer_x->__timer_oc->__TIM_OutputNState;
	tim_oc_init.TIM_Pulse        = timer_x->__timer_oc->__TIM_Pulse;
	tim_oc_init.TIM_OCPolarity   = timer_x->__timer_oc->__TIM_OCPolarity;
	tim_oc_init.TIM_OCNPolarity  = timer_x->__timer_oc->__TIM_OCNPolarity;
	tim_oc_init.TIM_OCIdleState  = timer_x->__timer_oc->__TIM_OCIdleState;
	tim_oc_init.TIM_OCNIdleState = timer_x->__timer_oc->__TIM_OCNIdleState;
	//app_pwm_timer_oc_init(timer_x, &tim_oc_init);
	TIM_OC1Init(tim,&tim_oc_init);
 
	/* 3.3 高级定时器要开 MOE，否则引脚不出波形 */
	if(tim == TIM1 || tim == TIM8)
	{
		TIM_CtrlPWMOutputs(tim, ENABLE);
	}
	
	/* 3.4 需要 TRGO 输出时才用（电机这里填的是 Reset，无害） */
	TIM_SelectOutputTrigger(tim, timer_x->__timer_trgo_source);
 
	/* 3.5 闭合定时器总开关 */
	TIM_Cmd(tim, ENABLE);
}


/**
 * @brief  控制电机驱动芯片 STBY 引脚（休眠/唤醒）
 * @param  pwm    PWM 句柄
 * @param  state  1=使能, 0=休眠
 * @retval None
 */
void app_pwm_stby_ctl(pwm_handler pwm,uint8_t state)
{
	if(pwm==NULL)
	{
		return;
	}
	pwm_stby_ctl(pwm->stby_handler_pwm,state);
}
