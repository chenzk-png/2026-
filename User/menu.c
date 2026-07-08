#include "main.h"
#include "debug.h"
#include "oled_black.h"
#include "OLED_Show.h"
#include "ESP8266.h"
#include "Red.h"
#include "Buzzer.h"

u32 last_display_time_temp = 0; 
u32 last_display_time_humi = 0; 
u32 last_display_time_light = 0; 
u32 last_display_time_air = 0; 
void charge_menu(void)//电量显示函数
{
    if(charge_num == 5)
	{
		static u8 charge_flag5 = 0;//标志位，电量图标有改变的话只显示一次
		Charge_photo();
		if(charge_flag5 == 0)
		{
        OLED_Display(OLED);
		charge_flag5 = 1;
		}
	}
	if(charge_num == 4)
	{
		static u8 charge_flag4 = 0;
		Charge_photo();
		if(charge_flag4 == 0)
		{
        OLED_Display(OLED);
		charge_flag4 = 1;
		}
	}
	if(charge_num == 3)
	{
		static u8 charge_flag3 = 0;
		Charge_photo();
		if(charge_flag3 == 0)
		{
        OLED_Display(OLED);
		charge_flag3 = 1;
		}
	}
	if(charge_num == 2)
	{
		static u8 charge_flag2 = 0;
		Charge_photo();
		if(charge_flag2 == 0)
		{
        OLED_Display(OLED);
		charge_flag2 = 1;
		}
	}
	if(charge_num == 1)
	{
		static u8 charge_flag1 = 0;
		Charge_photo();
		if(charge_flag1 == 0)
		{
        OLED_Display(OLED);
		charge_flag1 = 1;
		}
	}
}

void menu0(void)
{
    OLED_Show_menu0();
    if(place_menu0!=0)//如果矩形框是第2、3、4个就删掉前面一个矩形框，达到矩形框平移的效果
    {
        OLED_DrawRectangle(15+(place_menu0-1)*70,25,80+(place_menu0-1)*70,110,WHITE,0);
    }
    if(place_menu0 == 0)//如果矩形框到了第一个就删掉第四个矩形框
    {
        OLED_DrawRectangle(225,25,290,110,WHITE,0);
    }
    if (key == 5||menu0_first == 0)//菜单标志位，只有在第一次进入该级菜单或按下key5（下一项）按钮屏幕才会刷新
    {	
        OLED_Display(OLED);//局部刷新，只刷新会改变的地方
        menu0_first = 1;
    }
}

void menu1_0(void)
{
    OLED_Show_menu1();
    if(place_menu1!=0)//也是矩形框平移
    {
        OLED_DrawRectangle(15+(place_menu1-1)*70,25,80+(place_menu1-1)*70,110,WHITE,0);
    }
    if(place_menu1 == 0)
    {
        OLED_DrawRectangle(225,25,290,110,WHITE,0);
    }
    if (key == 5||menu1_first == 0)
    {	
        OLED_Display(OLED);
        menu1_first = 1;
    }
}

void menu1_1(void)
{
    // if(net_connected_flag == 0)//未连上网络，该标志位位于ESP8266.c
    // {
    //     if(WIFI_start == 0)//未按下配网键
    //     {
    //         OLED_Show_menu1();
    //         if(WIFI_place == 0)
    //         {
    //             OLED_Display(OLED);
    //             WIFI_place = 1;
    //         }
    //     }
    //     if(WIFI_start == 1)//按下配网键
    //     {
    //         OLED_Show_menu1();
    //         if(WIFI_start_place == 0)
    //         {
    //             OLED_Display(OLED);
    //             WIFI_start_place = 1;
    //         }
    //     }
    // }
    // if(net_connected_flag == 1)//连上网络
    // {
    //     OLED_Show_menu1();
    //     if(WIFI_success_place == 0)
    //     {
    //         OLED_Display(OLED);
    //         WIFI_success_place = 1;
    //     }
    // }
}

