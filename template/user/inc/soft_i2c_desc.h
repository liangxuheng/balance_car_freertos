#ifndef __SOFT_I2C_DESC_H_
#define __SOFT_I2C_DESC_H_
#include "stm32f10x.h"
struct soft_i2c{
		GPIO_TypeDef *Port_scl;
		uint16_t Pin_scl;
		GPIO_TypeDef *Port_sda;
		uint16_t Pin_sda;
};
#endif
