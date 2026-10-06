#ifndef __APP_RC_DESC_H_
#define __APP_RC_DESC_H_
#include <stdint.h>
/* usart_nvic_struct 定义已移入 my_usart_desc.h，此处仅前置声明 */
struct usart_nvic_struct;
typedef struct usart_nvic_struct* usart_nvic_handler;
struct pid_Typedef;
typedef struct pid_Typedef*pid_handler;
struct usart_struct;
typedef struct usart_struct* usart_handler;
struct app_rc_TypeDef{
	usart_handler __usart;
	usart_nvic_handler __usart_nvic;
	pid_handler __pid_velocity;
	pid_handler __pid_turn;
};
#endif
