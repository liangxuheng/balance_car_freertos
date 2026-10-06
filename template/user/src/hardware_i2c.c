/**
 ******************************************************************************
 * @file    hardware_i2c.c
 * @brief   硬件 I2C 驱动：阻塞读写 + 总线复位，供 MPU6050 使用
 ******************************************************************************
 */

#include "hardware_i2c.h"
#include "hardware_i2c_desc.h"
#include "FreeRTOS.h"
#include "task.h"
#include "delay.h"
#include <stddef.h>
#include <stdbool.h>

static void hard_i2c_gpio_init(i2c_gpio_handler gpio_x,uint8_t remap);
static void hard_i2c_base_init(i2c_base_hadler base_x);

/**
 * @brief  初始化硬件 I2C：GPIO + I2C 外设
 * @param  hard_i2c_x  I2C 句柄
 * @retval None
 */
void hardware_i2c_init(hard_i2c_handler hard_i2c_x)
{
	if(hard_i2c_x==NULL)
	{
		return;
	}
	hard_i2c_gpio_init(hard_i2c_x->__i2c_gpio,hard_i2c_x->__remap);
	hard_i2c_base_init(hard_i2c_x->__i2c_base);
}

static void hard_i2c_base_init(i2c_base_hadler base_x)
{
	if(base_x==NULL)
	{
		return;
	}
	I2C_InitTypeDef i2c_InStruct;
	I2C_DeInit(base_x->__I2C_x);
	I2C_StructInit(&i2c_InStruct);
	i2c_InStruct.I2C_Ack=base_x->__I2C_Ack;
	i2c_InStruct.I2C_AcknowledgedAddress=base_x->__I2C_AcknowledgedAddress;
	i2c_InStruct.I2C_ClockSpeed=base_x->__I2C_ClockSpeed;
	i2c_InStruct.I2C_DutyCycle=base_x->__I2C_DutyCycle;
	i2c_InStruct.I2C_Mode=base_x->__I2C_Mode;
	i2c_InStruct.I2C_OwnAddress1=base_x->__I2C_OwnAddress1;
	I2C_Init(base_x->__I2C_x,&i2c_InStruct);
	I2C_Cmd(base_x->__I2C_x,ENABLE);
}

static void hard_i2c_gpio_init(i2c_gpio_handler gpio_x,uint8_t remap)
{
	if(gpio_x==NULL)
	{
		return;
	}
	if(remap)
	{
		GPIO_PinRemapConfig(GPIO_Remap_I2C1,ENABLE);
	}
	GPIO_InitTypeDef gpio_InStruct;
	GPIO_StructInit(&gpio_InStruct);
	gpio_InStruct.GPIO_Mode=gpio_x->__GPIO_Mode_Scl;
	gpio_InStruct.GPIO_Pin=gpio_x->__GPIO_Pin_SCL;
	gpio_InStruct.GPIO_Speed=gpio_x->__GPIO_Speed_Scl;
	GPIO_Init(gpio_x->__GPIOx_SCL,&gpio_InStruct);
	gpio_InStruct.GPIO_Mode=gpio_x->__GPIO_Mode_Sda;
	gpio_InStruct.GPIO_Pin=gpio_x->__GPIO_Pin_SDA;
	gpio_InStruct.GPIO_Speed=gpio_x->__GPIO_Speed_Sda;
	GPIO_Init(gpio_x->__GPIOx_SDA,&gpio_InStruct);
}

/**
 * @brief  I2C 写多个字节到从机
 * @param  hard_i2c_x   I2C 句柄
 * @param  addr         7位从机地址
 * @param  pdata        待发送数据
 * @param  pdata_size   数据长度
 * @param  time_out     超时计数
 * @retval >=0=发送字节数, -1=超时/NACK, -2=数据NACK
 */
