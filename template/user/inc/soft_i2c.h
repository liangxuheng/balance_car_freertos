#ifndef __SOFT_I2C_H_
#define __SOFT_I2C_H_
#include <stdint.h>
struct soft_i2c;
typedef struct soft_i2c* soft_i2c_handler;
void soft_i2c_init(soft_i2c_handler i2cx);
void soft_i2c_general_start(soft_i2c_handler i2cx);
void soft_i2c_general_stop(soft_i2c_handler i2cx);
void soft_i2c_send_byte(soft_i2c_handler i2cx,uint8_t date_byte);
int8_t soft_i2c_recv_ack(soft_i2c_handler i2cx,int64_t timerout);
int8_t soft_i2c_read_byte(soft_i2c_handler i2cx);
void soft_i2c_send_ack(soft_i2c_handler i2cx,uint8_t ack);
int32_t soft_i2c_regsend_bytes(soft_i2c_handler i2cx,uint8_t addr,uint8_t reg,const uint8_t*pdata
	,uint16_t size,uint64_t timerout);
int32_t soft_i2c_regread_bytes(soft_i2c_handler i2cx,uint8_t addr,uint8_t reg
,uint8_t*pdata,uint16_t size,uint64_t timerout);
#endif