void menu1_2(void)
{
    OLED_Show_menu1();
    if(XiaoZhi_flag == 0)
    {
        OLED_Display(OLED);
        XiaoZhi_flag = 1;
    }
}

void menu1_3(void)
{
    OLED_Show_menu1();
    if(Setting_flag == 0)
    {
        OLED_Display(OLED);
        Setting_flag = 1;
    }
    if(key == 5)
    {
        OLED_Display(OLED);
    }
}

void menu2_1_1(void)
{
   
    //显示数据的界面使用定时器大约每五秒刷新一次数据，以及第一次点进去也会刷新一次
    if (second_count - last_display_time_temp >= 50000||last_display_time_temp== 0) 
    {
    last_display_time_temp = second_count;
    if(temp >= maxTemp && buzzer_flag == 0)//超过阈值报警（需要在设置中打开该功能）
    {
        Buzzer_Beep();
    }
    OLED_Show_menu2();
    OLED_Display(OLED);
    }
    if(temp >= maxTemp&& red_flag == 0)	//大于阈值温度打开空调（需要在设置中打开该功能）
    { 
        if(temp_flag == 0)//标志位，空调指令只发一次，避免温度高一直发送开机指令
        {
            uint8_t cmd1[] = {0x68, 0x08, 0x00, 0xFF, 0x12, 0x00, 0x11, 0x16 };
            Serial_SendArray(cmd1, 8);
            temp_flag = 1;
        }
    }
    if(temp <= maxTemp - 1)//温度降下去之后将开机标志位复原以便下次工作，（maxTemp - 1）是为了避免温度在阈值附近跳动从而频繁触发开机操作
    {
        temp_flag = 0;
    }
}

void menu2_2_1(void)
{
    
    if (second_count - last_display_time_humi >= 50000||last_display_time_humi== 0) 
    {
        last_display_time_humi = second_count;
        if(humi >= maxHumi&&buzzer_flag == 0)
        {
            Buzzer_Beep();
        }
        OLED_Show_menu2();
        OLED_Display(OLED);
    }
}

void menu2_3_1(void)
{
    
    if (second_count - last_display_time_light >= 50000||last_display_time_light== 0) 
    {
        last_display_time_light = second_count;
        if(result_light > maxlight && buzzer_flag == 0)
        {
            Buzzer_Beep();
        }
        OLED_Show_menu2();
        OLED_Display(OLED);
        }
        if(result_light <= 1500 && red_flag == 0)	//晚上光照弱关闭灯光
        {
            if(light_flag1 == 0)
            {
                uint8_t cmd4[] = {0x68, 0x08, 0x00, 0xFF, 0x12, 0x03, 0x14, 0x16};
                Serial_SendArray(cmd4, 8);
                light_flag1=1;
            }		
        }
        if(result_light >= maxlight && red_flag == 0) //白天光照强打开灯光
        {
            if(light_flag2 == 0)
            {
                uint8_t cmd3[] = {0x68, 0x08, 0x00, 0xFF, 0x12, 0x02, 0x13, 0x16};
                Serial_SendArray(cmd3, 8);
                light_flag2=1;
            }
        }
}

void menu2_4_1(void)
{
   
    if (second_count - last_display_time_air >= 50000||last_display_time_air== 0) 
    {
        last_display_time_air = second_count;
        if(airValue >= maxAir&&buzzer_flag == 0)
        {
            Buzzer_Beep();
        }
        OLED_Show_menu2();
        OLED_Display(OLED);
        }
        if(airValue >= maxAir)	//打开吹风
        {
            if(air_flag == 0&& red_flag == 0)
            {
                uint8_t cmd2[] = {0x68, 0x08, 0x00, 0xFF, 0x12, 0x01, 0x12, 0x16};
                Serial_SendArray(cmd2, 8);
                air_flag = 1;
            }
        }	
        if(airValue <= maxAir - 100)
        {
            air_flag = 0;
        }
}