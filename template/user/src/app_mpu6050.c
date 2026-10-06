/**
 ******************************************************************************
 * @file    app_mpu6050.c
 * @brief   MPU6050 六轴 IMU 驱动：硬件 I2C 读取原始数据，互补滤波融合欧拉角
 * @author  平衡车项目
 *
 * @details 互补滤波：95.2% 陀螺仪积分 + 4.8% 加速度计校正。
 *          I2C 通信失败自动总线复位重试一次。
 *          量程：加速度 ±2g，陀螺仪 ±2000°/s。
 *
 ******************************************************************************
 */

# include "hardware_i2c.h"
#include "app_mpu6050.h"
#include "app_mpu6050_desc.h"
#include "delay.h"
#include "qmath.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stddef.h>
# define MPU_I2C_TIMEOUT   20000U

//芯片的寄存器列表
#define	MPU6050_SMPLRT_DIV		0x19
#define	MPU6050_CONFIG			0x1A
#define	MPU6050_GYRO_CONFIG		0x1B
#define	MPU6050_ACCEL_CONFIG	0x1C

#define	MPU6050_ACCEL_XOUT_H	0x3B
#define	MPU6050_ACCEL_XOUT_L	0x3C
#define	MPU6050_ACCEL_YOUT_H	0x3D
#define	MPU6050_ACCEL_YOUT_L	0x3E
#define	MPU6050_ACCEL_ZOUT_H	0x3F
#define	MPU6050_ACCEL_ZOUT_L	0x40
#define	MPU6050_TEMP_OUT_H		0x41
#define	MPU6050_TEMP_OUT_L		0x42
#define	MPU6050_GYRO_XOUT_H		0x43
#define	MPU6050_GYRO_XOUT_L		0x44
#define	MPU6050_GYRO_YOUT_H		0x45
#define	MPU6050_GYRO_YOUT_L		0x46
#define	MPU6050_GYRO_ZOUT_H		0x47
#define	MPU6050_GYRO_ZOUT_L		0x48

#define	MPU6050_PWR_MGMT_1		0x6B
#define	MPU6050_PWR_MGMT_2		0x6C
#define	MPU6050_WHO_AM_I		0x75

#define MPU6050_WRITEADDRESS 0xD0
#define MPU6050_READADDRESS 0xD1

static float ax=0, ay=0, az=0; // 加速度计的结果，单位g
static float temperature=0; // 温度计的结果，单位摄氏度
static float gx=0, gy=0, gz=0; // 单位°/s
static float yaw, pitch, roll; // 欧拉角，单位°



static void mpu_write_reg(mpu6050_handler mpu_x,uint8_t address,uint8_t data);

void app_mpu6050_init(mpu6050_handler mpu6050_x)
{
	if(mpu6050_x==NULL)
	{
		return;
	}
	hardware_i2c_init(mpu6050_x->__hard_i2c);
	mpu_write_reg(mpu6050_x,MPU6050_PWR_MGMT_1,0x80);
	vTaskDelay(pdMS_TO_TICKS(100));
	mpu_write_reg(mpu6050_x,MPU6050_PWR_MGMT_1,0x00);
	mpu_write_reg(mpu6050_x,MPU6050_GYRO_CONFIG,0x18);
	mpu_write_reg(mpu6050_x,MPU6050_ACCEL_CONFIG,0x00);
}

static void mpu_write_reg(mpu6050_handler mpu_x,uint8_t address,uint8_t data)
{
	if(mpu_x->__hard_i2c==NULL)
	{
		return ;
	}
	//失败先救总线，再重试一次；仍失败则放弃
	if(hardware_i2c_memWriteBytes(mpu_x->__hard_i2c,MPU6050_WRITEADDRESS
		,address,&data,1,MPU_I2C_TIMEOUT)!=1)
	{
		hardware_i2c_bus_reset(mpu_x->__hard_i2c);
		hardware_i2c_memWriteBytes(mpu_x->__hard_i2c,MPU6050_WRITEADDRESS
			,address,&data,1,MPU_I2C_TIMEOUT);
	}
}

static int8_t mpu_read_reg(mpu6050_handler mpu_x,uint8_t address)
{
	if(mpu_x->__hard_i2c==NULL)
	{
		return -1;
	}
	uint8_t res=0;
	if(hardware_i2c_regReadBytes(mpu_x->__hard_i2c,MPU6050_WRITEADDRESS
		,address,&res,1,MPU_I2C_TIMEOUT)!=1)
	{
		return -1;
	}
	return (int8_t)res;
}

