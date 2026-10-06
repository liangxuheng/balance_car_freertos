#ifndef __APP_MOTOR_DESC_H_
#define __APP_MOTOR_DESC_H_
#include <stdbool.h>
struct pid_Typedef;
typedef struct pid_Typedef*pid_handler;
struct encoder_struct;
typedef struct encoder_struct* encoder_handler;
struct pwm_struct;
typedef struct pwm_struct* pwm_handler;
struct adc_struct;
typedef struct adc_struct* adc_handler;
struct motor_side{
	pid_handler pid;
	encoder_handler encoder;
	pwm_handler pwm;
};
typedef struct motor_side* motor_handler_side;
struct motor_Typedef{
	motor_handler_side motor_left;
	motor_handler_side motor_right;
	adc_handler adc_vbat;
};
#endif
