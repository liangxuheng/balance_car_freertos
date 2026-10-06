/**
 ******************************************************************************
 * @file    work_queue.c
 * @brief   工作队列：把慢操作从 ISR/高优先级任务摘出来，在低优先级任务执行
 * @author  平衡车项目
 *
 * @details 典型用法：电池采样、LED 闪烁等不需要实时性的操作，
 *          通过 work_queue_push 投递到队列，由优先级 1 的任务执行。
 *
 ******************************************************************************
 */

#include "work_queue.h"
#include "work_queue_desc.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include <stdbool.h>

static QueueHandle_t queue_handler = NULL;

/**
 * @brief  工作队列任务：阻塞等待队列消息，取出后执行回调
 * @param  p  任务参数（未使用）
 * @retval None
 */
static void work_queue_task(void *p)
{
	while (true)
	{
		struct work_item work_temp = {0};
		xQueueReceive(queue_handler, &work_temp, portMAX_DELAY);
		work_temp.work(work_temp.args);
	}
}

/**
 * @brief  创建工作队列和执行任务
 * @retval None
 */
void work_queue_init(void)
{
	queue_handler = xQueueCreate(32, sizeof(struct work_item));
	configASSERT(queue_handler);
	xTaskCreate(work_queue_task, "work_queue_task", configMINIMAL_STACK_SIZE * 2, NULL
		, tskIDLE_PRIORITY + 1, NULL);
}

/**
 * @brief  投递一个工作项到队列
 * @param  callback  回调函数
 * @param  args      回调参数
 * @retval None
 */
void work_queue_push(work_queue_callback_t callback, void *args)
{
	if (callback == NULL || args == NULL) return;
	struct work_item work_temp = {0};
	work_temp.work = callback;
	work_temp.args = args;
	xQueueSend(queue_handler, &work_temp, 0);
}
