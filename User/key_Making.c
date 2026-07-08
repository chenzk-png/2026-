#include "debug.h"
#include "main.h"
#include "oled_black.h" 
#include "Timer.h" 
#include "menu.h"
#include "ESP8266.h"

void Key_Making(void)
{
	if(key == 1&&(menu == 1||menu == 0))//确定键
	{
		menu++;
		OLED_Clear(WHITE);
		OLED_Display(OLED);
		if(menu == 1||menu == 2)//确定键在的三级菜单无效
		{
			menu0_first = 0;//将菜单标志位清零
		}
		if(menu == 0||menu == 2)
		{
			menu1_first =0;//将菜单标志位清零
		}
	}
	if(key == 2)//全屏刷新键，因前面用到的都是局部刷新，但局部刷新用久了会有残影，全局刷新能刷干净
	{
		OLED_Display_All(OLED);
	}
	if(key == 3&&(menu == 2 || menu == 1))//返回键
	{
		menu--;
		WIFI_place = 0;
		WIFI_success_place = 1;
		XiaoZhi_flag = 0;
		Setting_flag = 0 ;//将WIFI界面、小智界面、设置界面标志位复原
		OLED_Clear(WHITE);//全屏清除
		OLED_Display(OLED);
		if(menu == 0)
		{
			menu1_first = 0;//将菜单标志位清零
		}
	}
	if(key == 4 )//阈值键
	{
		if(place_menu1 == 0 && menu == 2)
		{
			maxTemp=maxTemp+2;
			if(maxTemp>36)
			{
				maxTemp=28;
			}
			OLED_ShowNum(160,70,maxTemp,2,24,BLACK);
			OLED_Display(OLED);
			last_display_time_temp = second_count ;//按下阈值键会刷新屏幕更新时间
		}
		if(place_menu1 == 1 && menu == 2)
		{
			maxHumi=maxHumi+2;
			if(maxHumi>90)
			{
				maxHumi=70;
			}
			OLED_ShowNum(160,70,maxHumi,2,24,BLACK);
			OLED_Display(OLED);
			last_display_time_humi = second_count ;
			
		}
		if(place_menu1 == 2 && menu == 2)
		{
			maxlight=maxlight-100;
			if(maxlight<300)
			{
				maxlight=800;
			}
			OLED_ShowNum(160,70,maxlight,5,24,BLACK);
			OLED_Display(OLED);
			last_display_time_light = second_count ;
			
		}
		if(place_menu1 == 3 && menu == 2)
		{
			maxAir=maxAir+200;
			if(maxAir>2000)
			{
				maxAir=800;
			}
			OLED_ShowNum(220,70,maxAir,5,24,BLACK);
			OLED_Display(OLED);
			last_display_time_air = second_count ;
		}
		if(place_menu0 == 3 && menu == 1)//按下阈值键改变工作状态
		{
			if(place_menu_Setting == 0)
			{
            buzzer_flag = buzzer_flag ^1;//0跟1状态取反
			}
			if(place_menu_Setting == 1)
			{
            red_flag = red_flag ^1;
			}
			Setting_flag = 0;
			
		}
	}
	if(key == 6 && menu == 1 && place_menu0 == 1 && WIFI_place == 1)//配网键
	{
		// ESP8266_Start1();
		WIFI_start = 1;
		OLED_Clear(WHITE);
	}
	if(key == 5)//下一项
	{
		if(menu == 0)
		{
			place_menu0++;//一级菜单矩形框的位置
			if(place_menu0>3)
			{
				place_menu0 = 0;
			}
			
		}
		if(menu == 1 && place_menu0 == 0)
		{
			place_menu1++;//二级菜单矩形框的位置
			if(place_menu1>3)
			{
				place_menu1 = 0;
			}
			
		}
		if(menu == 1 && place_menu0 == 3)
		{
			place_menu_Setting++;//设置界面里面矩形框的位置
			if(place_menu_Setting>1)
			{
				place_menu_Setting = 0;
			}
		}
	}
}