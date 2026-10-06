#ifndef __app_control_DESC_H_
#define __app_control_DESC_H_
struct pid_Typedef;
typedef struct pid_Typedef*pid_handler;
struct mpu6050_struct;
typedef struct mpu6050_struct* mpu6050_handler;
struct motor_Typedef;
typedef struct motor_Typedef* motor_handler;
struct app_control_TypeDef
{
	pid_handler pid_theta;
	pid_handler pid_theta_dot;
	pid_handler pid_velocity;
	pid_handler pid_turn;
	mpu6050_handler mpu;
	motor_handler motor;
};
#endif