__weak int hardware_i2c_sendBytes(hard_i2c_handler hard_i2c_x,uint8_t addr
	,const uint8_t*pdata,uint16_t pdata_size,uint64_t time_out)
{
	if(hard_i2c_x==NULL||pdata==NULL||pdata_size==0)
	{
		return -1;
	}
	//等待总线空闲
	while(true)
	{
		if(I2C_GetFlagStatus(hard_i2c_x->__i2c_base->__I2C_x,I2C_FLAG_BUSY)==RESET)
		{
			break;
		}
		else if(time_out==0)
		{
			return -1;
		}
		--time_out;
	}
	//发送起始位
	I2C_GenerateSTART(hard_i2c_x->__i2c_base->__I2C_x,ENABLE);
	//等待SB
	while(true)
	{
		if(I2C_GetFlagStatus(hard_i2c_x->__i2c_base->__I2C_x,I2C_FLAG_SB)==SET)
		{
			break;
		}
		else if(time_out==0)
		{
			I2C_GenerateSTOP(hard_i2c_x->__i2c_base->__I2C_x,ENABLE);
			return -1;
		}
		--time_out;
	}
	//寻址（写方向）
	I2C_ClearFlag(hard_i2c_x->__i2c_base->__I2C_x,I2C_FLAG_AF);
	I2C_SendData(hard_i2c_x->__i2c_base->__I2C_x,addr&0xfe);
	while(true)
	{
		if(I2C_GetFlagStatus(hard_i2c_x->__i2c_base->__I2C_x,I2C_FLAG_ADDR)==SET)
		{
			break;
		}
		if(I2C_GetFlagStatus(hard_i2c_x->__i2c_base->__I2C_x,I2C_FLAG_AF)==SET)
		{
			I2C_GenerateSTOP(hard_i2c_x->__i2c_base->__I2C_x,ENABLE);
			return -1;
		}
		else if(time_out==0)
		{
			I2C_GenerateSTOP(hard_i2c_x->__i2c_base->__I2C_x,ENABLE);
			return -1;
		}
		--time_out;
	}
	//读SR1再读SR2清除ADDR
	I2C_ReadRegister(hard_i2c_x->__i2c_base->__I2C_x,I2C_Register_SR1);
	I2C_ReadRegister(hard_i2c_x->__i2c_base->__I2C_x,I2C_Register_SR2);

	if(pdata_size==1)
	{
		//单字节：等TXE -> 发数据 -> 等BTF -> STOP
		while(true)
		{
			if(I2C_GetFlagStatus(hard_i2c_x->__i2c_base->__I2C_x,I2C_FLAG_TXE)==SET)
			{
				break;
			}
			if(I2C_GetFlagStatus(hard_i2c_x->__i2c_base->__I2C_x,I2C_FLAG_AF)==SET)
			{
				I2C_GenerateSTOP(hard_i2c_x->__i2c_base->__I2C_x,ENABLE);
				return -2;
			}
			else if(time_out==0)
			{
				I2C_GenerateSTOP(hard_i2c_x->__i2c_base->__I2C_x,ENABLE);
				return -1;
			}
			--time_out;
		}
		I2C_SendData(hard_i2c_x->__i2c_base->__I2C_x,pdata[0]);
		while(true)
		{
			if(I2C_GetFlagStatus(hard_i2c_x->__i2c_base->__I2C_x,I2C_FLAG_BTF)==SET)
			{
				break;
			}
			if(I2C_GetFlagStatus(hard_i2c_x->__i2c_base->__I2C_x,I2C_FLAG_AF)==SET)
			{
				I2C_GenerateSTOP(hard_i2c_x->__i2c_base->__I2C_x,ENABLE);
				return -2;
			}
			else if(time_out==0)
			{
				I2C_GenerateSTOP(hard_i2c_x->__i2c_base->__I2C_x,ENABLE);
				return -1;
			}
			--time_out;
		}
		I2C_GenerateSTOP(hard_i2c_x->__i2c_base->__I2C_x,ENABLE);
		return 1;
	}
	else
	{
		//多字节：逐字节发送
		for(uint16_t i=0;i<pdata_size;i++)
		{
			while(true)
			{
				if(I2C_GetFlagStatus(hard_i2c_x->__i2c_base->__I2C_x,I2C_FLAG_TXE)==SET)
				{
					break;
				}
				if(I2C_GetFlagStatus(hard_i2c_x->__i2c_base->__I2C_x,I2C_FLAG_AF)==SET)
				{
					I2C_GenerateSTOP(hard_i2c_x->__i2c_base->__I2C_x,ENABLE);
					return -2;
				}
				else if(time_out==0)
				{
					I2C_GenerateSTOP(hard_i2c_x->__i2c_base->__I2C_x,ENABLE);
					return -1;
				}
				--time_out;
			}
			I2C_SendData(hard_i2c_x->__i2c_base->__I2C_x,pdata[i]);
		}
		//STOP前等BTF，确认最后一字节连同ACK位传完
		while(true)
		{
			if(I2C_GetFlagStatus(hard_i2c_x->__i2c_base->__I2C_x,I2C_FLAG_BTF)==SET)
			{
				break;
			}
			if(I2C_GetFlagStatus(hard_i2c_x->__i2c_base->__I2C_x,I2C_FLAG_AF)==SET)
			{
				I2C_GenerateSTOP(hard_i2c_x->__i2c_base->__I2C_x,ENABLE);
				return -2;
			}
			else if(time_out==0)
			{
				I2C_GenerateSTOP(hard_i2c_x->__i2c_base->__I2C_x,ENABLE);
				return -1;
			}
			--time_out;
		}
		I2C_GenerateSTOP(hard_i2c_x->__i2c_base->__I2C_x,ENABLE);
		return (int)pdata_size;
	}
}

