#ifndef __app_control_H_
#define __app_control_H_
struct app_control_TypeDef;
typedef struct app_control_TypeDef* app_control_handler;
//void app_control_proc(app_control_handler contorl_x);
void app_control_reset(app_control_handler contorl_x);
void app_control_init(app_control_handler contorl_x);
#endif
