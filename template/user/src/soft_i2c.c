/**
 ******************************************************************************
 * @file    soft_i2c.c
 * @brief   软件模拟 I2C：GPIO 模拟时序，读写寄存器
 ******************************************************************************
 */

#include "soft_i2c.h"
#include "soft_i2c_desc.h"

#include <stddef.h>
#define ERROR -1
#define CORRECT 0

static void delay_us(uint32_t us) { for(uint32_t i = 0; i<8*us; i++); }

static void set_scl(soft_i2c_handler i2cx,BitAction bit_level)
{
	GPIO_WriteBit(i2cx->Port_scl,i2cx->Pin_scl,bit_level);
}

static void set_sda(soft_i2c_handler i2cx,BitAction bit_level)
{
	GPIO_WriteBit(i2cx->Port_sda,i2cx->Pin_sda,bit_level);
}

static BitAction read_scl(soft_i2c_handler i2cx)
{
	return GPIO_ReadInputDataBit(i2cx->Port_scl,i2cx->Pin_scl);
}

static BitAction read_sda(soft_i2c_handler i2cx)
{
	return GPIO_ReadInputDataBit(i2cx->Port_sda,i2cx->Pin_sda);
}

/**
 * @brief  初始化软件 I2C：GPIO 开漏输出
 * @param  i2cx  I2C 句柄
 * @retval None
 */
void soft_i2c_init(soft_i2c_handler i2cx)
{
		if(i2cx==NULL)
		{
			return;
		}
		GPIO_InitTypeDef GPIO_InStructer;
		GPIO_StructInit(&GPIO_InStructer);
		GPIO_InStructer.GPIO_Mode=GPIO_Mode_Out_OD;
		GPIO_InStructer.GPIO_Pin=i2cx->Pin_scl;
		GPIO_InStructer.GPIO_Speed=GPIO_Speed_50MHz;
		GPIO_Init(i2cx->Port_scl,&GPIO_InStructer);
		GPIO_InStructer.GPIO_Pin=i2cx->Pin_sda;
		GPIO_Init(i2cx->Port_sda,&GPIO_InStructer);
		set_scl(i2cx,Bit_SET);
		set_sda(i2cx,Bit_SET);
}

/**
 * @brief  产生 I2C 起始条件
 * @param  i2cx  I2C 句柄
 * @retval None
 */
void soft_i2c_general_start(soft_i2c_handler i2cx)
{
			if(i2cx==NULL)
			{
				return;
			}
			set_scl(i2cx,Bit_SET);
			set_sda(i2cx,Bit_SET);
			delay_us(1);
			set_sda(i2cx,Bit_RESET);
			delay_us(1);
			set_scl(i2cx,Bit_RESET);
			delay_us(1);
}

/**
 * @brief  产生 I2C 终止条件
 * @param  i2cx  I2C 句柄
 * @retval None
 */
void soft_i2c_general_stop(soft_i2c_handler i2cx)
{
	if(i2cx==NULL)
	{
		return;
	}
	set_scl(i2cx,Bit_RESET);
	set_sda(i2cx,Bit_RESET);
	delay_us(1);
	set_scl(i2cx,Bit_SET);
	delay_us(1);
	set_sda(i2cx,Bit_SET);
	delay_us(1);
}

/**
 * @brief  I2C 发送一个字节
 * @param  i2cx      I2C 句柄
 * @param  date_byte 待发送字节
 * @retval None
 */
void soft_i2c_send_byte(soft_i2c_handler i2cx,uint8_t date_byte)
{
		if(i2cx==NULL)
		{
			return;
		}
		for(uint8_t i=0;i<sizeof(uint8_t)*8;++i)
		{
				set_scl(i2cx,Bit_RESET);
				delay_us(2);
				if(date_byte&(0x1<<(7-i)))
				{
					set_sda(i2cx,Bit_SET);
				}
				else 
				{
					set_sda(i2cx,Bit_RESET);
				}
				delay_us(2);
				set_scl(i2cx,Bit_SET);
				delay_us(2);
		}
//		set_sda(i2cx,Bit_RESET);
		set_scl(i2cx,Bit_RESET);
		set_sda(i2cx,Bit_SET);
}

/**
 * @brief  I2C 等待从机 ACK
 * @param  i2cx     I2C 句柄
 * @param  timerout 超时计数
 * @retval 0=ACK, -1=NACK/超时
 */
int8_t soft_i2c_recv_ack(soft_i2c_handler i2cx,int64_t timerout)
{
		if(i2cx==NULL)
		{
			return ERROR;
		}
		set_scl(i2cx,Bit_RESET);
		delay_us(1);
		//释放总线
		set_sda(i2cx,Bit_SET);
		delay_us(1);
//		set_scl(i2cx,Bit_SET);
//		delay_us(10);
//		BitAction ack=read_sda(i2cx);
//		set_scl(i2cx,Bit_RESET);
//		if(ack==Bit_RESET)
//		{
//				return CORRECT;
//		}
//		else 
//		{
//				return ERROR;
//		}
			do{
				timerout--;
				delay_us(1);
			}while((timerout>=0)&&(read_sda(i2cx)));
			if(timerout<0)
			{
				return ERROR;
			}
			set_scl(i2cx,Bit_SET);
			if(read_sda(i2cx))
			{
				set_scl(i2cx,Bit_RESET);
				return ERROR;
			}
			else 
			{
				set_scl(i2cx,Bit_RESET);
				return CORRECT;
			}
}

