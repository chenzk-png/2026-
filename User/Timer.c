#include "Timer.h"
#include "debug.h"
#include "Key.h"
 u32 second_count = 0;

void TIM6_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void TIM6_IRQHandler(void)
{
    if(TIM_GetITStatus(TIM6, TIM_IT_Update) == SET)
    {
        second_count++;  // 1ms +1
        Key_Tick();
    }
    TIM_ClearITPendingBit(TIM6, TIM_IT_Update);
}

void Timer_Init(void)
{
    NVIC_InitTypeDef NVIC_InitStructure = {0};
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure = {0};

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, ENABLE);

    TIM_TimeBaseInitStructure.TIM_Period = 100 - 1;
    TIM_TimeBaseInitStructure.TIM_Prescaler = 80 - 1;
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;

    TIM_TimeBaseInit(TIM6, &TIM_TimeBaseInitStructure);

    TIM_ClearITPendingBit(TIM6, TIM_IT_Update);

    NVIC_InitStructure.NVIC_IRQChannel = TIM6_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    TIM_ITConfig(TIM6, TIM_IT_Update, ENABLE);
    TIM_Cmd(TIM6, ENABLE);  //
}

// 非阻塞延时：到达时间返回1，否则0
u8 NonBlock_Delay(DelayNonBlock_t *delay, u32 ms)
{
    if(second_count - delay->last_time >= ms)
    {
        delay->last_time = second_count;
        return 1;
    }
    return 0;
}
