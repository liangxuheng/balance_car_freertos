//#ifndef __APP_MPU6050_DESC_H_
//#define __APP_MPU6050_DESC_H_
//struct soft_i2c;
//typedef struct soft_i2c* soft_i2c_handler;
//struct mpu6050_struct{
//	soft_i2c_handler __soft_i2c;
//};
//#endif

# ifndef __APP_MPU6050_DESC_H_ 
# define __APP_MPU6050_DESC_H_ 
struct hard_i2c_Typedef;
typedef struct hard_i2c_Typedef* hard_i2c_handler; 
struct mpu6050_struct { 
	hard_i2c_handler __hard_i2c;
}; 
# endif
