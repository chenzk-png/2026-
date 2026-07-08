#include "debug.h"
#include "Buzzer.h"

/****************************************
 函 数 名：Buzzer_Init
 功    能：蜂鸣器引脚初始化 PA8
 ****************************************/
void Buzzer_Init(void)
{
    // 开 GPIOB 时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 推挽输出
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pin = BUZZER_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    
    GPIO_Init(BUZZER_GPIO, &GPIO_InitStructure);
    
    // 初始关闭蜂鸣器
    GPIO_SetBits(BUZZER_GPIO, BUZZER_PIN);
}

/****************************************
 函 数 名：Buzzer_Beep
 功    能：蜂鸣器响一下
 ****************************************/
void Buzzer_Beep(void)
{
    GPIO_ResetBits(BUZZER_GPIO, BUZZER_PIN);    // 响
    Delay_Ms(20);  
    GPIO_SetBits(BUZZER_GPIO, BUZZER_PIN);      // 停
    Delay_Ms(40);   
    GPIO_ResetBits(BUZZER_GPIO, BUZZER_PIN);    // 响
    Delay_Ms(20); 
    GPIO_SetBits(BUZZER_GPIO, BUZZER_PIN);      // 停
    Delay_Ms(40);                        
}

