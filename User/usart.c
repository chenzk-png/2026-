#include "debug.h"
#include <stdio.h>
#include <stdarg.h>

char Serial_ESP_RxPacket[100];				//串口接收数据包数组，数据包格??"@MSG\r\n"
uint8_t Serial_ESP_RxFlag;					//串口接收数据包标志位

/* TX 发送循环队??
 * 主循环的 Serial_SendByte / 中断的回环都把字节塞进这??
 * TXE 中断负责从队列里取出字节写到 TDR
 * 这样两路发送共用一个通道，互不打?? */
#define TX_BUF_SIZE 256
static volatile uint8_t TxBuf[TX_BUF_SIZE];
static volatile uint16_t TxHead = 0;		//写指针：下一个要写入的位??
static volatile uint16_t TxTail = 0;		//读指针：下一个要发送的位置

/**
  * ??    数：串口初始??
  * ??    数：??
  * ?? ?? 值：??
  */
void Serial_Init(void)
{
	/*开启时??*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	/*GPIO初始??*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/*USART初始??*/
	USART_InitTypeDef USART_InitStructure;					//定义结构体变??
	USART_InitStructure.USART_BaudRate = 115200;				//波特??
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;	//硬件流控制，不需??
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;	//模式，发送模式和接收模式都选择
	USART_InitStructure.USART_Parity = USART_Parity_No;		//奇偶校验，不需??
	USART_InitStructure.USART_StopBits = USART_StopBits_1;	//停止位，选择1??
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;		//字长，选择8??
	USART_Init(USART1, &USART_InitStructure);				//把结构体变量交给USART_Init函数，配置USART1

	/*中断使能配置*/
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);			//开启串口接收数据的中断

	// /*NVIC中断分组*/
	// NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);			//设置NVIC为分??2

	/*NVIC配置*/
	NVIC_InitTypeDef NVIC_InitStructure;					//定义结构体变??
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;		//选择配置NVIC的USART1??
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//指定NVIC路线使能
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;		//指定NVIC路线的抢占优先级??1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;		//指定NVIC路线的响应优先级??1
	NVIC_Init(&NVIC_InitStructure);							//把结构体变量交给NVIC_Init函数，配置NVIC

	/*USART使能*/
	USART_Cmd(USART1, ENABLE);								//使能USART1，串口开始运??

}

/**
  * ??    数：串口发送一个字节（通过 TX 队列，非阻塞??
  * ??    数：Byte 要发送的一个字??
  * ?? ?? 值：??
  */
void Serial_SendByte(uint8_t Byte)
{
	uint16_t next_head = (TxHead + 1) % TX_BUF_SIZE;
	if (next_head == TxTail) return;		//队列已满，丢弃（256 字节足够大，正常不会发生??

	TxBuf[TxHead] = Byte;
	TxHead = next_head;

	USART_ITConfig(USART1, USART_IT_TXE, ENABLE);	//唤醒 TXE 中断，让它把队列里的字节搬出??
}

/**
  * ??    数：串口发送一个数??
  * ??    数：Array 要发送数组的首地址
  * ??    数：Length 要发送数组的长度
  * ?? ?? 值：??
  */
void Serial_SendArray(uint8_t *Array, uint16_t Length)
{
	uint16_t i;
	for (i = 0; i < Length; i ++)		//遍历数组
	{
		Serial_SendByte(Array[i]);		//依次调用Serial_SendByte发送每个字节数??
	}
}

/**
  * ??    数：串口发送一个字符串
  * ??    数：String 要发送字符串的首地址
  * ?? ?? 值：??
  */
void Serial_ESP_SendString(char *String)
{
	uint8_t i;
	for (i = 0; String[i] != '\0'; i ++)//遍历字符串数组（字符串以'\0'为结束标志位，故遇到'\0'时停止）
	{
		Serial_SendByte(String[i]);		//依次调用Serial_SendByte发送每个字节数??
	}
}

/**
  * ??    数：次方函数（内部使用）
  * ?? ?? 值：返回值等于X的Y次方
  */
uint32_t Serial_Pow(uint32_t X, uint32_t Y)
{
	uint32_t Result = 1;	//设置结果初值为1
	while (Y --)			//执行Y??
	{
		Result *= X;		//把X累乘到结??
	}
	return Result;
}

/**
  * ??    数：串口发送数??
  * ??    数：Number 要发送的数字，范围：0~4294967295
  * ??    数：Length 要发送数字的长度，范围：0~10
  * ?? ?? 值：??
  */
void Serial_ESP_SendNumber(uint32_t Number, uint8_t Length)
{
	uint8_t i;
	for (i = 0; i < Length; i ++)		//根据数字的长度遍历数字的每一??
	{
		Serial_SendByte(Number / Serial_Pow(10, Length - i - 1) % 10 + '0');	//依次调用Serial_SendByte发送每位数??
	}
}

