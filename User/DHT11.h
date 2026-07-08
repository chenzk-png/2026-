#ifndef __DHT11_H
#define __DHT11_H

#include "debug.h"

#define TEMP_PIN GPIO_Pin_7
#define TEMP_GPIO GPIOA

u8 DHT11_Init(void);
u8 DHT11_Read_Data(u8 *temp, u8 *humi);
void DHT11_Rst(void);
u8 DHT11_Check(void);
u8 DHT11_Read_Bit(void);
u8 DHT11_Read_Byte(void);

#endif
