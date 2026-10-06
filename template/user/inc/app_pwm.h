#ifndef __APP_PWM_H_
#define __APP_PWM_H_
#include <stdint.h>
struct pwm_struct;
typedef struct pwm_struct* pwm_handler;
typedef enum{
	LEFT,
	RIGHT,
}forward_t;
void app_pwm_init(pwm_handler pwm);
void app_pwm_stby_ctl(pwm_handler pwm,uint8_t state);
void app_pwm_motor_set(pwm_handler pwm,forward_t forward,float duty);
#endif
