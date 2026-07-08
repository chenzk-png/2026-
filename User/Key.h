#ifndef _KEY_H
#define _KEY_H

void Key_Init(void);
uint8_t Key_GetNum(void);
uint8_t Key_GetState(void);
void Key_Tick(void);
void Key_Making(void);

#endif
