#ifndef __ESP8266_H
#define __ESP8266_H

#include "debug.h"
#include "Timer.h"

#define ESP8266_RX_PIN GPIO_Pin_9
#define ESP8266_GPIO GPIOA
#define ESP8266_TX_PIN GPIO_Pin_10


void ESP8266_Init(void);
void ESP8266_Start1(void);
void ESP8266_SendString(char *str);
void ESP8266_SendDataLoop(uint16_t t_int, uint16_t h_int, uint16_t A_int, uint16_t C_int);
u8 NonBlock_Delay(DelayNonBlock_t *delay, u32 ms);
 extern uint8_t net_connected_flag ;
#endif
