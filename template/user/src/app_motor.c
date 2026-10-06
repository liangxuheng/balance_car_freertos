/**
 ******************************************************************************
 * @file    app_motor.c
 * @brief   电机应用层：左右轮速度环 PID + PWM 输出 + 编码器测速
 * @author  平衡车项目
 *
 * @details 每个轮子独立一个速度环 PID，由 app_control.c 设定目标角速度。
 *          app_motor_proc 每 1ms 调用一次：读编码器 → PID 计算 → 占空比输出。
 *          PWM 占空比根据电池电压归一化，保证不同电压下出力一致。
 *
 ******************************************************************************
 */

#include "app_motor.h"
#include "app_motor_desc.h"
#include "app_encoder.h"
#include "app_pwm.h"
#include "app_vbat.h"
#include "pid.h"
#include "delay.h"
#include <stddef.h>

static void motor_side_init(motor_handler_side motor_side_x);

/**
 * @brief  初始化左右轮的编码器和 PWM
 * @param  motor_x  电机句柄
 * @retval None
 */
void app_motor_init(motor_handler motor_x)
{
	if (motor_x == NULL) return;
	motor_side_init(motor_x->motor_left);
	motor_side_init(motor_x->motor_right);
}

/**
 * @brief  初始化单侧电机：编码器 + PWM
 * @param  motor_side_x  单侧电机句柄
 * @retval None
 */
static void motor_side_init(motor_handler_side motor_side_x)
{
	if (motor_side_x == NULL) return;
	app_encoder_init(motor_side_x->encoder);
	app_pwm_init(motor_side_x->pwm);
}

/**
 * @brief  使能/失能电机驱动（STBY 脚），同时复位两轮 PID
 * @param  motor_x  电机句柄
 * @param  on      true=启动, false=停止
 * @retval None
 */
void app_motor_cmd(motor_handler motor_x, bool on)
{
	if (motor_x == NULL) return;
	if (motor_x->motor_left == NULL || motor_x->motor_right == NULL) return;
	app_pwm_stby_ctl(motor_x->motor_left->pwm, on);
	app_pwm_stby_ctl(motor_x->motor_right->pwm, on);
	pid_reset(motor_x->motor_left->pid);
	pid_reset(motor_x->motor_right->pid);
}

/**
 * @brief  设置目标角速度（速度环 Setpoint）
 * @param  motor_x  电机句柄
 * @param  forward  左轮/右轮/双轮
 * @param  omega    目标角速度 rad/s
 * @retval None
 */
void app_motor_set_omega(motor_handler motor_x, motor_forward_t forward, float omega)
{
	if (motor_x == NULL) return;
	if (forward == MOTOR_LEFT)
	{
		if (motor_x->motor_left) pid_setSp(motor_x->motor_left->pid, omega);
	}
	else if (forward == MOTOR_RIGHT)
	{
		if (motor_x->motor_right) pid_setSp(motor_x->motor_right->pid, omega);
	}
	else if (forward == MOTOR_BOTH)
	{
		if (motor_x->motor_left && motor_x->motor_right)
		{
			pid_setSp(motor_x->motor_left->pid, omega);
			pid_setSp(motor_x->motor_right->pid, omega);
		}
	}
}

/**
 * @brief  执行一次速度环 PID 并输出 PWM（1ms 周期调用）
 * @param  motor_x  电机句柄
 * @retval None
 */
void app_motor_proc(motor_handler motor_x)
{
	if (motor_x == NULL) return;

	float omega_L = 0.0f, omega_R = 0.0f;
	float ua_L = 0.0f, ua_R = 0.0f;

	if (motor_x->motor_left)
	{
		omega_L = encoder_get_speed(motor_x->motor_left->encoder);
		ua_L = pid_compute(motor_x->motor_left->pid, omega_L);
	}
	if (motor_x->motor_right)
	{
		omega_R = encoder_get_speed(motor_x->motor_right->encoder);
		ua_R = pid_compute(motor_x->motor_right->pid, omega_R);
	}

	/* 电池电压归一化：PID 输出的电压量除以实际电压 = 占空比 */
	float vbat = app_vbat_get(motor_x->adc_vbat);
	app_pwm_motor_set(motor_x->motor_left->pwm, LEFT, vbat ? ua_L / vbat * 100.0f : 0);
	app_pwm_motor_set(motor_x->motor_right->pwm, RIGHT, vbat ? ua_R / vbat * 100.0f : 0);
}

/**
 * @brief  读取编码器实际角速度
 * @param  motor_x  电机句柄
 * @param  forward  左轮/右轮/双轮
 * @param  res      输出缓冲区
 * @param  size     缓冲区大小
 * @retval 成功读取的电机数量
 */
uint8_t app_motor_getomega(motor_handler motor_x, motor_forward_t forward, float *res, uint8_t size)
{
	if (motor_x == NULL || res == NULL || size == 0) return 0;

	if (forward == MOTOR_LEFT)
	{
		if (motor_x->motor_left) { *res = encoder_get_speed(motor_x->motor_left->encoder); return 1; }
		return 0;
	}
	else if (forward == MOTOR_RIGHT)
	{
		if (motor_x->motor_right) { *res = encoder_get_speed(motor_x->motor_right->encoder); return 1; }
		return 0;
	}
	else if (forward == MOTOR_BOTH)
	{
		if (motor_x->motor_left == NULL || motor_x->motor_right == NULL) return 0;
		res[0] = encoder_get_speed(motor_x->motor_left->encoder);
		if (size == 1) return 1;
		res[1] = encoder_get_speed(motor_x->motor_right->encoder);
		return 2;
	}
	return 0;
}
