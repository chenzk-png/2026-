#include "debug.h"
#include "AD_air.h"
void AD_air_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC2, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;  // 模拟输入
    GPIO_InitStructure.GPIO_Pin = AIR_PIN;
    GPIO_Init(AIR_GPIO, &GPIO_InitStructure);
    
    // 规则通道配置
    ADC_RegularChannelConfig(ADC2, ADC_Channel_14, 1, ADC_SampleTime_55Cycles5);
    
    ADC_InitTypeDef ADC_InitStructure;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_NbrOfChannel = 1;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_Init(ADC2, &ADC_InitStructure);
    
    ADC_Cmd(ADC2, ENABLE);
    
    ADC_ResetCalibration(ADC2);
    while(ADC_GetResetCalibrationStatus(ADC2) == SET);
    ADC_StartCalibration(ADC2);
    while(ADC_GetCalibrationStatus(ADC2) == SET);
}

uint16_t AD_air_GetValue(void)
{
    ADC_SoftwareStartConvCmd(ADC2, ENABLE);
    while(ADC_GetFlagStatus(ADC2, ADC_FLAG_EOC) == RESET);
    return ADC_GetConversionValue(ADC2);
}
