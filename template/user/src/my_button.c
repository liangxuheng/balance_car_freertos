/**
 ******************************************************************************
 * @file    my_button.c
 * @brief   按键消抖：状态机检测短按/长按，周期扫描
 ******************************************************************************
 */

#include "my_button.h"
#include "my_button_desc.h"
#include "delay.h"
#include "FreeRTOS.h"
#include "task.h"

//默认时间间隔
#define BUTTON_SETTLING_TIME             10   // 按钮消抖延迟
#define BUTTON_CLICK_INTERVAL            200  // 按钮多击时每次点击的时间最大时间间隔
#define BUTTON_LONG_PRESS_THRESHOLD      1000 // 按钮长按最小时间
#define BUTTON_LONG_PRESS_TICK_INTERNVAL 100  // 长按后持续触发的时间间隔


static void my_OnButtonEveryPolled(my_button_handler button_x
	, uint64_t currentTime);

/**
 * @brief  初始化按键：GPIO + 默认参数 + 状态清零
 * @param  button_x           按键句柄
 * @param  __button_usr_read  按键电平读取回调
 * @retval None
 */
void my_button_init(my_button_handler button_x
	,uint8_t (*__button_usr_read)(void*))
{
	if(!button_x||!__button_usr_read)
	{
		return;
	}
	GPIO_InitTypeDef GPIO_InStructer;
	GPIO_StructInit(&GPIO_InStructer);
	GPIO_InStructer.GPIO_Mode=button_x->__GPIO_Mode;
	GPIO_InStructer.GPIO_Pin=button_x->__GPIO_Pin;
	GPIO_InStructer.GPIO_Speed=button_x->__GPIO_Speed;
	GPIO_Init(button_x->__GPIOx,&GPIO_InStructer);
	
	my_button_set_usrread(button_x,__button_usr_read);
	
	if(button_x->__ClickInterval==0)
	{
		button_x->__ClickInterval=BUTTON_CLICK_INTERVAL;
	}
	if(button_x->__LongPressTickInterval==0)
	{
		button_x->__LongPressTickInterval=BUTTON_LONG_PRESS_TICK_INTERNVAL;
	}
	if(button_x->__LongPressThreshold==0)
	{
		button_x->__LongPressThreshold=BUTTON_LONG_PRESS_THRESHOLD;
	}
	
	button_x->__LastState = 0; // 初始状态下假设按钮是松开的
	button_x->__ChangePending = 0; 
	button_x->__PendingTime = 0;
	button_x->__LastPressedTime = 0;
	button_x->__LastReleasedTime = 0;
	button_x->__LongPressTicks = 0;
	button_x->__ClickCnt = 0;
	
}

/**
 * @brief  获取按键当前状态
 * @param  button_x  按键句柄
 * @retval 0=松开, 1=按下, -1=错误
 */
int8_t my_button_get_state(my_button_handler button_x)
{
	if(!button_x)
	{
		return -1;
	}
	return button_x->__LastState;
}

/**
 * @brief  设置多击时间间隔(ms)
 * @param  button_x       按键句柄
 * @param  click_interval 两次点击最大间隔
 * @retval None
 */
void my_button_ClickIntervalConfig(my_button_handler button_x
	,uint32_t click_interval)
{
	if(!button_x)
	{
		return;
	}
	button_x->__ClickInterval=click_interval;
}

/**
 * @brief  设置长按持续触发间隔(ms)
 * @param  button_x           按键句柄
 * @param  long_press_interval 长按后每 N ms 触发一次回调
 * @retval None
 */
void my_button_LongPressIntervalConfig(my_button_handler button_x
	,uint32_t long_press_interval )
{
	if(!button_x)
	{
		return;
	}
	button_x->__LongPressTickInterval=long_press_interval;
}

/**
 * @brief  设置长按阈值(ms)
 * @param  button_x            按键句柄
 * @param  long_press_threshold 按下超过此时长算长按
 * @retval None
 */
void my_button_LongPressThresholdConfig(my_button_handler button_x
	,uint32_t long_press_threshold)
{
	if(!button_x)
	{
		return;
	}
	button_x->__LongPressThreshold=long_press_threshold;
}

/**
 * @brief  注册长按回调
 * @param  button_x   按键句柄
 * @param  LongPressCb 回调函数(按键句柄, 长按计数)
 * @retval None
 */
void my_button_set_longpresscb(my_button_handler button_x
	,void (*LongPressCb)(void*,uint8_t ticks))
{
		if(!button_x||!LongPressCb)
		{
			return;
		}
		button_x->__button_long_pressed_cb=LongPressCb;
}

/**
 * @brief  注册按下回调
 * @param  button_x  按键句柄
 * @param  PressCb   回调函数
 * @retval None
 */
