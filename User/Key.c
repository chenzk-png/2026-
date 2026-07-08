#include "debug.h"
#include "Timer.h" 
#include "ESP8266.h" 
static uint8_t Key_Num;

void Key_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
}


uint8_t Key_GetState(void)
{
	if(GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_0) == 0)
	{
		return 1;
	}
	if(GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_2) == 0)
	{
		return 2;
	}
	if(GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_1) == 0)
	{
		return 3;
	}
	if(GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_3) == 0)
	{
		return 4;
	}
	if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_12) == 0)
	{
		return 5;
	}
	if(GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_4) == 0)
	{
		return 6;
	}
	if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_0) == 0)
	{
		return 7;
	}
	
	return 0;
}

void Key_Tick(void)//×´Ì¬Ïû¶¶
{
	static uint8_t count_key;
	static uint8_t State_current,State_previous;
	count_key++;
	if(count_key>=20)
	{
		count_key = 0;
		State_previous = State_current;
		State_current = Key_GetState();
		if(State_current == 0&&State_previous != 0)
		{
			Key_Num = State_previous;
		}
	}
}
uint8_t Key_GetNum(void)
{
	uint8_t X;
	if(Key_Num)
	{
		X = Key_Num;
		Key_Num = 0;
		return X;
	}
	return 0;
}


