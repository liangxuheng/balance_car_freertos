#ifndef __TIMER_H_
#define __TIMER_H_
struct timer_struct;
typedef struct timer_struct* timer_handler;
void timer_init(timer_handler timer_x);
#endif
