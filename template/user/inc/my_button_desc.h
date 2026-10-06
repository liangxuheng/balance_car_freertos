#ifndef __MY_BUTTON_H_
#define __MY_BUTTON_H_

#include "stm32f10x.h"
#include <stdint.h>

struct my_button_struct
{
		/* 初始化参数 */
	GPIO_TypeDef *__GPIOx;
	uint16_t __GPIO_Pin;
	GPIOSpeed_TypeDef __GPIO_Speed;
	GPIOMode_TypeDef __GPIO_Mode;
	void (*__button_pressed_cb)(void*);
	void (*__button_released_cb)(void*);
	void (*__button_clicked_cb)(void*,uint8_t clicks);
	void (*__button_long_pressed_cb)(void*,uint8_t ticks);
	uint8_t (*__button_usr_read)(void*);//读取引脚电平函数
	uint8_t __press_level;//按键按下时的电平
	uint32_t __LongPressThreshold;
	uint32_t __LongPressTickInterval;
	uint32_t __ClickInterval; 
	
	uint8_t  __LastState;     // 按钮上次的状态，0 - 松开，1 - 按下
	uint8_t  __ChangePending; // 按钮的状态是否正在发生改变
	uint64_t __PendingTime;   // 按钮状态开始变化的时间
	
	uint64_t __LastPressedTime;  // 按钮上次按下的时间
	uint64_t __LastReleasedTime; // 按钮上次松开的时间
	
	uint8_t __LongPressTicks;
	uint64_t __LastLongPressTickTime; 
	
	uint8_t __ClickCnt;
};

#endif
