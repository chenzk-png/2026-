#ifndef _AD_AIR_H
#define _AD_AIR_H

#include "debug.h"

#define AIR_PIN GPIO_Pin_4
#define AIR_GPIO GPIOC

void AD_air_Init(void);
uint16_t AD_air_GetValue(void);

#endif