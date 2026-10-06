/**
 ******************************************************************************
 * @file    my_usart.c
 * @brief   串口驱动封装：GPIO/USART 初始化、阻塞发送、带超时接收
 * @author  平衡车项目
 *
 * @details 支持 printf 重定向（fputc 绑 USART2）。
 *          My_USART_ReceiveLine 按行分隔符读取，支持 \r / \n / \r\n。
 *
 ******************************************************************************
 */

#include "my_usart_desc.h"
#include "my_usart.h"
#include <stddef.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "delay.h"

#define LINE_SEPERATOR_CR   0x00 // 回车 \r
#define LINE_SEPERATOR_LF   0x01 // 换行 \n
#define LINE_SEPERATOR_CRLF 0x02 // 回车+换行 \r\n


/**
 * @brief  初始化串口：配置 GPIO、波特率、数据位、校验位
 * @param  usart_x  串口句柄
 * @retval None
 */
void usart_init(usart_handler usart_x)
{
	if(usart_x==NULL)
	{
		return;
	}
	GPIO_InitTypeDef GPIO_InStructer;
	GPIO_StructInit(&GPIO_InStructer);
	GPIO_InStructer.GPIO_Pin=usart_x->__GPIO_Tx_Pin;
	GPIO_InStructer.GPIO_Mode=usart_x->__GPIO_Tx_Mode;
	GPIO_InStructer.GPIO_Speed=usart_x->__GPIO_Tx_Speed;
	GPIO_Init(usart_x->__GPIO_X,&GPIO_InStructer);
	GPIO_InStructer.GPIO_Pin=usart_x->__GPIO_Rx_Pin;
	GPIO_InStructer.GPIO_Mode=usart_x->__GPIO_Rx_Mode;
	GPIO_InStructer.GPIO_Speed=usart_x->__GPIO_Rx_Speed;
	GPIO_Init(usart_x->__GPIO_X,&GPIO_InStructer);
	
	USART_InitTypeDef usart_InStructer;
	USART_StructInit(&usart_InStructer);
	usart_InStructer.USART_BaudRate=usart_x->__USART_BaudRate;
	usart_InStructer.USART_HardwareFlowControl=
	usart_x->__USART_HardwareFlowControl;
	usart_InStructer.USART_Mode=usart_x->__USART_Mode;
	usart_InStructer.USART_Parity=usart_x->__USART_Parity;
	usart_InStructer.USART_StopBits=usart_x->__USART_StopBits;
	usart_InStructer.USART_WordLength=usart_x->__USART_WordLength;
	USART_Init(usart_x->__usart_x,&usart_InStructer);
	
	USART_Cmd(usart_x->__usart_x,ENABLE);
}

/**
 * @brief  使能串口接收中断：配置 NVIC 并打开 USART 接收中断
 * @param  usart_x  串口句柄
 * @param  nvic_x   NVIC 配置（irqn + 中断类型）
 * @retval None
 */
void usart_enable_rx_it(usart_handler usart_x, usart_nvic_handler nvic_x)
{
	if(nvic_x==NULL||usart_x==NULL)
	{
		return;
	}
	NVIC_InitTypeDef nvic_InStruct;
	memset(&nvic_InStruct,0,sizeof(NVIC_InitTypeDef));
	nvic_InStruct.NVIC_IRQChannel=nvic_x->irqn;
	nvic_InStruct.NVIC_IRQChannelCmd=ENABLE;
	nvic_InStruct.NVIC_IRQChannelPreemptionPriority=5;
	nvic_InStruct.NVIC_IRQChannelSubPriority=0;
	NVIC_Init(&nvic_InStruct);
	USART_ITConfig(usart_x->__usart_x,nvic_x->usart_it_type,ENABLE);
}

/**
 * @brief  阻塞发送原始字节
 * @param  usart_x  串口句柄
 * @param  pData    数据缓冲区
 * @param  size     数据长度
 * @retval None
 */
__weak void my_usart_sendbytes(usart_handler usart_x,const uint8_t *pData
	,uint16_t size)
{
	if(usart_x==NULL||pData==NULL||size==0)
	{
		return;
	}
	
	for(uint16_t i=0;i<size;++i)
	{
		while(USART_GetFlagStatus(usart_x->__usart_x,USART_FLAG_TXE)==RESET);
		USART_SendData(usart_x->__usart_x,pData[i]);
	}
	while(USART_GetFlagStatus(usart_x->__usart_x,USART_FLAG_TC)==RESET);
}

