#ifndef __SERIAL_H
#define __SERIAL_H


#define RED_TX_PIN GPIO_Pin_10
#define RED_RX_PIN GPIO_Pin_11
#define RED_GPIO GPIOB

void Red_Init(void);
void Serial_SendByte(uint8_t Byte);
void Serial_SendArray(uint8_t *Array,uint16_t Length);
void Serial_SendString(char *String);
void Serial_SendNumber(uint32_t Number,uint16_t Length);
uint8_t Serial_GetRxFlag(void);
 void Serial_SendPacket(void);
 extern uint8_t Serial_TxPacket[];
 void USART3_IRQHandler(void);
 extern uint8_t Serial_RxPacket[];
#endif
