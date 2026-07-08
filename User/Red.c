#include "debug.h"                  // Device header
#include "Red.h"   
uint8_t Serial_TxPacket[8];
uint8_t Serial_RxPacket[4];
uint8_t Serial_RxFlag;

void Red_Init(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
 	// TX引脚是 PB10
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = RED_TX_PIN;
 	GPIO_Init(RED_GPIO, &GPIO_InitStructure);
	
	// RX引脚是 PB11
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; 
	GPIO_InitStructure.GPIO_Pin = RED_RX_PIN;
 	GPIO_Init(RED_GPIO, &GPIO_InitStructure);
	
	USART_InitTypeDef USART_InitStruture;
	USART_InitStruture.USART_BaudRate = 115200;
	USART_InitStruture.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStruture.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
	USART_InitStruture.USART_Parity = USART_Parity_No;
	USART_InitStruture.USART_StopBits = USART_StopBits_1;
	USART_InitStruture.USART_WordLength = USART_WordLength_8b;
	USART_Init(USART3, &USART_InitStruture); 
	
	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE); 
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_Init(&NVIC_InitStructure);
	
	USART_Cmd(USART3, ENABLE);
}

void Serial_SendByte(uint8_t Byte)
{
	USART_SendData(USART3, Byte); 
	while (USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
}

void Serial_SendArray(uint8_t *Array, uint16_t Length)
{
	 uint16_t i;
	 for(i = 0; i < Length; i++)
	 {
		 Serial_SendByte(Array[i]);
	 }
 }

 void Serial_SendString(char *String)
 {
	 uint8_t i;
	 for(i = 0; String[i] != '\0'; i++)
	 {
		 Serial_SendByte(String[i]);
	 }
 }
 
 uint32_t Serial_Pow(uint32_t x, uint32_t y)
 {
	 uint32_t Result = 1;
	 while(y--)
	 {
		 Result *= x;
	 }
	 return Result;
 }

 void Serial_SendNumber(uint32_t Number, uint16_t Length)
 {
	 uint8_t i;
	 for(i = 0; i < Length; i++)
	 {
		 Serial_SendByte(Number / Serial_Pow(10, Length - 1 - i) % 10 + '0');
	 }
 }   /*以上为模块函数*/
 
 void Serial_SendPacket(void)
 {
	 Serial_SendByte(0xFF);
	 Serial_SendArray(Serial_TxPacket, 8);
	 Serial_SendByte(0xFE);
 }/*发送模块*/
 
 uint8_t Serial_GetRxFlag(void)
 {
	 if(Serial_RxFlag == 1)
	 {
		 Serial_RxFlag = 0;
		 return 1;
	 }
	 return 0;
 }
 

 void USART3_IRQHandler(void)
 {
	 static uint8_t S = 0;
	 static uint8_t pRxPacket = 0;
	 if(USART_GetITStatus(USART3, USART_IT_RXNE) == SET)
	 {
		 uint8_t RxData = USART_ReceiveData(USART3); // 修改11：接收数据的对象改为 USART3
		 if(S == 0)
		 {
			 if(RxData == 0xFF)
			 {
				 S = 1;
				 pRxPacket = 0;
			 }
		 }
		 else if(S == 1)
		 {
			 Serial_RxPacket[pRxPacket] = RxData;
			 pRxPacket++;
			 if(pRxPacket >= 4)
			 {
				 S = 2;
			 }
		 }
		 else if(S == 2)
		 {
			 if(RxData == 0xFE)
			 {
				 S = 0;
				 Serial_RxFlag = 1;
			 }
		 }
		 USART_ClearITPendingBit(USART3, USART_IT_RXNE); 
	 }/*接收模块*/
 }

 