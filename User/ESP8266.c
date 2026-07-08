#include "ESP8266.h"
#include "debug.h"
#include "OLED.h"
#include "Timer.h"
#include "Buzzer.h"
#include <string.h>
#include <stdio.h>

char ESP8266_RxBuf[512] = {0};
uint16_t ESP8266_RxLen = 0;
static uint8_t esp_start_flag = 0;

void ESP8266_Start1(void) 
{
    esp_start_flag = 1;
}

typedef enum {
    ESP_INIT_IDLE,
    ESP_INIT_WAIT_POWER,
    ESP_INIT_AT,
    ESP_INIT_RST,
    ESP_INIT_CWMODE,
    ESP_INIT_DHCP,
    ESP_INIT_JOIN_AP,
    ESP_INIT_MQTT_USER,
    ESP_INIT_MQTT_CONN,
    ESP_INIT_MQTT_SUB,
    ESP_INIT_DONE
} ESP_InitStage_t;

typedef enum {
    ESP_SEND_IDLE,
    ESP_SEND_TEMP,
    ESP_SEND_HUMI,
    ESP_SEND_LIGHT,
    ESP_SEND_AIR
} ESP_SendStage_t;

static ESP_InitStage_t esp_init_stage = ESP_INIT_IDLE;
static DelayNonBlock_t esp_delay;
 uint8_t net_connected_flag = 0;

static ESP_SendStage_t esp_send_stage = ESP_SEND_IDLE;
static DelayNonBlock_t esp_send_delay; 

static void USART1_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.GPIO_Pin = ESP8266_RX_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(ESP8266_GPIO, &GPIO_InitStruct);

    GPIO_InitStruct.GPIO_Pin = ESP8266_TX_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(ESP8266_GPIO, &GPIO_InitStruct);

    USART_InitTypeDef USART_InitStruct;
    USART_InitStruct.USART_BaudRate = 115200;
    USART_InitStruct.USART_WordLength = USART_WordLength_8b;
    USART_InitStruct.USART_StopBits = USART_StopBits_1;
    USART_InitStruct.USART_Parity = USART_Parity_No;
    USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStruct.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(USART1, &USART_InitStruct);

    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    NVIC_InitStruct.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

    USART_Cmd(USART1, ENABLE);
}

void ESP8266_SendString(char *str)
{
    while (*str)
    {
        USART_SendData(USART1, *str);
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
        str++;
    }
}

void ESP8266_Init(void)
{
    if (esp_start_flag == 0) return;
    // 防止定时器未初始化
    if(second_count == 0) return;

    if(esp_init_stage == ESP_INIT_IDLE)
    {
        USART1_Init();
        esp_init_stage = ESP_INIT_WAIT_POWER;
        esp_delay.last_time = second_count;
    }

    if(esp_init_stage == ESP_INIT_WAIT_POWER && NonBlock_Delay(&esp_delay,15000))
    {
        esp_init_stage = ESP_INIT_AT;
    }

    if(esp_init_stage == ESP_INIT_AT && NonBlock_Delay(&esp_delay,10000))
    {
        ESP8266_SendString("AT\r\n");
        esp_init_stage = ESP_INIT_RST;
    }

    else if(esp_init_stage == ESP_INIT_RST && NonBlock_Delay(&esp_delay,10000))
    {
        ESP8266_SendString("AT+RST\r\n");
        esp_init_stage = ESP_INIT_CWMODE;
    }

    else if(esp_init_stage == ESP_INIT_CWMODE && NonBlock_Delay(&esp_delay,35000))
    {
        ESP8266_SendString("AT+CWMODE=1\r\n");
        esp_init_stage = ESP_INIT_DHCP;
    }

    else if(esp_init_stage == ESP_INIT_DHCP && NonBlock_Delay(&esp_delay,10000))
    {
        ESP8266_SendString("AT+CWAUTOCONN=1\r\n");
        esp_init_stage = ESP_INIT_JOIN_AP;
    }

    else if(esp_init_stage == ESP_INIT_JOIN_AP && NonBlock_Delay(&esp_delay,15000))
    {
        ESP8266_SendString("AT+CWSTARTSMART=3\r\n");
        esp_init_stage = ESP_INIT_MQTT_USER;
    }

    else if(esp_init_stage == ESP_INIT_MQTT_USER && NonBlock_Delay(&esp_delay,200000))
    {
        ESP8266_SendString("AT+MQTTUSERCFG=0,1,\"temp\",\"G6oiYcsXyA\",\"version=2018-10-31&res=products%2FG6oiYcsXyA%2Fdevices%2Ftemp&et=2192672778&method=md5&sign=2BScdZ8Lz%2BPYAPV4g3czeg%3D%3D\",0,0,\"\"\r\n");
        esp_init_stage = ESP_INIT_MQTT_CONN;
    }

    else if(esp_init_stage == ESP_INIT_MQTT_CONN && NonBlock_Delay(&esp_delay,20000))
    {
        ESP8266_SendString("AT+MQTTCONN=0,\"mqtts.heclouds.com\",1883,1\r\n");
        esp_init_stage = ESP_INIT_MQTT_SUB;
    }

    else if(esp_init_stage == ESP_INIT_MQTT_SUB && NonBlock_Delay(&esp_delay,20000))
    {
        ESP8266_SendString("AT+MQTTSUB=0,\"$sys/G6oiYcsXyA/temp/thing/property/post/reply\",1\r\n");
        esp_init_stage = ESP_INIT_DONE;
        net_connected_flag = 1;
        Delay_Ms(10);
        // Buzzer_Beep();

    }
}