/**
  * ??    数：使用printf需要重定向的底层函??
  * ??    数：保持原始格式即可，无需变动
  * ?? ?? 值：保持原始格式即可，无需变动
  */
int fputc(int ch, FILE *f)
{
	Serial_SendByte(ch);			//将printf的底层重定向为自己的发送字节函??
	return ch;
}

/**
  * ??    数：自己封装的prinf函数
  * ??    数：format 格式字符??
  * ??    数：... 可变的参数列??
  * ?? ?? 值：??
  */
void Serial_Printf(char *format, ...)
{
	char String[100];				//定义字符串数??
	va_list arg;					//定义可变参数列表数据类型变量arg
	va_start(arg, format);			//从format开始接收参数列表到arg变量
	vsprintf(String, format, arg);	//使用vsprintf打印格式化字符串和参数列表到字符串数??
	va_end(arg);					//结束变量arg
	Serial_ESP_SendString(String);		//串口发送字符串数组（字符串??
}

 u8 Serial_ESP_RecvByte(void)
  {
      while (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == RESET);
      return (u8)USART_ReceiveData(USART1);
  }


/**
  * ??    数：USART1中断函数
  * ??    数：??
  * ?? ?? 值：??
  * 注意事项：此函数为中断函数，无需调用，中断触发后自动执行
  *           函数名为预留指定名称，所以放在此处即??
  *           确保中断名称正确，不能有丝毫差别，否则中断函数不能进??
  */
void USART1_IRQHandler(void)
{
	static uint8_t RxState = 0;		//用于表示当前状态机状态的静态变??
	static uint8_t pRxPacket = 0;	//用于表示当前接收数据位置的静态变??
	if (USART_GetITStatus(USART1, USART_IT_RXNE) == SET)	//判断是否是USART1的接收事件产生的中断
	{
		uint8_t RxData = USART_ReceiveData(USART1);			//读取数据寄存器，存放到接收到的数据变??

		/* 把每个收到的字节塞进 TX 队列里回发（回环??
		 * Serial_SendByte 内部会自动开?? TXE 中断
		 * 主循环里 Serial_SendString 发的字节也走同一个队??
		 * 两路发送自动按到达顺序排队，不会互相覆?? */
		Serial_SendByte(RxData);

		/*使用状态机思路，依次处理数据包的不同部??*/

		/*当前状态为0，接收数据包包头*/
		if (RxState == 0)
		{
			if (RxData == '@' && Serial_ESP_RxFlag == 0)		//如果收到确实是包头，并且上一个数据包已处理完??
			{
				RxState = 1;			//进入下一个状??
				pRxPacket = 0;			//数据包位置归??
			}
		}
		/*当前状态为1，接收数据包数据，同时判断是否接收到了第一个包??*/
		else if (RxState == 1)
		{
			if (RxData == '\r')			//如果收到第一个包??
			{
				RxState = 2;			//进入下一个状??
			}
			else						//收到的还是数据包正文
			{
				Serial_ESP_RxPacket[pRxPacket] = RxData;		//把数据存放到数据包数组指定位??
				pRxPacket ++;			//数据包位置自??
			}
		}
		/*当前状态为2，接收数据包第二个包??*/
		else if (RxState == 2)
		{
			if (RxData == '\n')			//如果收到第二个包??
			{
				RxState = 0;			//状态归0
				Serial_ESP_RxPacket[pRxPacket] = '\0';			//将收到的字符串数据包加一个字符串结束标志
				Serial_ESP_RxFlag = 1;		//数据包标志位??1，成功接收一个数据包
			}
		}

		USART_ClearITPendingBit(USART1, USART_IT_RXNE);		//清除标志??
	}

	/* TXE 中断：从 TX 队列里取出下一个字节写?? TDR
	 * 主循环的 Serial_SendByte ?? RXNE 的回环都往队列里塞字节
	 * 队列空了就把 TXE 中断关掉，下次有字节进来再开 */
	if (USART_GetITStatus(USART1, USART_IT_TXE) == SET)
	{
		if (TxTail != TxHead)							//队列里还有字节没发完
		{
			USART_SendData(USART1, TxBuf[TxTail]);
			TxTail = (TxTail + 1) % TX_BUF_SIZE;
		}
		else											//队列空了
		{
			USART_ITConfig(USART1, USART_IT_TXE, DISABLE);	//关闭 TXE 中断，省??
		}
	}
}
void Serial_SendFormatted(float temp, uint8_t humi) {
    // ?? snprintf 拼字符串
    char line[32];
    int len = snprintf(line, sizeof(line), "!T%.1f,H%d\n", temp, humi);
    // 一字节一字节??
    for (int i = 0; i < len; i++) {
        Serial_SendByte((uint8_t)line[i]);
    }
}

/* 串口发送一个字?? */
void USART1_SendByte(uint8_t b)
{
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    USART_SendData(USART1, b);
}