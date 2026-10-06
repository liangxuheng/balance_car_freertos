#ifndef __MY_USART_H_
#define __MY_USART_H_

#include <stdint.h>
struct usart_struct;
typedef struct usart_struct* usart_handler;
struct usart_nvic_struct;
typedef struct usart_nvic_struct* usart_nvic_handler;

void usart_init(usart_handler usart_x);
void usart_enable_rx_it(usart_handler usart_x, usart_nvic_handler nvic_x);
void my_usart_sendbytes(usart_handler usart_x,const uint8_t *pData
	,uint16_t size);
void my_usart_sendString(usart_handler usart_x,const char*string);
void My_USART_Printf(usart_handler usart_x, const char *Format, ...);
int My_USART_ReceiveLine(usart_handler usart_x, char *pStrOut, 
	uint16_t MaxLength, uint16_t LineSeperator, int Timeout);
#endif
