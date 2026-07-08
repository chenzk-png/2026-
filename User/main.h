#ifndef __MAIN_H
#define __MAIN_H

#include "debug.h"

extern uint8_t place_menu0,place_menu1,menu;
extern u8 menu1_first ,menu0_first ;
extern u8 temp,humi;
extern uint16_t ADValue,airValue,result_light;
int main(void);
extern u8 key;
extern u8 menu1_first ,menu0_first ;
extern u8 WIFI_place ,WIFI_start ,WIFI_start_place ,WIFI_success_place, XiaoZhi_flag ,Setting_flag  ;
extern u8  OLED[4736];
extern uint16_t maxTemp ,maxHumi ,maxlight,maxAir;
extern u8 buzzer_flag ,red_flag,place_menu_Setting;
extern u32 charge_num;
extern u8 temp_flag ,air_flag ,light_flag1 ,light_flag2 ;

#endif