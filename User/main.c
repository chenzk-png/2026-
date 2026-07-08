#include "debug.h"
#include "oled_black.h"  //墨水屏
#include "DHT11.h"
#include "Key.h"
#include "AD_light.h"
#include "AD_air.h"
// #include "ESP8266.h"
#include "Timer.h"
// #include "Red.h"
#include "Buzzer.h"
#include "OLED_Show.h"
#include "flash.h"
#include "menu.h"
#include "usart.h"
#include "key_Making.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

u8 temp;      
u8 humi; 
u16 light;     
u16 air;
u8 humi_data;
u8 temp_data;
uint16_t ADValue,airValue,result_light;
float Voltage;
uint16_t maxTemp = 28,
maxHumi = 70,
maxlight=800,
maxAir=800;
uint8_t place_menu0=0,place_menu1=0,place_menu_Setting=0,menu = 0;
u8  OLED[4736];
u8 key;
u32 time;
u8 menu1_first = 0,menu0_first = 0;
u8 WIFI_place = 0,WIFI_start = 0,WIFI_start_place = 0,WIFI_success_place = 0,
XiaoZhi_flag = 0,
Setting_flag = 0;
u8 temp_flag = 0,air_flag = 0,light_flag1 = 0,light_flag2 = 0;
u8 buzzer_flag = 1,red_flag = 1;
u32 charge_num=5;
u8 power_flag=0;

int main(void)
{
	//电源拉高PB9锁住通电
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    GPIO_SetBits(GPIOB, GPIO_Pin_9);

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	SystemCoreClockUpdate();
    Delay_Init();
	Timer_Init();  
	DHT11_Init();
	Key_Init();	
	AD_light_Init();
	AD_air_Init();
	// Red_Init();
    Buzzer_Init();
	Serial_Init();
	OLED_GUIInit();
    Paint_NewImage(OLED,OLED_W,OLED_H,0,WHITE); 
    OLED_Clear(WHITE);	
	OLED_GUIInit();
    GPIO_ResetBits(GPIOA, GPIO_Pin_15);
	// charge_num= Read_Data(); 
	// charge_num++; 
    // Save_Data(charge_num); //保存到flash函数，不要放到while里面
  while (1)
  {
	if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_0) == 1)//开机键松手后标志位置1
	{
	 	power_flag = 1;
	}
	if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_0) == 0 && power_flag == 1)//关机
	{	
	 	GPIO_ResetBits(GPIOB, GPIO_Pin_9);
	}	
	   
	// ESP8266_Init();	
	key=Key_GetNum();
	Key_Making();
	DHT11_Read_Data(&temp,&humi);
	humi_data = humi;
	temp_data = temp;
	static u32 usart_timer = 0;
	if(second_count - usart_timer > 10000)//串口向ESP32发送数据以及接收数据
	{
		usart_timer = second_count;
		Serial_ESP_SendString("humi:");
		Serial_ESP_SendNumber(humi_data,2);
		Serial_ESP_SendString("temp:");
		Serial_ESP_SendNumber(temp_data,2);
		Serial_ESP_SendString("\r\n");
	}
	static u32 charge_timer = 0;
	if(second_count - charge_timer >18000000)//电量大约每隔25分钟下降一格
	{
		charge_timer = second_count;
        charge_num--;
	}
	charge_menu();//电池图标显示
	
	//以下是菜单显示
	if(menu == 0)
	{
		menu0();
	}
	if(menu == 1)
	{
		if(place_menu0 == 0)//环境监测
		{
		    menu1_0();
		}	
		if(place_menu0 == 1)//联网
		{
			menu1_1();
		}
		if(place_menu0 == 2)//小智
		{
			menu1_2();
		}
		if(place_menu0 == 3)//设置
		{
		    menu1_3();
		}
	}
	if(menu == 2)
	{
		if(place_menu0 == 0&&place_menu1 == 0)//温度
		{
			menu2_1_1();
		}
		if(place_menu0 == 0&&place_menu1 == 1)//湿度
		{
			menu2_2_1();
		}		
		if(place_menu0 == 0&&place_menu1 == 2)//光照
		{
			menu2_3_1();
		}	
		if(place_menu0 == 0&&place_menu1 == 3)//烟雾浓度
		{
			menu2_4_1();
		}
	}
	// ESP8266_SendDataLoop(temp, humi, ADValue, airValue);//发送数据至onenet平台
    }	
}
