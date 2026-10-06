/**
  ******************************************************************************
  * @file    delay.c
  * @author  铁头山羊
  * @version V 1.0.0
  * @date    2022年8月30日
  * @brief   延迟函数源头文件
  ******************************************************************************
  */

#ifndef _DELAY_H_
#define _DELAY_H_

#include "stm32f10x.h"

struct timer_struct;
typedef struct timer_struct* timer_handler;
void Delay_Init(timer_handler timer_x); // 延迟函数初始化
uint64_t GetTick(void); // 获取系统的当前时间
uint64_t GetUs(void); // 获取当前的微秒级时间
void DelayUs(uint64_t us); // 微秒级延迟

#endif
