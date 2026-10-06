#ifndef __HARDWARE_I2C_H_
#define __HARDWARE_I2C_H_

#include "stm32f10x.h"
#include <stdint.h>
/* I2C 基本配置 */
struct i2c_base_struct {
    I2C_TypeDef *__I2C_x;                   // I2C1 或 I2C2
    uint32_t     __I2C_ClockSpeed;          // 如 100000 (100K) 或 400000 (400K)
    uint16_t     __I2C_DutyCycle;           // I2C_DutyCycle_2 / 16_9
    uint16_t     __I2C_Mode;                // I2C_Mode_I2C
    uint16_t     __I2C_OwnAddress1;         // 本机地址(寄存器值,主机填 (0x0A<<1))
    uint16_t     __I2C_Ack;                 // I2C_Ack_Enable / Disable
    uint16_t     __I2C_AcknowledgedAddress; // I2C_AcknowledgedAddress_7bit
};

/* SCL/SDA 引脚 */
struct i2c_gpio_struct {
    GPIO_TypeDef *__GPIOx_SCL;
    uint16_t      __GPIO_Pin_SCL;
    GPIO_TypeDef *__GPIOx_SDA;
    uint16_t      __GPIO_Pin_SDA;
    GPIOSpeed_TypeDef __GPIO_Speed_Scl;
    GPIOMode_TypeDef  __GPIO_Mode_Scl;
    GPIOSpeed_TypeDef __GPIO_Speed_Sda;
    GPIOMode_TypeDef  __GPIO_Mode_Sda;
};

typedef struct i2c_base_struct* i2c_base_hadler;
typedef struct i2c_gpio_struct* i2c_gpio_handler;

struct hard_i2c_Typedef{
	i2c_base_hadler __i2c_base;
	i2c_gpio_handler __i2c_gpio;
	uint8_t __remap;
};

#endif