void my_button_set_presscb(my_button_handler button_x,void (*PressCb)(void*))
{
	if(!button_x||!PressCb)
	{
		return;
	}
	button_x->__button_pressed_cb=PressCb;
}

/**
 * @brief  注册松开回调
 * @param  button_x   按键句柄
 * @param  ReleaseCb  回调函数
 * @retval None
 */
void my_button_set_releasecb(my_button_handler button_x,void (*ReleaseCb)(void*))
{
	if(!button_x||!ReleaseCb)
	{
		return;
	}
	button_x->__button_released_cb=ReleaseCb;
}

/**
 * @brief  注册点击回调
 * @param  button_x  按键句柄
 * @param  ClickCb   回调函数(按键句柄, 点击次数)
 * @retval None
 */
void my_button_set_clickcb(my_button_handler button_x
	,void (*ClickCb)(void*,uint8_t clicks))
{
	if(!button_x||!ClickCb)
	{
		return;
	}
	button_x->__button_clicked_cb=ClickCb;
}

/**
 * @brief  设置按键电平读取函数
 * @param  button_x       按键句柄
 * @param  button_usr_read  读取回调
 * @retval None
 */
void my_button_set_usrread(my_button_handler button_x
	,uint8_t (*button_usr_read)(void*))
{
	if(!button_x||!button_usr_read)
	{
		return;
	}
	button_x->__button_usr_read=button_usr_read;
}

/**
 * @brief  按键状态机处理：消抖 + 短按/长按/连击判断
 * @param  button_x  按键句柄
 * @retval None
 */
void my_button_proc(my_button_handler button_x)
{
	if(!button_x)
	{
		return;
	}
	TickType_t cur_tick=xTaskGetTickCount();
	if(button_x->__ChangePending)
	{
			if(!button_x->__button_usr_read)
			{
				return;
			}
		//状态正在改变
		if(cur_tick>BUTTON_SETTLING_TIME+button_x->__PendingTime)
		{
			//过了消抖的时间
			uint8_t cur_state=button_x->__button_usr_read((void*)button_x);
			if(cur_state!=button_x->__LastState)
			{
				if(cur_state==button_x->__press_level){
					button_x->__LastPressedTime=cur_tick;
					if(button_x->__button_pressed_cb)
					{
						button_x->__button_pressed_cb((void*)button_x);
					}
				}
				else 
				{
					button_x->__LastReleasedTime=GetTick();
					if(button_x->__button_released_cb)
					{
						button_x->__button_released_cb(button_x);
					}
					//松开后开始计数
					button_x->__LongPressTicks=0;
					button_x->__LastReleasedTime=cur_tick;
					if(button_x->__LastReleasedTime-button_x->__LastPressedTime<button_x->__LongPressThreshold)
					{
						button_x->__ClickCnt++;
					}
					else 
					{
						button_x->__ClickCnt=0;
					}
				}
			}
			button_x->__ChangePending=0;
			button_x->__LastState=cur_state;
		}
	}
	else 
	{
		uint8_t cur_state=button_x->__button_usr_read((void*)button_x);
		if(cur_state!=button_x->__LastState)
		{
			button_x->__ChangePending=1;
			button_x->__PendingTime=cur_tick;
		}
	}
	//判断连击和长按的事件
	my_OnButtonEveryPolled(button_x,cur_tick);
}

static void my_OnButtonEveryPolled(my_button_handler button_x
	, uint64_t currentTime){
		if(!button_x)
		{
			return;
		}
		if(button_x->__LastState)
		{
			if(button_x->__LongPressTicks==0)
			{
				//第一次长按
				if(button_x->__LastPressedTime!=0
				&&currentTime-button_x->__LastPressedTime>=button_x->__LongPressThreshold)
				{
					button_x->__LastLongPressTickTime=currentTime;
					button_x->__LongPressTicks++;
					if(button_x->__button_long_pressed_cb)
					{
						button_x->__button_long_pressed_cb(button_x
						,button_x->__LongPressTicks);
					}
				}
			}
			else 
			{
				if(currentTime-button_x->__LastLongPressTickTime
					>=button_x->__LongPressTickInterval)
				{
					button_x->__LastLongPressTickTime=currentTime;
					button_x->__LongPressTicks++;
					if(button_x->__button_long_pressed_cb)
					{
						button_x->__button_long_pressed_cb(button_x
						,button_x->__LongPressTicks);
					}
				}
			}
		}
		else 
		{
			//连击事件在按键松开后才进行响应
			if(button_x->__ClickCnt>0
				&&currentTime-button_x->__LastReleasedTime>button_x->__ClickInterval)
			{
				if(button_x->__button_clicked_cb)
				{
					button_x->__button_clicked_cb(button_x,button_x->__ClickCnt);
				}
				button_x->__ClickCnt=0;
			}
		}
}
