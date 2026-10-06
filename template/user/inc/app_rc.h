#ifndef __APP_RC_H_
#define __APP_RC_H_

struct app_rc_TypeDef;
typedef struct app_rc_TypeDef* app_rc_handler;
void app_rc_init(app_rc_handler app_rc_x);
void app_rc_proc(app_rc_handler app_rc_x);
void app_rc_target_get(float *vel,float *turn);
#endif
