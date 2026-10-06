#ifndef __WORK_QUEUE_H_
#define __WORK_QUEUE_H_
typedef void(*work_queue_callback_t)(void*args);
void work_queue_init(void);
void work_queue_push(work_queue_callback_t callback,void*args);
#endif
