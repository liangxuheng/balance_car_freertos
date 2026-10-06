/**
 ******************************************************************************
 * @file    app_bat_test.c
 * @brief   电池采样测试代码（已注释，保留备用）
 ******************************************************************************
 */

//#include "app_vbat_test.h"
//#include <stddef.h>
//#include <stdbool.h>
//#include "delay.h"

//void app_vbat_test(timer_handler timer_x,adc_handler adc_x
//	,usart_handler usart_x)
//{
//	if(timer_x==NULL||adc_x==NULL||usart_x==NULL)
//	{
//		return;
//	}
//	app_vbat_init(timer_x,adc_x);
//	usart_init(usart_x);
//	while(true)
//	{
//		float vbat=app_vbat_get(adc_x);
//		My_USART_Printf(usart_x,"%.3f\r\n",vbat);
//		Delay(10);
//	}
//}
