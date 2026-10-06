/**
 ******************************************************************************
 * @file    app_control.c
 * @brief   平衡车三级串级 PID 控制：速度外环 → 角度中环 → 角度微分内环
 * @author  平衡车项目
 *
 * @details 控制结构：
 *            速度环（pid_velocity）：编码器测速 → 输出目标倾角 theta_ref
 *            角度环（pid_theta）：    MPU6050 倾角 → 输出目标角速度
 *            角速度环（pid_theta_dot）：陀螺仪角速度 → 输出角加速度
 *            转向环（pid_turn）：    Z 轴陀螺仪 → 左右轮差速
 *
 *          采样周期 5ms，运行在 FreeRTOS 优先级 3 任务中。
 *          使用定点数学库 qatan/qsin/qcos 减少 FPU 开销。
 *
 ******************************************************************************
 */

#include "app_control.h"
#include "app_control_desc.h"
#include "pid.h"
#include "app_mpu6050.h"
#include "app_motor.h"
#include "delay.h"
#include "qmath.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stddef.h>

#define IF_DEBUG        1
#define UPDATE_TIME_MS  5       /* 控制环采样周期 (ms) */

static const float g = 9.8;        /* 重力加速度 m/s^2 */
static const float lp = 0.062;     /* 轮胎中心到重心距离 m */
static const float rw = 0.032;     /* 轮胎半径 m */
static float omega_ref = 0.0f;     /* 目标角速度（速度环积分输出） */

#if IF_DEBUG
float pitch_debug = 0.0f;
float gx = 0.0f;
float omega_L_debug = 0, omega_R_debug = 0;
#endif

static uint64_t last_us = 0;

/**
 * @brief  执行一次串级 PID 计算并输出左右轮目标速度
 * @param  control_x  控制句柄
 * @retval None
 */
static void app_control_proc(app_control_handler control_x)
{
	if (control_x == NULL || control_x->pid_theta == NULL || control_x->pid_theta_dot == NULL
		|| control_x->motor == NULL || control_x->mpu == NULL || control_x->pid_velocity == NULL)
	{
		return;
	}

	uint64_t cur_us = GetUs();
	float delaT = (cur_us - last_us) * 1e-6f;

	/* 从遥控获取目标速度和转向 */
	float vel = 0.0f, turn = 0.0f;
	extern void app_rc_target_get(float *vel, float *turn);
	app_rc_target_get(&vel, &turn);
	pid_setSp(control_x->pid_velocity, vel);
	pid_setSp(control_x->pid_turn, turn);

	/* 读取编码器实际速度 */
	float res[2] = {0};
	uint8_t ret_val = app_motor_getomega(control_x->motor, MOTOR_BOTH, res, sizeof(res) / sizeof(float));
	if (ret_val < sizeof(res) / sizeof(float)) return;

	float sum = 0;
#if IF_DEBUG
	omega_L_debug = res[0];
	omega_R_debug = res[1];
#endif
	for (uint8_t i = 0; i < sizeof(res) / sizeof(float); ++i) sum += res[i];
	float omega = 0.5f * sum;

	/* 读取 MPU6050 倾角和角速度（角度制→弧度制） */
	float theta = App_MPU6050_GetPitch(control_x->mpu) * 0.0174533f;
#if IF_DEBUG
	pitch_debug = theta;
#endif
	float theta_dot = App_MPU6050_GetGx(control_x->mpu) * 0.0174533f;
#if IF_DEBUG
	gx = theta_dot;
#endif

	/* 轮式机器人运动学：线速度 x_dot = rw*omega + theta_dot*(lp+rw) */
	float x_dot = rw * (omega + theta_dot * (lp + rw) / rw);

	/* 速度环：输出目标倾角（前倾=前进） */
	float theta_ref = qatan(pid_compute(control_x->pid_velocity, x_dot) / g);
	pid_setSp(control_x->pid_theta, theta_ref);

	/* 角度环：输出目标角速度 */
	float pid_theta_dot = pid_compute(control_x->pid_theta, theta);
	pid_setSp(control_x->pid_theta_dot, pid_theta_dot);

	/* 角速度环：输出目标角加速度 */
	float pid_theta_dot_dot = pid_compute(control_x->pid_theta_dot, theta_dot);

	/* 运动学反推：x_dot_dot = (g*sin(theta) - u*lp) / cos(theta) */
	float x_dot_dot = (g * qsin(theta) - pid_theta_dot_dot * lp) / qcos(theta);

	if (last_us)
	{
		omega_ref += 1.0f / rw * delaT * x_dot_dot;
	}

	/* 转向环：Z 轴角速度 → 左右轮差速 */
	float gz = App_MPU6050_GetGz(control_x->mpu) * 0.0174533f;
	float omega_diff = pid_compute(control_x->pid_turn, gz);

	app_motor_set_omega(control_x->motor, MOTOR_LEFT, omega_ref + omega_diff);
	app_motor_set_omega(control_x->motor, MOTOR_RIGHT, omega_ref - omega_diff);

	last_us = cur_us;
}

/**
 * @brief  复位控制环所有 PID 状态和积分量
 * @param  control_x  控制句柄
 * @retval None
 */
void app_control_reset(app_control_handler control_x)
{
	if (control_x == NULL) return;
	last_us = 0;
	omega_ref = 0;
	pid_reset(control_x->pid_theta);
	pid_reset(control_x->pid_theta_dot);
	pid_reset(control_x->pid_velocity);
}

/**
 * @brief  平衡控制任务：周期驱动 MPU6050 采样和 PID 计算
 * @param  p  任务参数（app_control_handler）
 * @retval None
 */
static void app_control_balance_task(void *p)
{
	app_control_handler control_x = (app_control_handler)p;
	TickType_t last_tick = xTaskGetTickCount();
	while (true)
	{
		mpu6050_proc(control_x->mpu, UPDATE_TIME_MS);
		app_control_proc(control_x);
		vTaskDelayUntil(&last_tick, pdMS_TO_TICKS(UPDATE_TIME_MS));
	}
}

/**
 * @brief  创建平衡控制 FreeRTOS 任务
 * @param  control_x  控制句柄
 * @retval None
 */
void app_control_init(app_control_handler control_x)
{
	if (control_x == NULL) return;
	xTaskCreate(app_control_balance_task, "app_control_balance_task",
		configMINIMAL_STACK_SIZE * 2, control_x, tskIDLE_PRIORITY + 3, NULL);
}