static void app_mpu6050_update(mpu6050_handler mpu6050_x)
{	
	uint8_t buf[ 14 ]; /* 0x3B~0x48 共14字节；读失败本次不更新，直接返回 */ 
	if (hardware_i2c_regReadBytes(mpu6050_x->__hard_i2c,MPU6050_WRITEADDRESS
		,MPU6050_ACCEL_XOUT_H,buf, 14 ,MPU_I2C_TIMEOUT)!= 14 )
	{ 
			hardware_i2c_bus_reset(mpu6050_x->__hard_i2c); 
			if (hardware_i2c_regReadBytes(mpu6050_x->__hard_i2c,MPU6050_WRITEADDRESS
        ,MPU6050_ACCEL_XOUT_H,buf, 14 ,MPU_I2C_TIMEOUT)!= 14 )
    { 
			return;
    }
	}

	int16_t ax_raw = ( int16_t )((( uint16_t )buf[ 0 ]<< 8 ) | buf[ 1 ]); 
	int16_t ay_raw = ( int16_t )((( uint16_t )buf[ 2 ]<< 8 ) | buf[ 3 ]); 
	int16_t az_raw = ( int16_t )((( uint16_t )buf[ 4 ]<< 8 ) | buf[ 5 ]);

	ax = ax_raw * 6.1035e-5f;
	ay = ay_raw * 6.1035e-5f;
	az = az_raw * 6.1035e-5f; 
	int16_t temperature_raw = ( int16_t )((( uint16_t )buf[ 6 ]<< 8 ) | buf[ 7 ]);
	temperature = temperature_raw / 333.87f + 21.0f ; 
	int16_t gx_raw = ( int16_t )((( uint16_t )buf[ 8 ]<< 8 )  | buf[ 9 ]); 
	int16_t gy_raw = ( int16_t )((( uint16_t )buf[ 10 ]<< 8 ) | buf[ 11 ]); 
	int16_t gz_raw = ( int16_t )((( uint16_t )buf[ 12 ]<< 8 ) | buf[ 13 ]);

	gx = gx_raw * 6.1035e-2f;
	gy = gy_raw * 6.1035e-2f;
	gz = gz_raw * 6.1035e-2f;
	
}

void mpu6050_proc(mpu6050_handler mpu_x,uint16_t ms)
{
	if(mpu_x==NULL||ms==0)
	{
		return;
	}
//	static uint64_t last_tick=0;
//	uint64_t cur_tick=GetTick();
//	static TickType_t last_tick=0;
//	TickType_t cur_tick=xTaskGetTickCount();
//	if(cur_tick-last_tick>=ms)
//	{
		app_mpu6050_update(mpu_x);
			// 通过陀螺仪的测量结果计算欧拉角
		float yaw_g = yaw + gz * (ms/1000.0f);
		float pitch_g = pitch + gx *(ms/ 1000.0f);
		float roll_g = roll - gy * (ms/1000.0f);
		
		// 通过加速度计解算欧拉角
		float pitch_a = qatan2(ay, az) / 3.1415927f * 180.0f;
		float roll_a = qatan2(ax, az) / 3.1415927f * 180.0f;
		
		// 使用互补滤波器对陀螺仪和加速度计得计算结果进行融合
		yaw = yaw_g;
		pitch = 0.95238 * pitch_g + (1-0.95238) * pitch_a;
		roll = 0.95238 * roll_g + (1-0.95238) * roll_a;
//		last_tick=cur_tick;
//	}
}


//
// @简介：获取x轴向加速度，单位g
// 
float App_MPU6050_GetAx(mpu6050_handler mpu6050_x)
{
	if(mpu6050_x==NULL)
	{
		return 0;
	}
	return ax;
}

//
// @简介：获取y轴向加速度，单位g
// 
float App_MPU6050_GetAy(mpu6050_handler mpu6050_x)
{
	if(mpu6050_x==NULL)
	{
		return 0;
	}
	return ay;
}

//
// @简介：获取z轴向加速度，单位g
// 
float App_MPU6050_GetAz(mpu6050_handler mpu6050_x)
{
	if(mpu6050_x==NULL)
	{
		return 0;
	}
	return az;
}

//
// @简介：获取温度计的值，单位摄氏度
// 
float App_MPU6050_GetTemperature(mpu6050_handler mpu6050_x)
{
	if(mpu6050_x==NULL)
	{
		return 0;
	}
	return temperature;
}

//
// @简介：获取绕x轴的角速度，单位°/s
// 
float App_MPU6050_GetGx(mpu6050_handler mpu6050_x)
{
	if(mpu6050_x==NULL)
	{
		return 0;
	}
	return gx;
}

//
// @简介：获取绕y轴的角速度，单位°/s
// 
float App_MPU6050_GetGy(mpu6050_handler mpu6050_x)
{
	if(mpu6050_x==NULL)
	{
		return 0;
	}
	return gy;
}

//
// @简介：获取绕z轴的角速度，单位°/s
// 
float App_MPU6050_GetGz(mpu6050_handler mpu6050_x)
{
	if(mpu6050_x==NULL)
	{
		return 0;
	}
	return gz;
}

//
// @简介：用来获取偏航角，单位是°
//
float App_MPU6050_GetYaw(mpu6050_handler mpu_x)
{
	if(mpu_x==NULL)
	{
		return 0;
	}
	return yaw;
}

//
// @简介：用来获取俯仰角，单位是°
//
float App_MPU6050_GetPitch(mpu6050_handler mpu_x)
{
	if(mpu_x==NULL)
	{
		return 0;
	}
	return pitch;
}

//
// @简介：用来获取翻滚角，单位是°
//
float App_MPU6050_GetRoll(mpu6050_handler mpu_x)
{
	if(mpu_x==NULL)
	{
		return 0;
	}
	return roll;
}
