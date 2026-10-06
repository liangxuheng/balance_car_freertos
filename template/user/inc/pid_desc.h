#ifndef __PID_DESC_H_
#define __PID_DESC_H_
#include <stdint.h>
typedef enum{
	PID_LEFT,
	PID_RIGHT,
	PID_BOTH,
}pid_t;
struct pid_Typedef{
	float Kp;//比例系数
	float Ki;//积分系数
	float Kd;//微分系数
	float Sp;//用户设定的值
	uint64_t last_tick;//上一个时刻
	float err_last;//上一次运行的误差值
	float err_last_int;//上一次运行的积分值
	float uLimit;//上限
	float lLimit;//下限
	pid_t forward;//方向
};
#endif
