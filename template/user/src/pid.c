/**
 ******************************************************************************
 * @file    pid.c
 * @brief   PID 控制器实现（位置式，带积分限幅与输出限幅）
 * @author  平衡车项目
 *
 * @details 离散化 PID 公式：
 *            u(k) = Kp * e(k) + Ki * ∫e dt + Kd * de/dt
 *          积分项用梯形法近似，首次调用跳过积分/微分避免跳变。
 *          输出和积分项均做上下限限幅，防止积分饱和。
 *
 ******************************************************************************
 */

#include "pid.h"
#include "pid_desc.h"
#include "delay.h"
#include <stddef.h>

/**
 * @brief  设置 PID 输出上下限
 * @param  pid_x  PID 句柄
 * @param  up     输出上限
 * @param  lower  输出下限
 */
void pid_set_up_and_down_limit(pid_handler pid_x, float up, float lower)
{
	if (pid_x == NULL) return;
	pid_x->uLimit = up;
	pid_x->lLimit = lower;
}

/**
 * @brief  设置目标值（Setpoint）
 * @param  pid_x  PID 句柄
 * @param  sp     目标值
 */
void pid_setSp(pid_handler pid_x, float sp)
{
	if (pid_x == NULL) return;
	pid_x->Sp = sp;
}

/**
 * @brief  获取当前目标值
 * @param  pid_x  PID 句柄
 * @retval 当前目标值
 */
float pid_getSp(pid_handler pid_x)
{
	if (pid_x == NULL) return 0.0f;
	return pid_x->Sp;
}

/**
 * @brief  复位 PID 内部状态（积分累积、上次误差、时间戳）
 * @param  pid_x  PID 句柄
 */
void pid_reset(pid_handler pid_x)
{
	if (pid_x == NULL) return;
	pid_x->last_tick = 0;
	pid_x->err_last = 0;
	pid_x->err_last_int = 0;
}

/**
 * @brief  计算一次 PID 输出
 * @param  pid_x  PID 句柄
 * @param  fb     当前反馈值（Feedback）
 * @retval PID 控制量（已限幅）
 *
 * @note   首次调用时 last_tick=0，跳过积分和微分，仅输出比例项。
 *         积分项做抗饱和限幅，防止长时间偏差导致积分溢出。
 */
float pid_compute(pid_handler pid_x, float fb)
{
	if (pid_x == NULL) return 0.0f;

	uint64_t cur_us = GetUs();
	float err = pid_x->Sp - fb;
	float err_cur_int = 0.0f;
	float err_cur_dev = 0.0f;

	/* 首次调用忽略积分项和微分项 */
	if (pid_x->last_tick)
	{
		err_cur_dev = (err - pid_x->err_last) / ((cur_us - pid_x->last_tick) * 1.0e-6f);
		err_cur_int = pid_x->err_last_int + (err + pid_x->err_last) * (cur_us - pid_x->last_tick) * 1.0e-6f * 0.5f;
	}

	float co = pid_x->Kp * err + pid_x->Ki * err_cur_int + pid_x->Kd * err_cur_dev;

	pid_x->last_tick = cur_us;
	pid_x->err_last = err;
	pid_x->err_last_int = err_cur_int;

	/* 输出限幅 */
	if (co > pid_x->uLimit)      co = pid_x->uLimit;
	else if (co < pid_x->lLimit) co = pid_x->lLimit;

	/* 积分项抗饱和限幅 */
	if (pid_x->err_last_int > pid_x->uLimit)      pid_x->err_last_int = pid_x->uLimit;
	else if (pid_x->err_last_int < pid_x->lLimit) pid_x->err_last_int = pid_x->lLimit;

	return co;
}