/**
 * @brief  I2C 从从机读多个字节
 * @param  hard_i2c_x  I2C 句柄
 * @param  addr        7位从机地址
 * @param  pbuf        接收缓冲区
 * @param  buf_size    读取长度
 * @param  time_out    超时计数
 * @retval >=0=接收字节数, -1=超时/NACK
 */
__weak int hardware_i2c_receiveBytes(hard_i2c_handler hard_i2c_x,uint8_t addr
	,uint8_t*pbuf,uint16_t buf_size,uint64_t time_out)
{
	if(hard_i2c_x==NULL||pbuf==NULL||buf_size==0)
	{
		return -1;
	}
	//等待总线空闲
	while(true)
	{
		if(I2C_GetFlagStatus(hard_i2c_x->__i2c_base->__I2C_x,I2C_FLAG_BUSY)==RESET)
		{
			break;
		}
		else if(time_out==0)
		{
			return -1;
		}
		--time_out;
	}
	//发送起始位
	I2C_GenerateSTART(hard_i2c_x->__i2c_base->__I2C_x,ENABLE);
	//等待SB
	while(true)
	{
		if(I2C_GetFlagStatus(hard_i2c_x->__i2c_base->__I2C_x,I2C_FLAG_SB)==SET)
		{
			break;
		}
		else if(time_out==0)
		{
			I2C_GenerateSTOP(hard_i2c_x->__i2c_base->__I2C_x,ENABLE);
			return -1;
		}
		--time_out;
	}
	//寻址（读方向）
	I2C_ClearFlag(hard_i2c_x->__i2c_base->__I2C_x,I2C_FLAG_AF);
	I2C_SendData(hard_i2c_x->__i2c_base->__I2C_x,addr|0x01);
	while(true)
	{
		if(I2C_GetFlagStatus(hard_i2c_x->__i2c_base->__I2C_x,I2C_FLAG_ADDR)==SET)
		{
			break;
		}
		if(I2C_GetFlagStatus(hard_i2c_x->__i2c_base->__I2C_x,I2C_FLAG_AF)==SET)
		{
			I2C_GenerateSTOP(hard_i2c_x->__i2c_base->__I2C_x,ENABLE);
			return -1;
		}
		else if(time_out==0)
		{
			I2C_GenerateSTOP(hard_i2c_x->__i2c_base->__I2C_x,ENABLE);
			return -1;
		}
		--time_out;
	}

	if(buf_size==1)
	{
		//单字节：先关ACK，临界区清ADDR+STOP，再等RXNE读取
		I2C_AcknowledgeConfig(hard_i2c_x->__i2c_base->__I2C_x,DISABLE);
		taskENTER_CRITICAL();
		I2C_ReadRegister(hard_i2c_x->__i2c_base->__I2C_x,I2C_Register_SR1);
		I2C_ReadRegister(hard_i2c_x->__i2c_base->__I2C_x,I2C_Register_SR2);
		I2C_GenerateSTOP(hard_i2c_x->__i2c_base->__I2C_x,ENABLE);
		taskEXIT_CRITICAL();
		while(true)
		{
			if(I2C_GetFlagStatus(hard_i2c_x->__i2c_base->__I2C_x,I2C_FLAG_RXNE)==SET)
			{
				break;
			}
			else if(time_out==0)
			{
				return -1;
			}
			--time_out;
		}
		pbuf[0]=I2C_ReceiveData(hard_i2c_x->__i2c_base->__I2C_x);
		return 1;
	}
	else
	{
		//多字节：前N-1字节回ACK
		I2C_AcknowledgeConfig(hard_i2c_x->__i2c_base->__I2C_x,ENABLE);
		I2C_ReadRegister(hard_i2c_x->__i2c_base->__I2C_x,I2C_Register_SR1);
		I2C_ReadRegister(hard_i2c_x->__i2c_base->__I2C_x,I2C_Register_SR2);

		for(uint16_t i=0;i<buf_size-1;i++)
		{
			//读倒数第二字节时进临界区：关ACK+STOP必须赶在最后一字节ACK位之前
			if(i==buf_size-2)
			{
				taskENTER_CRITICAL();
			}
			while(true)
			{
				if(I2C_GetFlagStatus(hard_i2c_x->__i2c_base->__I2C_x,I2C_FLAG_RXNE)==SET)
				{
					break;
				}
				else if(time_out==0)
				{
					taskEXIT_CRITICAL();
					return -1;
				}
				--time_out;
			}
			pbuf[i]=I2C_ReceiveData(hard_i2c_x->__i2c_base->__I2C_x);
		}
		//最后一字节：关ACK+STOP
		I2C_AcknowledgeConfig(hard_i2c_x->__i2c_base->__I2C_x,DISABLE);
		I2C_GenerateSTOP(hard_i2c_x->__i2c_base->__I2C_x,ENABLE);
		taskEXIT_CRITICAL();
		while(true)
		{
			if(I2C_GetFlagStatus(hard_i2c_x->__i2c_base->__I2C_x,I2C_FLAG_RXNE)==SET)
			{
				break;
			}
			else if(time_out==0)
			{
				return -1;
			}
			--time_out;
		}
		pbuf[buf_size-1]=I2C_ReceiveData(hard_i2c_x->__i2c_base->__I2C_x);
		return (int)buf_size;
	}
}

