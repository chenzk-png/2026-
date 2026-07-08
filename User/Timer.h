#ifndef __TIMER_H
#define __TIMER_H

// #include "ch32v30x.h"
#include "debug.h"

extern  u32 second_count;  // 1ms 自增
void Timer_Init(void);

// 非阻塞延时结构体
typedef struct {
    u32 last_time;
    u32 interval;
} DelayNonBlock_t;

// 非阻塞延时判断函数
u8 NonBlock_Delay(DelayNonBlock_t *delay, u32 ms);

#endif