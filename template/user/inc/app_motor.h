#ifndef __APP_MOTOR_H_
#define __APP_MOTOR_H_
#include <stdbool.h>
#include <stdint.h>
struct motor_Typedef;
typedef struct motor_Typedef* motor_handler;
typedef enum{
	MOTOR_LEFT,
	MOTOR_RIGHT,
	MOTOR_BOTH,
}motor_forward_t;
void app_motor_init(motor_handler motor_x);
void app_motor_cmd(motor_handler motor_x,bool on);
void app_motor_set_omega(motor_handler motor_x,motor_forward_t forward,float omega);
void app_motor_proc(motor_handler motor_x);
uint8_t app_motor_getomega(motor_handler motor_x,motor_forward_t forward,float*res,uint8_t size);
#endif
