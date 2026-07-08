#include "debug.h"
#include "oled_black.h"  //墨水屏
#include "DHT11.h"
#include "BMP.h"
#include "Key.h"
#include "main.h"
#include "ESP8266.h"
#include "AD_light.h"
#include "AD_air.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

void Charge_photo(void)//电池容量变化图标
{
    if(charge_num == 5)//五格电
    {
    OLED_ShowPicture(250,1,40,20,gImage_26,WHITE);
    }
    if(charge_num == 4)//四格电
    {
    OLED_ShowPicture(250,1,40,20,gImage_27,WHITE);
    }
    if(charge_num == 3)
    {
    OLED_ShowPicture(250,1,40,20,gImage_28,WHITE);
    }
    if(charge_num == 2)
    {
    OLED_ShowPicture(250,1,40,20,gImage_29,WHITE);
    }
    if(charge_num == 1)
    {
    OLED_ShowPicture(250,1,40,20,gImage_30,WHITE);
    }
}
void OLED_Show_menu0(void)//一级菜单显示界面，包括文字、图标
{
    OLED_ShowChinese(20,30,12,24,BLACK);
	OLED_ShowChinese(50,30,13,24,BLACK);
    OLED_ShowPicture(27,60,40,40,gImage_8,WHITE);
	OLED_ShowChinese(90,30,14,24,BLACK);
	OLED_ShowChinese(120,30,15,24,BLACK);
    OLED_ShowPicture(97,55,48,48,gImage_5,WHITE);
	OLED_ShowChinese(160,30,16,24,BLACK);
	OLED_ShowChinese(190,30,17,24,BLACK);
    OLED_ShowPicture(167,60,48,48,gImage_9,WHITE);
	OLED_ShowChinese(230,30,18,24,BLACK);
	OLED_ShowChinese(260,30,19,24,BLACK);
	OLED_ShowPicture(237,60,48,48,gImage_7,WHITE);
    OLED_ShowPicture(250,1,40,20,gImage_26,WHITE);
	OLED_DrawRectangle(15+place_menu0*70,25,80+place_menu0*70,110,BLACK,0);
}
void OLED_Show_menu1(void)//二级菜单显示界面，包括文字、图标
{
    if(place_menu0 == 0)
    {
        OLED_ShowChinese(20,30,0,24,BLACK);
        OLED_ShowChinese(50,30,1,24,BLACK);
        OLED_ShowChinese(90,30,2,24,BLACK);
        OLED_ShowChinese(120,30,1,24,BLACK);
        OLED_ShowChinese(160,30,3,24,BLACK);
        OLED_ShowChinese(190,30,4,24,BLACK);
        OLED_ShowChinese(230,30,5,24,BLACK);
        OLED_ShowChinese(260,30,6,24,BLACK);
        OLED_ShowPicture(167,60,40,40,gImage_1,WHITE);	
        OLED_ShowPicture(97,60,40,40,gImage_2,WHITE);
        OLED_ShowPicture(237,60,40,40,gImage_3,WHITE);
        OLED_ShowPicture(27,60,40,40,gImage_4,WHITE);
        OLED_DrawRectangle(15+place_menu1*70,25,80+place_menu1*70,110,BLACK,0);//矩形框移动显示
    }	
	if(place_menu0 == 1 )
    {
        // if(net_connected_flag == 0 )
        // {
        //     if(WIFI_start == 0)//WIFI未连接时显示的文字
        //     {
        //         OLED_ShowChinese(20,30,20,24,BLACK);
        //         OLED_ShowChinese(50,30,21,24,BLACK);
        //         OLED_ShowChinese(80,30,22,24,BLACK);
        //         OLED_ShowChinese(110,30,23,24,BLACK);
        //         OLED_ShowChinese(140,30,24,24,BLACK);
        //         OLED_ShowChinese(170,30,25,24,BLACK);

        //         OLED_ShowChinese(20,70,26,24,BLACK);
        //         OLED_ShowChinese(50,70,27,24,BLACK);
        //         OLED_ShowChinese(80,70,28,24,BLACK);
        //         OLED_ShowChinese(110,70,29,24,BLACK);
        //         OLED_ShowChinese(140,70,30,24,BLACK);
        //         OLED_ShowChinese(170,70,31,24,BLACK);
        //         OLED_ShowChinese(200,70,32,24,BLACK);
        //         OLED_ShowChinese(230,70,28,24,BLACK);
        //         OLED_ShowChinese(260,70,29,24,BLACK);
        //     }
        //     if(WIFI_start == 1)//WIFI连接时显示的文字
        //     {
        //         OLED_ShowChinese(20,30,28,24,BLACK);
        //         OLED_ShowChinese(50,30,29,24,BLACK);
        //         OLED_ShowChinese(80,30,31,24,BLACK);
        //         OLED_ShowChinese(110,30,32,24,BLACK);
        //         OLED_ShowChinese(140,30,33,24,BLACK);
        //         OLED_ShowString(170,30,". . .",24,BLACK);
        //     }
        // }
        // if(net_connected_flag == 1 )//连接成功的画面
        // {
        //     OLED_ShowChinese(20,30,28,24,BLACK);
        //     OLED_ShowChinese(50,30,29,24,BLACK);
        //     OLED_ShowChinese(80,30,45,24,BLACK);
        //     OLED_ShowChinese(110,30,34,24,BLACK);
        //     OLED_ShowChinese(140,30,35,24,BLACK);
        //     OLED_ShowString(170,30,"      ",24,BLACK);
        // }
    }
    if(place_menu0 == 2)//小智
    {
        OLED_ShowChinese(50,30,21,24,BLACK);
        OLED_ShowChinese(80,30,22,24,BLACK);
        OLED_ShowChinese(110,30,23,24,BLACK);
        OLED_ShowChinese(140,30,24,24,BLACK);
        OLED_ShowChinese(170,30,25,24,BLACK);
    }
     if(place_menu0 == 3)//设置界面
    {
        OLED_ShowChinese(20,30,36,24,BLACK);
        OLED_ShowChinese(50,30,37,24,BLACK);
        OLED_ShowChinese(80,30,38,24,BLACK);
        OLED_ShowChinese(110,30,39,24,BLACK);
        OLED_ShowChinese(140,30,40,24,BLACK);
        OLED_ShowString(170,30,(u8*)":",24,BLACK);
       
        OLED_ShowChinese(20,70,46,24,BLACK);
        OLED_ShowChinese(50,70,47,24,BLACK);
        OLED_ShowChinese(80,70,39,24,BLACK);
        OLED_ShowChinese(110,70,40,24,BLACK);
        OLED_ShowString(140,70,(u8*)":",24,BLACK);
        
        if(place_menu_Setting == 0)//设置蜂鸣器工作状态
        {
            OLED_DrawRectangle(15,25,260,60,BLACK,0);
            OLED_DrawRectangle(15,65,260,100,WHITE,0);
        }
         if(place_menu_Setting == 1)//设置红外模块工作状态
        {
            OLED_DrawRectangle(15,65,260,100,BLACK,0);
            OLED_DrawRectangle(15,25,260,60,WHITE,0);
        }

        if(buzzer_flag == 0)//0为关闭
        {
            OLED_ShowChinese(200,30,41,24,BLACK);
            OLED_ShowChinese(230,30,42,24,BLACK);
        }
        if(buzzer_flag == 1)//1为打开
        {
            OLED_ShowChinese(200,30,43,24,BLACK);
            OLED_ShowChinese(230,30,44,24,BLACK);
        }
         if(red_flag == 0)
        {
            OLED_ShowChinese(170,70,41,24,BLACK);
            OLED_ShowChinese(200,70,42,24,BLACK);
        }
        if(red_flag == 1)
        {
            OLED_ShowChinese(170,70,43,24,BLACK);
            OLED_ShowChinese(200,70,44,24,BLACK);
        }
    }	
}
void OLED_Show_menu2(void)//三级菜单显示界面，包括文字、图标
{
    if(place_menu0 == 0)
    {
        if(place_menu1 == 0)//温度
        {
            OLED_ShowChinese(10,10,10,24,BLACK);
            OLED_ShowChinese(40,10,11,24,BLACK); 
            OLED_ShowChinese(70,10,0,24,BLACK);
            OLED_ShowChinese(100,10,1,24,BLACK); 
            OLED_ShowString(130,10,(u8*)":",24,BLACK);

            OLED_ShowChinese(10,50,8,24,BLACK);
            OLED_ShowChinese(40,50,9,24,BLACK); 
            OLED_ShowChinese(70,50,0,24,BLACK);
            OLED_ShowChinese(100,50,1,24,BLACK); 
            OLED_ShowString(130,50,(u8*)":",24,BLACK);

            DHT11_Read_Data(&temp, &humi);
            OLED_ShowNum(150,10,temp,2,24,BLACK);
            OLED_ShowNum(150,50,maxTemp,2,24,BLACK);

            if(temp <= 20)//温度计图标的动态变化
            {
                OLED_ShowPicture(200,25,60,60,gImage_10,WHITE);
            }
            if(temp <= 23 && temp > 20)
            {
                OLED_ShowPicture(200,25,60,60,gImage_20,WHITE);
            }
             if(temp <= 26 && temp > 23)
            {
                OLED_ShowPicture(200,25,60,60,gImage_21,WHITE);
            }
             if(temp <= 29 && temp > 26)
            {
                OLED_ShowPicture(200,25,60,60,gImage_22,WHITE);
            }
             if(temp <= 32&& temp > 29)
            {
                OLED_ShowPicture(200,25,60,60,gImage_23,WHITE);
            }
            if(temp <= 35&& temp > 32)
            {
                OLED_ShowPicture(200,25,60,60,gImage_24,WHITE);
            }
             if(temp > 35)
            {
                OLED_ShowPicture(200,25,60,60,gImage_25,WHITE);
            }

            
        }
        if(place_menu1 == 1)
        {
            OLED_ShowChinese(10,10,10,24,BLACK);
            OLED_ShowChinese(40,10,11,24,BLACK); 
            OLED_ShowChinese(70,10,2,24,BLACK);
            OLED_ShowChinese(100,10,1,24,BLACK); 
            OLED_ShowString(130,10,(u8*)":",24,BLACK);

            OLED_ShowChinese(10,50,8,24,BLACK);
            OLED_ShowChinese(40,50,9,24,BLACK); 
            OLED_ShowChinese(70,50,2,24,BLACK);
            OLED_ShowChinese(100,50,1,24,BLACK); 
            OLED_ShowString(130,50,(u8*)":",24,BLACK);

            DHT11_Read_Data(&temp, &humi);
            OLED_ShowNum(150,10,humi,2,24,BLACK);
            OLED_ShowNum(150,50,maxHumi,2,24,BLACK);
 
            if(humi <= 40)//湿度图标的动态变化
            {
               OLED_ShowPicture(200,25,64,64,gImage_11,WHITE);
            }
            if(humi > 40 && humi <= 80)
            {
               OLED_ShowPicture(200,25,64,64,gImage_12,WHITE);
            }
            if(humi > 80)
            {
               OLED_ShowPicture(200,25,64,64,gImage_13,WHITE);
            }
        }
        if(place_menu1 == 2)
        {
            OLED_ShowChinese(10,10,10,24,BLACK);
            OLED_ShowChinese(40,10,11,24,BLACK); 
            OLED_ShowChinese(70,10,3,24,BLACK);
            OLED_ShowChinese(100,10,4,24,BLACK); 
            OLED_ShowString(130,10,(u8*)":",24,BLACK);

            OLED_ShowChinese(10,50,8,24,BLACK);
            OLED_ShowChinese(40,50,9,24,BLACK); 
            OLED_ShowChinese(70,50,3,24,BLACK);
            OLED_ShowChinese(100,50,4,24,BLACK); 
            OLED_ShowString(130,50,(u8*)":",24,BLACK);

            ADValue = AD_light_GetValue();
            result_light = 4095 - ADValue;
            OLED_ShowNum(150,10,result_light,4,24,BLACK);
            OLED_ShowNum(150,50,maxlight,4,24,BLACK);

            if(ADValue <= 500)//光照图标的动态变化
            {
                OLED_ShowPicture(212,20,64,64,gImage_14,WHITE);
            }
            if(ADValue <= 2000 && ADValue > 500)
            {
                OLED_ShowPicture(212,20,64,64,gImage_15,WHITE);
            }
            if(ADValue <= 3000 && ADValue > 2000)
            {
                OLED_ShowPicture(212,20,64,64,gImage_16,WHITE);
            }
            if(ADValue > 3000)
            {
                OLED_ShowPicture(212,20,64,64,gImage_17,WHITE);
            }
            
        }
        if(place_menu1 == 3)
        {
            OLED_ShowChinese(10,10,10,24,BLACK);
            OLED_ShowChinese(40,10,11,24,BLACK); 
            OLED_ShowChinese(70,10,5,24,BLACK);
            OLED_ShowChinese(100,10,6,24,BLACK); 
            OLED_ShowString(130,10,(u8*)":",24,BLACK);

            OLED_ShowChinese(10,50,8,24,BLACK);
            OLED_ShowChinese(40,50,9,24,BLACK); 
            OLED_ShowChinese(70,50,5,24,BLACK);
            OLED_ShowChinese(100,50,6,24,BLACK); 
            OLED_ShowString(130,50,(u8*)":",24,BLACK);

            airValue = AD_air_GetValue();
            OLED_ShowNum(150,10,airValue,4,24,BLACK);
            OLED_ShowNum(150,50,maxAir,4,24,BLACK);

            if(airValue < 2000)//烟雾图标动态变化
            {
                OLED_ShowPicture(212,20,64,64,gImage_18,WHITE);
            }
            if(airValue >= 2000)
            {
                OLED_ShowPicture(212,20,64,64,gImage_19,WHITE);
            }
        }
    }
}