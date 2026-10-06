#ifndef __MY_USART_DESC_H_
#define __MY_USART_DESC_H_

#include "stm32f10x.h"
#include <stdint.h>
/* 串口接收中断的 NVIC 配置（由驱动层统一初始化） */
struct usart_nvic_struct{
	uint8_t irqn;
	uint16_t usart_it_type;
};
typedef struct usart_nvic_struct* usart_nvic_handler;
struct usart_struct
{
	//串口波特率
	uint32_t __USART_BaudRate;
	//数据帧的长度
	uint16_t __USART_WordLength;
	//停止位的长度
	uint16_t __USART_StopBits;
	//校验位
	uint16_t __USART_Parity;
	//串口的模式
	uint16_t __USART_Mode;
	//硬件流控
	uint16_t __USART_HardwareFlowControl;
	uint16_t __GPIO_Tx_Pin;
	GPIOSpeed_TypeDef __GPIO_Tx_Speed;
	GPIOMode_TypeDef __GPIO_Tx_Mode;
	uint16_t __GPIO_Rx_Pin;
	GPIOSpeed_TypeDef __GPIO_Rx_Speed;
	GPIOMode_TypeDef __GPIO_Rx_Mode;
	USART_TypeDef*__usart_x;   /* 新增：串口外设，如 USART2 */
	GPIO_TypeDef*__GPIO_X;     /* 新增：Tx/Rx 所在端口，如 GPIOA */
};

#endif
