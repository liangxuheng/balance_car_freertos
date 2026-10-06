#ifndef __BOARD_H_
#define __BOARD_H_
/* 仅前置声明各模块句柄类型，不再 include 任何模块头，
 * 避免任何 .c 引入 board.h 就看到整个系统 */
struct timer_struct;
typedef struct timer_struct* timer_handler;
struct adc_struct;
typedef struct adc_struct* adc_handler;
struct usart_struct;
typedef struct usart_struct* usart_handler;
struct bat_led_init_struct;
typedef struct bat_led_init_struct* bat_led_init_handler;
struct my_button_struct;
typedef struct my_button_struct* my_button_handler;
struct mpu6050_struct;
typedef struct mpu6050_struct* mpu6050_handler;
struct motor_Typedef;
typedef struct motor_Typedef* motor_handler;
struct app_control_TypeDef;
typedef struct app_control_TypeDef* app_control_handler;
struct app_rc_TypeDef;
typedef struct app_rc_TypeDef* app_rc_handler;

void board_init(void);
extern timer_handler timer_1;
extern adc_handler adc_1;
extern usart_handler usart_1;
extern bat_led_init_handler __bat_led_init;
extern my_button_handler my_button_1;
extern mpu6050_handler mpu6050_1;
extern motor_handler motor_handler_my;
extern app_control_handler app_control_1;
extern app_rc_handler app_rc_1;
extern timer_handler timer_us;
#endif
