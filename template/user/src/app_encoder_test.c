/**
 ******************************************************************************
 * @file    app_encoder_test.c
 * @brief   编码器读数测试：串口打印左右轮转速
 ******************************************************************************
 */

#include "app_encoder_test.h"
#include "my_usart.h"
#include <stdio.h>
#include "delay.h"

static volatile float last_pos_l=0;
static volatile float last_pos_r=0;
__IO float speed_l=0;
__IO float speed_r=0;

void app_encoder_test_pro(encoder_handler encoder_x_1,encoder_handler encoder_x_2)
{
	if(!encoder_x_1||!encoder_x_2)
	{
		return;
	}
	static uint64_t last_tick=0;
	uint64_t tick_now=GetTick();
//	if(tick_now-last_tick>50){
//		float left=encoder_get_pos(encoder_x_1);
//		float right=encoder_get_pos(encoder_x_2);
//		printf("%f %f\n",left,right);
//		last_tick=tick_now;
//	}
//	//M测速法
//	if(tick_now-last_pos_l>=1)
//	{
//		float cur_pos_l=encoder_get_pos(encoder_x_1);
//		float cur_pos_r=encoder_get_pos(encoder_x_2);
//		printf("%f %f\n",(cur_pos_l-last_pos_l)/0.001f
//		,(cur_pos_r-last_pos_r)/0.001f);
//		last_tick=tick_now;
//		last_pos_l=cur_pos_l;
//		last_pos_r=cur_pos_r;
//	}
	
	//T测速法
	if(tick_now-last_tick>=1)
	{
		speed_l=encoder_get_speed(encoder_x_1);
		speed_r=encoder_get_speed(encoder_x_2);
//		printf("%f,%f\n",speed_l
//		,speed_r);
		last_tick=tick_now;
	}
}
