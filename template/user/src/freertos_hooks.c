/**
 ******************************************************************************
 * @file    freertos_hooks.c
 * @brief   FreeRTOS 钩子函数：断言失败/栈溢出/内存分配失败处理
 ******************************************************************************
 */

#include "FreeRTOS.h"
#include "task.h"

/**
 * @brief  FreeRTOS assert 钩子：断言失败时死循环
 * @param  file  文件名
 * @param  line  行号
 * @retval None
 */
void vAssertCalled(const char *file, int line)
{
	(void)file;
	(void)line;
	while (1) {}
}

/**
 * @brief  栈溢出钩子：任务栈溢出时触发断言
 * @param  t  任务句柄
 * @param  n  任务名
 * @retval None
 */
void vApplicationStackOverflowHook(TaskHandle_t t, char *n)
{
	(void)t;
	(void)n;
	configASSERT(0);
}

/**
 * @brief  内存分配失败钩子：pvPortMalloc 失败时触发断言
 * @retval None
 */
void vApplicationMallocFailedHook(void)
{
	configASSERT(0);
}
