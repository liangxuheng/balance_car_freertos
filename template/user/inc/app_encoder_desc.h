#ifndef __APP_ENCODE_DESC_H_
#define __APP_ENCODE_DESC_H_

#include "stm32f10x.h"
#include <stdint.h>
#include <stdbool.h>

typedef enum{
	EN_LEFT,
	EN_RIGHT,
}encoder_t;

struct encoder_gpio_struct{
	GPIO_TypeDef *__GPIOx;
	uint16_t __GPIO_Pin;
	GPIOSpeed_TypeDef __GPIO_Speed;
	GPIOMode_TypeDef __GPIO_Mode;
};

struct encoder_exti_struct{
	uint32_t __EXTI_Line;
	EXTIMode_TypeDef __EXTI_Mode;
	EXTITrigger_TypeDef __EXTI_Trigger;
	FunctionalState __EXTI_LineCmd;
};

struct encoder_nvic_struct{
	uint8_t __NVIC_IRQChannel;
};

typedef struct encoder_gpio_struct* encoder_gpio_handler;
typedef struct encoder_exti_struct* encoder_exti_handler;
typedef struct encoder_nvic_struct* encoder_nvic_handler;

struct encoder_struct{
	encoder_gpio_handler __gpio_1;
	encoder_gpio_handler __gpio_2;
	encoder_exti_handler __exti;
	bool __is_interrupt;
	encoder_nvic_handler __nvic;
	encoder_t forward;
};

#endif
