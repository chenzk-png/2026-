#include "DHT11.h"
#include "debug.h"

// PA7 输出模式
void DHT11_IO_OUT(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = TEMP_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(TEMP_GPIO, &GPIO_InitStructure);
}

// PA7 上拉输入模式
void DHT11_IO_IN(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = TEMP_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(TEMP_GPIO, &GPIO_InitStructure);
}

// 复位 DHT11
void DHT11_Rst(void)
{
    DHT11_IO_OUT();
    GPIO_ResetBits(TEMP_GPIO, TEMP_PIN);
    Delay_Ms(20);
    GPIO_SetBits(TEMP_GPIO, TEMP_PIN);
    Delay_Us(30);
}

// 检测 DHT11 响应
u8 DHT11_Check(void)
{
    u8 retry = 0;
    DHT11_IO_IN();

    while (GPIO_ReadInputDataBit(TEMP_GPIO, TEMP_PIN) && retry < 100)
    {
        retry++;
        Delay_Us(1);
    }
    if (retry >= 100) return 1;

    retry = 0;
    while (!GPIO_ReadInputDataBit(TEMP_GPIO, TEMP_PIN) && retry < 100)
    {
        retry++;
        Delay_Us(1);
    }
    if (retry >= 100) return 1;
    return 0;
}

// 读取一个位
u8 DHT11_Read_Bit(void)
{
    u8 retry = 0;
    while (GPIO_ReadInputDataBit(TEMP_GPIO, TEMP_PIN) && retry < 100)
    {
        retry++;
        Delay_Us(1);
    }

    retry = 0;
    while (!GPIO_ReadInputDataBit(TEMP_GPIO, TEMP_PIN) && retry < 100)
    {
        retry++;
        Delay_Us(1);
    }

    Delay_Us(30); // CH32 必须用 30us，否则读 00
    return GPIO_ReadInputDataBit(TEMP_GPIO, TEMP_PIN) ? 1 : 0;
}

// 读取一个字节
u8 DHT11_Read_Byte(void)
{
    u8 i, dat;
    dat = 0;
    for (i = 0; i < 8; i++)
    {
        dat <<= 1;
        dat |= DHT11_Read_Bit();
    }
    return dat;
}

// 读取温湿度
u8 DHT11_Read_Data(u8 *temp, u8 *humi)
{
    u8 buf[5];
    u8 i;
    DHT11_Rst();
    if (DHT11_Check() == 0)
    {
        for (i = 0; i < 5; i++)
        {
            buf[i] = DHT11_Read_Byte();
        }
        if ((buf[0] + buf[1] + buf[2] + buf[3]) == buf[4])
        {
            *humi = buf[0];
            *temp = buf[2];
        }
    }
    else return 1;
    return 0;
}

// 初始化
u8 DHT11_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitStructure.GPIO_Pin = TEMP_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(TEMP_GPIO, &GPIO_InitStructure);

    GPIO_SetBits(TEMP_GPIO, TEMP_PIN);

    DHT11_Rst();
    return DHT11_Check();
}