// ====================== 修复：循环周期上传（每3秒上传一次） ======================
void ESP8266_SendDataLoop(uint16_t t_int, uint16_t h_int, uint16_t A_int, uint16_t C_int)
{
    // 如果网络还没连上，直接返回，不执行上报
    if (net_connected_flag == 0) {
        esp_send_stage = ESP_SEND_IDLE;
        return;
    }

    // 状态机处理数据上报
    switch (esp_send_stage)
    {
        case ESP_SEND_IDLE:
            // 初始化定时器，进入第一次上报
            esp_send_delay.last_time = second_count;
            esp_send_stage = ESP_SEND_TEMP;
            break;

        case ESP_SEND_TEMP:
            if (NonBlock_Delay(&esp_send_delay, 30000)) // 等待1.5秒
            {
                char buf[256];
                sprintf(buf, "AT+MQTTPUB=0,\"$sys/G6oiYcsXyA/temp/thing/property/post\",\"{\\\"id\\\":\\\"123\\\"\\,\\\"params\\\":{\\\"temp\\\":{\\\"value\\\":%d\\}}}\",0,0\r\n", t_int);
                ESP8266_SendString(buf);
                esp_send_stage = ESP_SEND_HUMI;
            }
            break;

        case ESP_SEND_HUMI:
            if (NonBlock_Delay(&esp_send_delay, 30000)) // 等待1.5秒
            {
                char buf[256];
                sprintf(buf, "AT+MQTTPUB=0,\"$sys/G6oiYcsXyA/temp/thing/property/post\",\"{\\\"id\\\":\\\"123\\\"\\,\\\"params\\\":{\\\"humi\\\":{\\\"value\\\":%d\\}}}\",0,0\r\n", h_int);
                ESP8266_SendString(buf);
                esp_send_stage = ESP_SEND_LIGHT;
            }
            break;

        case ESP_SEND_LIGHT:
            if (NonBlock_Delay(&esp_send_delay, 30000)) // 等待1.5秒
            {
                char buf[256];
                sprintf(buf, "AT+MQTTPUB=0,\"$sys/G6oiYcsXyA/temp/thing/property/post\",\"{\\\"id\\\":\\\"123\\\"\\,\\\"params\\\":{\\\"ADValue\\\":{\\\"value\\\":%d\\}}}\",0,0\r\n", A_int);
                ESP8266_SendString(buf);
                esp_send_stage = ESP_SEND_AIR;
            }
            break;

        case ESP_SEND_AIR:
            if (NonBlock_Delay(&esp_send_delay, 30000)) // 等待1.5秒
            {
                char buf[256];
                sprintf(buf, "AT+MQTTPUB=0,\"$sys/G6oiYcsXyA/temp/thing/property/post\",\"{\\\"id\\\":\\\"123\\\"\\,\\\"params\\\":{\\\"airValue\\\":{\\\"value\\\":%d\\}}}\",0,0\r\n", C_int);
                ESP8266_SendString(buf);
                esp_send_stage = ESP_SEND_TEMP; 
            }
            break;
    }
}
void USART1_IRQHandler(void)
{
    if (USART_GetITStatus(USART1, USART_IT_RXNE) == SET)
    {
        uint8_t ch = USART_ReceiveData(USART1);
        if (ESP8266_RxLen < 511)
            ESP8266_RxBuf[ESP8266_RxLen++] = ch;
    }
}