/**
 ******************************************************************************
 * @file    app_mpu_test.c
 * @brief   MPU6050 原始数据测试：串口打印加速度/角速度/欧拉角
 ******************************************************************************
 */

#include "app_mpu_test.h"
#include "app_mpu6050.h"
#include "delay.h"

// float ax, ay, az; // 加速度计的结果，单位g
 float temperature; // 温度计的结果，单位摄氏度
// float gx, gy, gz; // 单位°/s
 static uint64_t last_tick=0;
 float yaw=0, pitch=0, roll=0; // 欧拉角，单位°

void app_mpu_test_pro(mpu6050_handler mpu_x)
{
	if(!mpu_x)
	{
		return;
	}
	uint64_t cur=GetTick();
	if(cur-last_tick>=10)
	{
		last_tick=cur;
		
//		 ax = App_MPU6050_GetAx(mpu_x);
//		 ay = App_MPU6050_GetAy(mpu_x);
//		 az = App_MPU6050_GetAz(mpu_x);
		
		 temperature = App_MPU6050_GetTemperature(mpu_x);
		
//		 gx = App_MPU6050_GetGx(mpu_x);
//		 gy = App_MPU6050_GetGy(mpu_x);
//		 gz = App_MPU6050_GetGz(mpu_x);
			yaw=App_MPU6050_GetYaw(mpu_x);
			pitch=App_MPU6050_GetPitch(mpu_x);
			roll=App_MPU6050_GetRoll(mpu_x);
		//My_USART_Printf(USART2, "%f,%f,%f,%f,%f,%f,%f\n", ax, ay, az, temperature, gx, gy, gz);
	}
}
