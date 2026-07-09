#ifndef CH32_RX_H
#define CH32_RX_H

#include <cstdint>
#include <esp_err.h>

/**
 * @brief CH32 串口通信模块
 *
 * 协议（双向共用）：
 *   每帧一行：  "humi:XXtemp:YY\r\n"
 *   XX / YY 是 2 位十进制数（前导 0，例如 humi=5 → "05"）
 *   与 CH32 端 Serial_SendNumber(humi, 2) 完全对齐
 *
 * 完整流程：
 *   1. UartComm::Init()          →  打开 UART（CH32 ↔ ESP32）
 *   2. ch32_rx_start()            →  启动后台接收任务
 *   3. ch32_rx_get_latest(...)    →  读取 CH32 发来的最新温湿度
 *   4. ch32_rx_send(temp, humi)   →  ESP32 回传（用同一种格式）
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 启动后台接收任务（一秒内反复从 UART 抓数据并解析）
 */
esp_err_t ch32_rx_start(uint32_t stack_size = 2048);

/** @brief 停止后台任务 */
void ch32_rx_stop(void);

/**
 * @brief 读取最近一次成功解析的温湿度
 * @param temp  输出温度（°C），可为 nullptr
 * @param humi  输出湿度（%），可为 nullptr
 * @return true = 有新数据；false = 还没收到任何一帧
 */
bool ch32_rx_get_latest(int* temp, int* humi);

/**
 * @brief ESP32 → CH32：发送温湿度（格式 humi:XXtemp:YY\r\n 与 CH32 完全一致）
 *
 * 用 %02d 让 humi=5 发成 "05"，与 Serial_SendNumber(humi, 2) 对齐。
 */
esp_err_t ch32_rx_send(int temp, int humi);

/**
 * @brief ESP32 → CH32：发送任意一行（需自己保证格式和 \r\n 结尾）
 *
 * 例：ch32_rx_send_raw("ack\r\n");
 */
esp_err_t ch32_rx_send_raw(const char* line);

/**
 * @brief ESP32 → CH32：发送显示更新命令
 *        协议格式："TYPE:text\r\n"
 *
 * 用于把 ESP32 的 OLED 状态同步到 CH32 控制的屏幕上：
 *   type 可以是 "STATUS" / "EMOTION" / "CHAT" / "NOTIFY"
 *
 * 例：
 *   ch32_rx_send_command("STATUS", "待命中");      →  发 "STATUS:待命中\r\n"
 *   ch32_rx_send_command("EMOTION", "happy");       →  发 "EMOTION:happy\r\n"
 *   ch32_rx_send_command("CHAT", "user:你好");      →  发 "CHAT:user:你好\r\n"
 *   ch32_rx_send_command("NOTIFY", "扫描 Wi-Fi...");  →  发 "NOTIFY:扫描 Wi-Fi...\r\n"
 */
esp_err_t ch32_rx_send_command(const char* type, const char* text);

#ifdef __cplusplus
}
#endif

#endif  // CH32_RX_H