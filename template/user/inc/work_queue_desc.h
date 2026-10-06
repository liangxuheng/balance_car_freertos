#ifndef __WORK_QUEUE_DESC_H_
#define __WORK_QUEUE_DESC_H_
typedef void(*work_queue_callback_t)(void*args);
struct work_item
{
	work_queue_callback_t work;
	void*args;
};
typedef struct work_item*work_item_handler;
#endif