/**
 * @brief  I2C 寄存器读：写寄存器地址后重复起始读多个字节
 * @param  hard_i2c_x  I2C 句柄
 * @param  addr        7位从机地址
 * @param  reg         寄存器地址
 * @param  pbuf        接收缓冲区
 * @param  buf_size    读取长度
 * @param  time_out    超时计数
 * @retval >=0=接收字节数, -1=超时/NACK, -2=数据NACK
 */
__weak int hardware_i2c_regReadBytes(hard_i2c_handler hard_i2c_x,uint8_t addr
	,uint8_t reg,uint8_t*pbuf,uint16_t buf_size,uint64_t time_out)
{
	if(hard_i2c_x==NULL||pbuf==NULL||buf_size==0)
	{
		return -1;
	}
	//等待总线空闲
	while(true)
	{
		if(I2C_GetFlagStatus(hard_i2c_x->__i2c_base->__I2C_x,I2C_FLAG_BUSY)==RESET)
		{
			break;
		}
		else if(time_out==0)
		{
			return -1;
		}
		--time_out;
	}
	//发送起始位
	I2C_GenerateSTART(hard_i2c_x->__i2c_base->__I2C_x,ENABLE);
	//等待SB
	while(true)
	{
		if(I2C_GetFlagStatus(hard_i2c_x->__i2c_base->__I2C_x,I2C_FLAG_SB)==SET)
		{
			break;
		}
		else if(time_out==0)
		{
			I2C_GenerateSTOP(hard_i2c_x->__i2c_base->__I2C_x,ENABLE);
			return -1;
		}
		--time_out;
	}
	//寻址（写方向）
	I2C_ClearFlag(hard_i2c_x->__i2c_base->__I2C_x,I2C_FLAG_AF);
	I2C_SendData(hard_i2c_x->__i2c_base->__I2C_x,addr&0xfe);
	while(true)
	{
		if(I2C_GetFlagStatus(hard_i2c_x->__i2c_base->__I2C_x,I2C_FLAG_ADDR)==SET)
		{
			break;
		}
		if(I2C_GetFlagStatus(hard_i2c_x->__i2c_base->__I2C_x,I2C_FLAG_AF)==SET)
		{
			I2C_GenerateSTOP(hard_i2c_x->__i2c_base->__I2C_x,ENABLE);
			return -1;
		}
		else if(time_out==0)
		{
			I2C_GenerateSTOP(hard_i2c_x->__i2c_base->__I2C_x,ENABLE);
			return -1;
		}
		--time_out;
	}
	I2C_ReadRegister(hard_i2c_x->__i2c_base->__I2C_x,I2C_Register_SR1);
	I2C_ReadRegister(hard_i2c_x->__i2c_base->__I2C_x,I2C_Register_SR2);
	//发送寄存器地址（后面跟重复起始，只等TXE，不等BTF）
	while(true)
	{
		if(I2C_GetFlagStatus(hard_i2c_x->__i2c_base->__I2C_x,I2C_FLAG_TXE)==SET)
		{
			break;
		}
		else if(time_out==0)
		{
			I2C_GenerateSTOP(hard_i2c_x->__i2c_base->__I2C_x,ENABLE);
			return -1;
		}
		--time_out;
	}
	I2C_SendData(hard_i2c_x->__i2c_base->__I2C_x,reg);
	while(true)
	{
		if(I2C_GetFlagStatus(hard_i2c_x->__i2c_base->__I2C_x,I2C_FLAG_TXE)==SET)
		{
			break;
		}
		if(I2C_GetFlagStatus(hard_i2c_x->__i2c_base->__I2C_x,I2C_FLAG_AF)==SET)
		{
			I2C_GenerateSTOP(hard_i2c_x->__i2c_base->__I2C_x,ENABLE);
			return -2;
		}
		else if(time_out==0)
		{
			I2C_GenerateSTOP(hard_i2c_x->__i2c_base->__I2C_x,ENABLE);
			return -1;
		}
		--time_out;
	}
	//重复起始，换读方向
	I2C_GenerateSTART(hard_i2c_x->__i2c_base->__I2C_x,ENABLE);
	while(true)
	{
		if(I2C_GetFlagStatus(hard_i2c_x->__i2c_base->__I2C_x,I2C_FLAG_SB)==SET)
		{
			break;
		}
		else if(time_out==0)
		{
			I2C_GenerateSTOP(hard_i2c_x->__i2c_base->__I2C_x,ENABLE);
			return -1;
		}
		--time_out;
	}
	//寻址（读方向）
	I2C_ClearFlag(hard_i2c_x->__i2c_base->__I2C_x,I2C_FLAG_AF);
	I2C_SendData(hard_i2c_x->__i2c_base->__I2C_x,addr|0x01);
	while(true)
	{
		if(I2C_GetFlagStatus(hard_i2c_x->__i2c_base->__I2C_x,I2C_FLAG_ADDR)==SET)
		{
			break;
		}
		if(I2C_GetFlagStatus(hard_i2c_x->__i2c_base->__I2C_x,I2C_FLAG_AF)==SET)
		{
			I2C_GenerateSTOP(hard_i2c_x->__i2c_base->__I2C_x,ENABLE);
			return -1;
		}
		else if(time_out==0)
		{
			I2C_GenerateSTOP(hard_i2c_x->__i2c_base->__I2C_x,ENABLE);
			return -1;
		}
		--time_out;
	}

	if(buf_size==1)
	{
		//单字节读
		I2C_AcknowledgeConfig(hard_i2c_x->__i2c_base->__I2C_x,DISABLE);
		taskENTER_CRITICAL();
		I2C_ReadRegister(hard_i2c_x->__i2c_base->__I2C_x,I2C_Register_SR1);
		I2C_ReadRegister(hard_i2c_x->__i2c_base->__I2C_x,I2C_Register_SR2);
		I2C_GenerateSTOP(hard_i2c_x->__i2c_base->__I2C_x,ENABLE);
		taskEXIT_CRITICAL();
		while(true)
		{
			if(I2C_GetFlagStatus(hard_i2c_x->__i2c_base->__I2C_x,I2C_FLAG_RXNE)==SET)
			{
				break;
			}
			else if(time_out==0)
			{
				return -1;
			}
			--time_out;
		}
		pbuf[0]=I2C_ReceiveData(hard_i2c_x->__i2c_base->__I2C_x);
		return 1;
	}
	else
	{
		//多字节读
		I2C_AcknowledgeConfig(hard_i2c_x->__i2c_base->__I2C_x,ENABLE);
		I2C_ReadRegister(hard_i2c_x->__i2c_base->__I2C_x,I2C_Register_SR1);
		I2C_ReadRegister(hard_i2c_x->__i2c_base->__I2C_x,I2C_Register_SR2);

		for(uint16_t i=0;i<buf_size-1;i++)
		{
			if(i==buf_size-2)
			{
				taskENTER_CRITICAL();
			}
			while(true)
			{
				if(I2C_GetFlagStatus(hard_i2c_x->__i2c_base->__I2C_x,I2C_FLAG_RXNE)==SET)
				{
					break;
				}
				else if(time_out==0)
				{
					taskEXIT_CRITICAL();
					return -1;
				}
				--time_out;
			}
			pbuf[i]=I2C_ReceiveData(hard_i2c_x->__i2c_base->__I2C_x);
		}
		I2C_AcknowledgeConfig(hard_i2c_x->__i2c_base->__I2C_x,DISABLE);
		I2C_GenerateSTOP(hard_i2c_x->__i2c_base->__I2C_x,ENABLE);
		taskEXIT_CRITICAL();
		while(true)
		{
			if(I2C_GetFlagStatus(hard_i2c_x->__i2c_base->__I2C_x,I2C_FLAG_RXNE)==SET)
			{
				break;
			}
			else if(time_out==0)
			{
				return -1;
			}
			--time_out;
		}
		pbuf[buf_size-1]=I2C_ReceiveData(hard_i2c_x->__i2c_base->__I2C_x);
		return (int)buf_size;
	}
}

