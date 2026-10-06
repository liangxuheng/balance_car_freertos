#ifndef __MOTOR_DO_H_
#define __MOTOR_DO_H_
#include <stdbool.h>
struct motor_Typedef;
typedef struct motor_Typedef* motor_handler;
struct motor_do_TypeDef;
typedef struct motor_do_TypeDef*motor_do_handler;
void motor_do_init(motor_handler motor_x);
void motor_do_set_do_cmd(motor_handler motor_x,bool on);
void motor_do_set_do_set(motor_handler motor_x,float value);
#endif