/**
 * @brief  I2C 读取一个字节
 * @param  i2cx  I2C 句柄
 * @retval 读取到的字节
 */
int8_t soft_i2c_read_byte(soft_i2c_handler i2cx)
{
	if(i2cx==NULL)
	{
		return -1;
	}
	uint8_t recv_data=0;
	set_sda(i2cx,Bit_SET);
	delay_us(1);
	for(uint8_t i=0;i<sizeof(uint8_t)*8;++i)
	{
			//等待从机发送应答
			set_scl(i2cx,Bit_RESET);
			delay_us(2);
			set_scl(i2cx,Bit_SET);
			delay_us(2);
			BitAction res=read_sda(i2cx);
			if(res==Bit_SET)
			{
				recv_data|=((0x1)<<(7-i));
			}
	}
	set_scl(i2cx,Bit_RESET);
	return recv_data;
}

/**
 * @brief  I2C 发送 ACK/NACK
 * @param  i2cx  I2C 句柄
 * @param  ack   0=ACK, 1=NACK
 * @retval None
 */
void soft_i2c_send_ack(soft_i2c_handler i2cx,uint8_t ack)
{
		if(i2cx==NULL)
		{
			return;
		}
		set_scl(i2cx,Bit_RESET);
		if(ack==0)
		{
			set_sda(i2cx,Bit_RESET);
		}
		else 
		{
			set_sda(i2cx,Bit_SET);
		}
		delay_us(2);
		set_scl(i2cx,Bit_SET);
		delay_us(2);
		set_scl(i2cx,Bit_RESET);
}


/**
 * @brief  I2C 寄存器写多个字节
 * @param  i2cx     I2C 句柄
 * @param  addr     7位从机地址
 * @param  reg      寄存器地址
 * @param  pdata    待发送数据
 * @param  size     数据长度
 * @param  timerout 超时计数
 * @retval >=0=发送字节数, -1=失败
 */
int32_t soft_i2c_regsend_bytes(soft_i2c_handler i2cx,uint8_t addr,uint8_t reg,const uint8_t*pdata
	,uint16_t size,uint64_t timerout)
{
	if(i2cx==NULL||pdata==NULL||size==0)
	{
		return -1;
	}
	soft_i2c_general_start(i2cx);
	soft_i2c_send_byte(i2cx,addr&0xFE);
	if(soft_i2c_recv_ack(i2cx,timerout))
	{
		soft_i2c_general_stop(i2cx);
		return -1;
	}
	soft_i2c_send_byte(i2cx,reg);
	if(soft_i2c_recv_ack(i2cx,timerout))
	{
		soft_i2c_general_stop(i2cx);
		return -1;
	}
	for(uint16_t i=0;i<size;++i)
	{
		soft_i2c_send_byte(i2cx,pdata[i]);
		if(soft_i2c_recv_ack(i2cx,timerout))
		{
			soft_i2c_general_stop(i2cx);
			return i;
		}
	}
	soft_i2c_general_stop(i2cx);
	return size;
}

/**
 * @brief  I2C 寄存器读多个字节
 * @param  i2cx     I2C 句柄
 * @param  addr     7位从机地址
 * @param  reg      寄存器地址
 * @param  pdata    接收缓冲区
 * @param  size     读取长度
 * @param  timerout 超时计数
 * @retval >=0=接收字节数, -1=失败
 */
int32_t soft_i2c_regread_bytes(soft_i2c_handler i2cx,uint8_t addr,uint8_t reg
,uint8_t*pdata,uint16_t size,uint64_t timerout)
{
		if(i2cx==NULL||pdata==NULL||size==0)
	{
		return -1;
	}
	soft_i2c_general_start(i2cx);
	soft_i2c_send_byte(i2cx,addr&0xFE);
	if(soft_i2c_recv_ack(i2cx,timerout))
	{
		soft_i2c_general_stop(i2cx);
		return -1;
	}
	soft_i2c_send_byte(i2cx,reg);
	if(soft_i2c_recv_ack(i2cx,timerout))
	{
		soft_i2c_general_stop(i2cx);
		return -1;
	}
	soft_i2c_general_start(i2cx);
	soft_i2c_send_byte(i2cx,addr|0x01);
	if(soft_i2c_recv_ack(i2cx,timerout))
	{
		soft_i2c_general_stop(i2cx);
		return -1;
	}
	for(uint16_t i=0;i<size;++i)
	{
		pdata[i]=soft_i2c_read_byte(i2cx);
		if(i==size-1)
		{
			soft_i2c_send_ack(i2cx,1);
			break;
		}
		soft_i2c_send_ack(i2cx,0);
	}
	soft_i2c_general_stop(i2cx);
	return size;
}
