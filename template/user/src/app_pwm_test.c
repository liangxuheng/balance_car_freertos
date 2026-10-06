/**
 ******************************************************************************
 * @file    app_pwm_test.c
 * @brief   PWM 输出测试：左右轮正反转 demo
 ******************************************************************************
 */

#include "app_pwm_test.h"
#include "delay.h"
void pwm_test_pro(pwm_handler pwm_x_1,pwm_handler pwm_x_2)
{
	if(!pwm_x_1||!pwm_x_2)
	{
		return;
	}
	static uint64_t last_tick=0;
	uint64_t now_tick=GetTick();
	if(now_tick-last_tick>6000)
	{
		app_pwm_motor_set(pwm_x_1,LEFT,90);
		app_pwm_motor_set(pwm_x_2,RIGHT,90);
		last_tick=now_tick;
	}
	else if(now_tick-last_tick>4000)
	{
		app_pwm_motor_set(pwm_x_1,LEFT,60);
		app_pwm_motor_set(pwm_x_2,RIGHT,60);
	}
	else if(now_tick-last_tick>2000)
	{
		app_pwm_motor_set(pwm_x_1,LEFT,30);
		app_pwm_motor_set(pwm_x_2,RIGHT,30);
	}
	else 
	{
		return;
	}
}
