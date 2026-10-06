#ifndef __PID_H_
#define __PID_H_

struct pid_Typedef;
typedef struct pid_Typedef*pid_handler;

void pid_set_up_and_down_limit(pid_handler pid_x,float up,float lower);
void pid_setSp(pid_handler pid_x,float sp);
void pid_reset(pid_handler pid_x);
float pid_compute(pid_handler pid_x,float fb);
float pid_getSp(pid_handler pid_x);

#endif
