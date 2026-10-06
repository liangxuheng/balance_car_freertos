#ifndef __PWM_DESC_H_
#define __PWM_DESC_H_
#include "stm32f10x.h"
#include "timer_desc.h"
#include <stdint.h>
struct GPIO_IN_struct;
typedef struct GPIO_IN_struct* gpio_in_handler;
struct GPIO_PWM_struct;
typedef struct GPIO_PWM_struct* gpio_pwm_handler;
struct stby_struct;
typedef struct stby_struct* stby_handler;
struct stby_struct{
	GPIO_TypeDef *__GPIOx;
	uint16_t __GPIO_Pin;
	GPIOSpeed_TypeDef __GPIO_Speed;
	GPIOMode_TypeDef __GPIO_Mode;
};
struct GPIO_IN_struct{
	GPIO_TypeDef *__GPIOx_1;
	uint16_t __GPIO_Pin_1;
	GPIOSpeed_TypeDef __GPIO_Speed_1;
	GPIOMode_TypeDef __GPIO_Mode_1;
	GPIO_TypeDef *__GPIOx_2;
	uint16_t __GPIO_Pin_2;
	GPIOSpeed_TypeDef __GPIO_Speed_2;
	GPIOMode_TypeDef __GPIO_Mode_2;
};
struct GPIO_PWM_struct{
	GPIO_TypeDef *__GPIOx;
	uint16_t __GPIO_Pin;
	GPIOSpeed_TypeDef __GPIO_Speed;
	GPIOMode_TypeDef __GPIO_Mode;
};
struct pwm_struct{
	struct stby_struct* stby_handler_pwm;
	timer_handler timer;
	struct GPIO_IN_struct* GPIO_IN;
	struct GPIO_PWM_struct*GPIO_PWM;
};
#endif
