#ifndef __APP_CTX_H_
#define __APP_CTX_H_

struct motor_Typedef;
typedef struct motor_Typedef* motor_handler;
struct app_control_TypeDef;
typedef struct app_control_TypeDef* app_control_handler;
struct app_rc_TypeDef;
typedef struct app_rc_TypeDef* app_rc_handler;
struct mpu6050_struct;
typedef struct mpu6050_struct* mpu6050_handler;
struct my_button_struct;
typedef struct my_button_struct* my_button_handler;
struct usart_struct;
typedef struct usart_struct* usart_handler;

/* 平衡车应用上下文：main.c 填充一次，业务回调通过它访问句柄（替代直接 extern 全局） */
typedef struct {
    motor_handler       motor;    /* 电机（原全局 motor_handler_my） */
    app_control_handler control;  /* 平衡控制（原 app_control_1） */
    app_rc_handler      rc;       /* 遥控接收（原 app_rc_1） */
    mpu6050_handler     mpu;      /* MPU6050（原 mpu6050_1） */
    my_button_handler   button;   /* 按键（原 my_button_1） */
    usart_handler       usart;    /* 串口（原 usart_1） */
} balance_app_t;

#endif
