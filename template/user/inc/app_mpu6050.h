#ifndef __APP_MPU6050_H_
#define __APP_MPU6050_H_
#include <stdint.h>
struct mpu6050_struct;
typedef struct mpu6050_struct* mpu6050_handler;
void app_mpu6050_init(mpu6050_handler mpu6050_x);
void mpu6050_proc(mpu6050_handler mpu_x,uint16_t ms);
float App_MPU6050_GetAx(mpu6050_handler mpu6050_x);
float App_MPU6050_GetAy(mpu6050_handler mpu6050_x);
float App_MPU6050_GetAz(mpu6050_handler mpu6050_x);

float App_MPU6050_GetTemperature(mpu6050_handler mpu6050_x);

float App_MPU6050_GetGx(mpu6050_handler mpu6050_x);
float App_MPU6050_GetGy(mpu6050_handler mpu6050_x);
float App_MPU6050_GetGz(mpu6050_handler mpu6050_x);
float App_MPU6050_GetYaw(mpu6050_handler mpu_x);
float App_MPU6050_GetPitch(mpu6050_handler mpu_x);
float App_MPU6050_GetRoll(mpu6050_handler mpu_x);
#endif
