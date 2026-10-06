#ifndef __BAT_LED_DESC_H_
#define __BAT_LED_DESC_H_

#include "stm32f10x.h"
#include <stdint.h>

struct bat_led_struct{
	GPIO_TypeDef*__GPIO_X;   /* 新增：LED 所在端口 */
	uint16_t __GPIO_Pin;
	GPIOSpeed_TypeDef __GPIO_Speed;
	GPIOMode_TypeDef __GPIO_Mode;
};

struct bat_led_init_struct{
	struct bat_led_struct* __led_top;
	struct bat_led_struct* __led_mid;
	struct bat_led_struct* __led_low;
};


#endif
