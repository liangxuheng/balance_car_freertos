#ifndef __ADC_DESC_H_
#define __ADC_DESC_H_
#include "stm32f10x.h"
#include <stdint.h>
#include <stdbool.h>
struct adc_base_struct{
	//adc的工作模式
	uint32_t __ADC_Mode;
	//adc是否开启扫描模式
	FunctionalState __ADC_ScanConvMode;
	//adc是否开启连续转换
	FunctionalState __ADC_ContinuousConvMode;
	//规则组的外部触发信号
	uint32_t __ADC_ExternalTrigConv;
	//adc转换的结果存在寄存器的对齐方式
	uint32_t __ADC_DataAlign;
	//规则组的转换通道的数量
	uint8_t __ADC_NbrOfChannel;
	uint8_t __ADC_Channel;
	uint8_t __Rank; 
	uint8_t __ADC_SampleTime;
	bool __is_use_trig;
};

struct adc_injected_struct{
	//adc注入组的信号触发来源
	uint32_t __ADC_ExternalTrigInjecConv;
	//是否使用触发信号
	bool __is_use_trig; 
	uint8_t __ADC_Channel;
	uint8_t __Rank; 
	uint8_t __ADC_SampleTime;

};

struct adc_struct{
	struct adc_base_struct*adc_base;
	ADC_TypeDef*__adc_x;
	//是否使用注入通道
	bool __InjectedConvCmd;
	struct adc_injected_struct*adc_injected;
	GPIO_TypeDef*__GPIO_X;
	uint16_t __GPIO_Pin;
	GPIOSpeed_TypeDef __GPIO_Speed;
	GPIOMode_TypeDef __GPIO_Mode;
	bool __is_jeoc;
	bool __is_eoc;
};
#endif
