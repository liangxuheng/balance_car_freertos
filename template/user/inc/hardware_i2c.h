#ifndef __HARDWARE_I2C_DESC_H_
#define __HARDWARE_I2C_DESC_H_
#include <stdint.h>
struct hard_i2c_Typedef;
typedef struct hard_i2c_Typedef* hard_i2c_handler;

void hardware_i2c_init(hard_i2c_handler hard_i2c_x);
int  hardware_i2c_sendBytes(hard_i2c_handler,uint8_t,const uint8_t*,uint16_t,uint64_t);
int  hardware_i2c_receiveBytes(hard_i2c_handler,uint8_t,uint8_t*,uint16_t,uint64_t);
int  hardware_i2c_regReadBytes(hard_i2c_handler,uint8_t,uint8_t,uint8_t*,uint16_t,uint64_t);
int  hardware_i2c_memWriteBytes(hard_i2c_handler,uint8_t,uint8_t,const uint8_t*,uint16_t,uint64_t);
void hardware_i2c_bus_reset(hard_i2c_handler);
#endif
