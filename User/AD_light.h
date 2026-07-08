#ifndef _AD_LIGHT_H
#define _AD_LIGHT_H

#include "debug.h"

#define LIGHT_PIN GPIO_Pin_1
#define LIGHT_GPIO GPIOB

void AD_light_Init(void);
uint16_t AD_light_GetValue(void);

#endif