/**
 * @brief  发送字符串
 * @param  usart_x  串口句柄
 * @param  string   以 \0 结尾的字符串
 * @retval None
 */
void my_usart_sendString(usart_handler usart_x,const char*string)
{
	if(usart_x==NULL||string==NULL)
	{
		return;
	}
	my_usart_sendbytes(usart_x,(const uint8_t *)string,strlen(string));
}

/**
 * @brief  格式化打印（类似 printf）
 * @param  usart_x  串口句柄
 * @param  Format   格式字符串
 * @retval None
 */
void My_USART_Printf(usart_handler usart_x, const char *Format, ...)
{
	char format_buffer[128]={0};
	va_list argptr;
	
	__va_start(argptr, Format);
	
	vsprintf(format_buffer, Format, argptr);
	
	__va_end(argptr);
	
	my_usart_sendString(usart_x, format_buffer);
}

/**
 * @brief  带超时接收指定长度字节
 * @param  usart_x   串口句柄
 * @param  pDataOut  接收缓冲区
 * @param  Size      期望接收长度
 * @param  Timeout   超时(ms)，负数=无限等待
 * @retval 实际接收到的字节数
 */
__weak uint16_t My_USART_ReceiveBytes(usart_handler usart_x, uint8_t *pDataOut
, uint16_t Size, int Timeout)
{
	if(usart_x==NULL||pDataOut==NULL||Size==0)
	{
		return 0;
	}
	uint64_t expireTime=0;
//	Delay_Init();
	if(Timeout>=0)
	{
		expireTime=GetTick()+Timeout;
	}
	uint16_t i=0;
	do {
		if(USART_GetFlagStatus(usart_x->__usart_x,USART_FLAG_RXNE)==SET)
		{
			pDataOut[i++]=USART_ReceiveData(usart_x->__usart_x);
			if(i==Size)
			{
				break;
			}
		}
	}while(Timeout<0||expireTime>0);
	return i;
}

/**
 * @brief  按行分隔符接收一行字符串
 * @param  usart_x       串口句柄
 * @param  pStrOut       输出缓冲区
 * @param  MaxLength     缓冲区最大长度
 * @param  LineSeperator 行分隔符(CR/LF/CRLF)
 * @param  Timeout       超时(ms)，负数=无限等待
 * @retval 0=成功, -1=超时, -2=超长
 */
int My_USART_ReceiveLine(usart_handler usart_x, char *pStrOut,
	uint16_t MaxLength, uint16_t LineSeperator, int Timeout)
{
	if(usart_x==NULL||pStrOut==NULL)
	{
		return -1;
	}
	
	if(MaxLength<2||(LineSeperator==LINE_SEPERATOR_CRLF&&MaxLength<3))
	{
		return -2;
	}
	
	int ret=-1;
	uint64_t expiretime=0;
	if(Timeout>=0)
	{
		expiretime=GetTick()+Timeout;
	}
	
	uint16_t i=0;
	
	do{
		if(USART_GetFlagStatus(usart_x->__usart_x,USART_FLAG_RXNE)==SET)
		{
			pStrOut[i++]=USART_ReceiveData(usart_x->__usart_x);
			if(LineSeperator==LINE_SEPERATOR_CR&&pStrOut[i-1]=='\r')
			{
				ret=0;
				break;
			}
			else if(LineSeperator==LINE_SEPERATOR_LF&&pStrOut[i-1]=='\n')
			{
				ret=0;
				break;
			}
			else if(LineSeperator==LINE_SEPERATOR_CRLF&&pStrOut[i-1]=='\n'
				&&i>=2&&pStrOut[i-2]=='\r')
			{
				ret=0;
				break;
			}
			
			if(i==MaxLength)
			{
				ret=-2;
				break;
			}
		}
	}while(Timeout<0||expiretime>0);
	
	pStrOut[i]=0;
	return ret;
}

/**
 * @brief  printf 重定向到 USART2
 * @param  ch     待发送字符
 * @param  file   文件指针（未使用）
 * @retval 发送的字符
 */
int fputc(int ch,FILE*file)
{
	(void)file;
	while(USART_GetFlagStatus(USART2,USART_FLAG_TXE)==RESET);
	USART_SendData(USART2,ch);
	return ch;
}
