/**
 ******************************************************************************
 * @file    adc.c
 * @brief   ADC 驱动：电池电压采样
 ******************************************************************************
 */

#include "adc.h"
#include "adc_desc.h"
#include <stddef.h>
#include <string.h>
static volatile float vbat=0;

/**
 * @brief  初始化 ADC：配置通道、注入转换、中断和校准
 * @param  adc_x  ADC 句柄
 * @retval None
 */
void adc_init(adc_handler adc_x)
{
	if(adc_x==NULL||adc_x->adc_base==NULL||adc_x->adc_injected==NULL)
	{
		return;
	}
	ADC_InitTypeDef adc_inStructer;
	ADC_StructInit(&adc_inStructer);
	adc_inStructer.ADC_ContinuousConvMode=adc_x->adc_base->__ADC_ContinuousConvMode;
	adc_inStructer.ADC_DataAlign=adc_x->adc_base->__ADC_DataAlign;
	adc_inStructer.ADC_ExternalTrigConv=adc_x->adc_base->__ADC_ExternalTrigConv;
	adc_inStructer.ADC_Mode=adc_x->adc_base->__ADC_Mode;
	adc_inStructer.ADC_NbrOfChannel=adc_x->adc_base->__ADC_NbrOfChannel;
	adc_inStructer.ADC_ScanConvMode=adc_x->adc_base->__ADC_ScanConvMode;
	ADC_Init(adc_x->__adc_x,&adc_inStructer);
	
	GPIO_InitTypeDef GPIO_InStructer;
	GPIO_StructInit(&GPIO_InStructer);
	GPIO_InStructer.GPIO_Mode=adc_x->__GPIO_Mode;
	GPIO_InStructer.GPIO_Pin=adc_x->__GPIO_Pin;
	GPIO_InStructer.GPIO_Speed=adc_x->__GPIO_Speed;
	GPIO_Init(adc_x->__GPIO_X,&GPIO_InStructer);
	
	if(adc_x->__InjectedConvCmd)
	{
		if(adc_x->adc_injected->__is_use_trig){
		ADC_ExternalTrigInjectedConvConfig(adc_x->__adc_x,
			adc_x->adc_injected->__ADC_ExternalTrigInjecConv);
		ADC_ExternalTrigInjectedConvCmd(adc_x->__adc_x,ENABLE);
		}
		ADC_InjectedChannelConfig(adc_x->__adc_x
		,adc_x->adc_injected->__ADC_Channel,adc_x->adc_injected->__Rank,
		adc_x->adc_injected->__ADC_SampleTime);
	}
	else 
	{
		if(adc_x->adc_base->__is_use_trig)
		{
			ADC_ExternalTrigConvCmd(adc_x->__adc_x,ENABLE);
		}
		ADC_RegularChannelConfig(adc_x->__adc_x
		,adc_x->adc_base->__ADC_Channel,adc_x->adc_base->__Rank,
		adc_x->adc_base->__ADC_SampleTime);
	}
	if(adc_x->__is_eoc||adc_x->__is_jeoc)
	{
		NVIC_InitTypeDef nvic_inStructer;
		memset(&nvic_inStructer,0,sizeof(NVIC_InitTypeDef));
		nvic_inStructer.NVIC_IRQChannel=ADC1_2_IRQn;
		nvic_inStructer.NVIC_IRQChannelCmd=ENABLE;
		nvic_inStructer.NVIC_IRQChannelPreemptionPriority=0;
		nvic_inStructer.NVIC_IRQChannelSubPriority=0;
		NVIC_Init(&nvic_inStructer);
		if(adc_x->__is_eoc)
		{
			ADC_ITConfig(adc_x->__adc_x,ADC_IT_EOC,ENABLE);
		}
		else if(adc_x->__is_jeoc)
		{
			ADC_ITConfig(adc_x->__adc_x,ADC_IT_JEOC,ENABLE);
		}
	}
	
	ADC_Cmd(adc_x->__adc_x,ENABLE);
	ADC_ResetCalibration(adc_x->__adc_x);
	while(ADC_GetResetCalibrationStatus(adc_x->__adc_x)==SET);
	ADC_StartCalibration(adc_x->__adc_x);
	while(ADC_GetCalibrationStatus(adc_x->__adc_x)==SET);
	
}

/**
 * @brief  ADC 注入转换中断服务函数：计算电池电压
 * @retval None
 */
void ADC1_2_IRQHandler(void)
{
	if(ADC_GetITStatus(ADC1,ADC_IT_JEOC)==SET)
	{
		uint16_t result=ADC_GetInjectedConversionValue(ADC1
		,ADC_InjectedChannel_1);
		ADC_ClearITPendingBit(ADC1,ADC_IT_JEOC);
		vbat=result/4095.0f*3.3/3.3f*8.4f;
	}
}

/**
 * @brief  获取电池电压
 * @param  adc_x  ADC 句柄
 * @retval 电池电压(V)
 */
float vbat_get(adc_handler adc_x)
{
	if(!adc_x)
	{
		return -1;
	}
	return vbat;
}