/**
 * @brief  I2C 寄存器写：写寄存器地址后写多个字节
 * @param  hard_i2c_x  I2C 句柄
 * @param  addr        7位从机地址
 * @param  reg         寄存器地址
 * @param  pdata       待发送数据
 * @param  pdata_size  数据长度
 * @param  time_out    超时计数
 * @retval >=0=发送字节数, -1=超时/NACK, -2=数据NACK
 */
__weak int hardware_i2c_memWriteBytes(hard_i2c_handler hard_i2c_x,uint8_t addr
	,uint8_t reg,const uint8_t*pdata,uint16_t pdata_size,uint64_t time_out)
{
	if(hard_i2c_x==NULL||pdata==NULL||pdata_size==0)
	{
		return -1;
	}
	//等待总线空闲
	while(true)
	{
		if(I2C_GetFlagStatus(hard_i2c_x->__i2c_base->__I2C_x,I2C_FLAG_BUSY)==RESET)
		{
			break;
		}
		else if(time_out==0)
		{
			return -1;
		}
		--time_out;
	}
	//发送起始位
	I2C_GenerateSTART(hard_i2c_x->__i2c_base->__I2C_x,ENABLE);
	//等待SB
	while(true)
	{
		if(I2C_GetFlagStatus(hard_i2c_x->__i2c_base->__I2C_x,I2C_FLAG_SB)==SET)
		{
			break;
		}
		else if(time_out==0)
		{
			I2C_GenerateSTOP(hard_i2c_x->__i2c_base->__I2C_x,ENABLE);
			return -1;
		}
		--time_out;
	}
	//寻址（写方向）
	I2C_ClearFlag(hard_i2c_x->__i2c_base->__I2C_x,I2C_FLAG_AF);
	I2C_SendData(hard_i2c_x->__i2c_base->__I2C_x,addr&0xfe);
	while(true)
	{
		if(I2C_GetFlagStatus(hard_i2c_x->__i2c_base->__I2C_x,I2C_FLAG_ADDR)==SET)
		{
			break;
		}
		if(I2C_GetFlagStatus(hard_i2c_x->__i2c_base->__I2C_x,I2C_FLAG_AF)==SET)
		{
			I2C_GenerateSTOP(hard_i2c_x->__i2c_base->__I2C_x,ENABLE);
			return -1;
		}
		else if(time_out==0)
		{
			I2C_GenerateSTOP(hard_i2c_x->__i2c_base->__I2C_x,ENABLE);
			return -1;
		}
		--time_out;
	}
	I2C_ReadRegister(hard_i2c_x->__i2c_base->__I2C_x,I2C_Register_SR1);
	I2C_ReadRegister(hard_i2c_x->__i2c_base->__I2C_x,I2C_Register_SR2);
	//发送寄存器地址
	while(true)
	{
		if(I2C_GetFlagStatus(hard_i2c_x->__i2c_base->__I2C_x,I2C_FLAG_TXE)==SET)
		{
			break;
		}
		else if(time_out==0)
		{
			I2C_GenerateSTOP(hard_i2c_x->__i2c_base->__I2C_x,ENABLE);
			return -1;
		}
		--time_out;
	}
	I2C_SendData(hard_i2c_x->__i2c_base->__I2C_x,reg);
	while(true)
	{
		if(I2C_GetFlagStatus(hard_i2c_x->__i2c_base->__I2C_x,I2C_FLAG_TXE)==SET)
		{
			break;
		}
		if(I2C_GetFlagStatus(hard_i2c_x->__i2c_base->__I2C_x,I2C_FLAG_AF)==SET)
		{
			I2C_GenerateSTOP(hard_i2c_x->__i2c_base->__I2C_x,ENABLE);
			return -2;
		}
		else if(time_out==0)
		{
			I2C_GenerateSTOP(hard_i2c_x->__i2c_base->__I2C_x,ENABLE);
			return -1;
		}
		--time_out;
	}

	if(pdata_size==1)
	{
		//单字节：等TXE -> 发数据 -> 等BTF -> STOP
		while(true)
		{
			if(I2C_GetFlagStatus(hard_i2c_x->__i2c_base->__I2C_x,I2C_FLAG_TXE)==SET)
			{
				break;
			}
			if(I2C_GetFlagStatus(hard_i2c_x->__i2c_base->__I2C_x,I2C_FLAG_AF)==SET)
			{
				I2C_GenerateSTOP(hard_i2c_x->__i2c_base->__I2C_x,ENABLE);
				return -2;
			}
			else if(time_out==0)
			{
				I2C_GenerateSTOP(hard_i2c_x->__i2c_base->__I2C_x,ENABLE);
				return -1;
			}
			--time_out;
		}
		I2C_SendData(hard_i2c_x->__i2c_base->__I2C_x,pdata[0]);
		while(true)
		{
			if(I2C_GetFlagStatus(hard_i2c_x->__i2c_base->__I2C_x,I2C_FLAG_BTF)==SET)
			{
				break;
			}
			if(I2C_GetFlagStatus(hard_i2c_x->__i2c_base->__I2C_x,I2C_FLAG_AF)==SET)
			{
				I2C_GenerateSTOP(hard_i2c_x->__i2c_base->__I2C_x,ENABLE);
				return -2;
			}
			else if(time_out==0)
			{
				I2C_GenerateSTOP(hard_i2c_x->__i2c_base->__I2C_x,ENABLE);
				return -1;
			}
			--time_out;
		}
		I2C_GenerateSTOP(hard_i2c_x->__i2c_base->__I2C_x,ENABLE);
		return 1;
	}
	else
	{
		//多字节：逐字节发送数据
		for(uint16_t i=0;i<pdata_size;i++)
		{
			while(true)
			{
				if(I2C_GetFlagStatus(hard_i2c_x->__i2c_base->__I2C_x,I2C_FLAG_TXE)==SET)
				{
					break;
				}
				if(I2C_GetFlagStatus(hard_i2c_x->__i2c_base->__I2C_x,I2C_FLAG_AF)==SET)
				{
					I2C_GenerateSTOP(hard_i2c_x->__i2c_base->__I2C_x,ENABLE);
					return -2;
				}
				else if(time_out==0)
				{
					I2C_GenerateSTOP(hard_i2c_x->__i2c_base->__I2C_x,ENABLE);
					return -1;
				}
				--time_out;
			}
			I2C_SendData(hard_i2c_x->__i2c_base->__I2C_x,pdata[i]);
		}
		//等BTF再STOP
		while(true)
		{
			if(I2C_GetFlagStatus(hard_i2c_x->__i2c_base->__I2C_x,I2C_FLAG_BTF)==SET)
			{
				break;
			}
			if(I2C_GetFlagStatus(hard_i2c_x->__i2c_base->__I2C_x,I2C_FLAG_AF)==SET)
			{
				I2C_GenerateSTOP(hard_i2c_x->__i2c_base->__I2C_x,ENABLE);
				return -2;
			}
			else if(time_out==0)
			{
				I2C_GenerateSTOP(hard_i2c_x->__i2c_base->__I2C_x,ENABLE);
				return -1;
			}
			--time_out;
		}
		I2C_GenerateSTOP(hard_i2c_x->__i2c_base->__I2C_x,ENABLE);
		return (int)pdata_size;
	}
}

