#ifndef __BUZZER_H
#define __BUZZER_H

#include "debug.h"

#define BUZZER_PIN GPIO_Pin_8
#define BUZZER_GPIO GPIOA

void Buzzer_Init(void);
void Buzzer_Beep(void);
void Buzzer_On(void);
void Buzzer_Off(void);

#endif