/**
 * @brief  I2C 总线死锁恢复：SCL 手动打9脉冲 + STOP + 重新初始化
 * @param  hard_i2c_x  I2C 句柄
 * @retval None
 */
void hardware_i2c_bus_reset(hard_i2c_handler hard_i2c_x)
{
	if(hard_i2c_x==NULL||hard_i2c_x->__i2c_gpio==NULL||hard_i2c_x->__i2c_base==NULL)
	{
		return;
	}
	GPIO_InitTypeDef gpio_InStruct;

	//① 禁用I2C外设，否则它霸占引脚
	I2C_Cmd(hard_i2c_x->__i2c_base->__I2C_x,DISABLE);

	//② SCL/SDA切成普通开漏，软件接管
	GPIO_StructInit(&gpio_InStruct);
	gpio_InStruct.GPIO_Mode=GPIO_Mode_Out_OD;
	gpio_InStruct.GPIO_Speed=GPIO_Speed_50MHz;
	gpio_InStruct.GPIO_Pin=hard_i2c_x->__i2c_gpio->__GPIO_Pin_SCL;
	GPIO_Init(hard_i2c_x->__i2c_gpio->__GPIOx_SCL,&gpio_InStruct);
	gpio_InStruct.GPIO_Pin=hard_i2c_x->__i2c_gpio->__GPIO_Pin_SDA;
	GPIO_Init(hard_i2c_x->__i2c_gpio->__GPIOx_SDA,&gpio_InStruct);

	//③ 释放SDA，SCL打9拍把从机位计数器推过一个字节
	GPIO_SetBits(hard_i2c_x->__i2c_gpio->__GPIOx_SDA,hard_i2c_x->__i2c_gpio->__GPIO_Pin_SDA);
	for(uint8_t i=0;i<9;i++)
	{
		GPIO_ResetBits(hard_i2c_x->__i2c_gpio->__GPIOx_SCL,hard_i2c_x->__i2c_gpio->__GPIO_Pin_SCL);
		DelayUs(5);
		GPIO_SetBits(hard_i2c_x->__i2c_gpio->__GPIOx_SCL,hard_i2c_x->__i2c_gpio->__GPIO_Pin_SCL);
		DelayUs(5);
	}
	//④ 手动STOP：SCL高电平期间SDA上升沿
	GPIO_ResetBits(hard_i2c_x->__i2c_gpio->__GPIOx_SDA,hard_i2c_x->__i2c_gpio->__GPIO_Pin_SDA);
	DelayUs(5);
	GPIO_SetBits(hard_i2c_x->__i2c_gpio->__GPIOx_SCL,hard_i2c_x->__i2c_gpio->__GPIO_Pin_SCL);
	DelayUs(5);
	GPIO_SetBits(hard_i2c_x->__i2c_gpio->__GPIOx_SDA,hard_i2c_x->__i2c_gpio->__GPIO_Pin_SDA);
	DelayUs(5);

	//⑤ 引脚交回外设（AF_OD）+ 外设重新初始化
	hardware_i2c_init(hard_i2c_x);
}
