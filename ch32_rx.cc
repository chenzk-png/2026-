// =====================================================================
//  ch32_rx.cc —— CH32 ↔ ESP32 双向串口通信
//
//  接收：后台任务逐字节累积，遇到 '\n' 解析 "humi:XXtemp:YY\r\n"
//  发送：按相同格式通过 UartComm::Send 发回给 CH32
//
//  依赖：uart_comm.h 里的 UartComm 类（已配置 GPIO39/40, 115200, 8N1）
// =====================================================================

#include "ch32_rx.h"
#include "uart_comm.h"

#include <atomic>
#include <cstdio>
#include <cstring>
#include <vector>

#include <esp_log.h>
#include <freertos/FreeRTOS.h>

#define TAG "Ch32Rx"

namespace {

// -------------------- 全局状态 --------------------

struct Ch32Data {
    int      temp    = 0;
    int      humi    = 0;
    bool     valid   = false;       // 是否至少成功解析过一帧
    uint32_t last_ms = 0;          // 上次解析成功的时间戳
};

Ch32Data           g_data;         // 全局最新数据
std::atomic<bool>  g_stop{false};  // 任务退出标志
TaskHandle_t       g_task = nullptr;

// -------------------- 接收：行解析 --------------------

/**
 * @brief 解析一行：期望格式 "humi:XXtemp:YY"
 *
 * @return true = 解析成功
 */
bool parse_line(const char* line, int len, Ch32Data& out) {
    if (line == nullptr || len <= 0) return false;

    int h = 0, t = 0;
    // 格式与 CH32 端完全一致：humi:%dtemp:%d
    // 注意：sscanf 会跳过空白，数字前导 0 也接受（"05" → 5）
    int n = sscanf(line, "humi:%dtemp:%d", &h, &t);
    if (n != 2) {
        ESP_LOGW(TAG, "parse fail: '%.*s'", len, line);
        return false;
    }

    out.humi = h;
    out.temp = t;
    return true;
}

/**
 * @brief 后台任务入口：逐字节累积 → 满一行解析一次
 */
void task_entry(void* arg) {
    std::vector<char> line;
    line.reserve(64);                       // 预分配，避免频繁 malloc

    ESP_LOGI(TAG, "task started");

    while (!g_stop.load()) {
        char c;
        // 阻塞收 1 字节（200ms 超时，让任务能响应 stop_flag）
        int n = UartComm::Receive(&c, 1, 200);
        if (n != 1) continue;

        if (c == '\n') {                    // 行结束标记
            // 去尾部 \r（CH32 端发的是 "\r\n"）
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            // 解析
            Ch32Data tmp;
            if (parse_line(line.data(), (int)line.size(), tmp)) {
                tmp.last_ms    = xTaskGetTickCount() * portTICK_PERIOD_MS;
                g_data         = tmp;
                g_data.valid   = true;
                ESP_LOGI(TAG, "[CH32 OK] temp=%d humi=%d", tmp.temp, tmp.humi);
            } else {
                // ★ 调试用：打印解析失败的原始内容（最多 64 字节）
                ESP_LOGW(TAG, "parse fail: '%.*s'", (int)line.size(), line.data());
            }
            line.clear();                    // 重置，准备下一行
        } else {
            line.push_back(c);
            // 异常长（如果丢 \n 时）→ 清掉重来
            if (line.size() > 64) {
                ESP_LOGW(TAG, "line too long (%d bytes), reset: '%.*s'",
                         (int)line.size(), (int)line.size(), line.data());
                line.clear();
            }
        }
    }

    ESP_LOGI(TAG, "task exiting");
    vTaskDelete(nullptr);                    // 自删（安全）
}

// ====================================================
//  对外 C API（用 extern "C" 包装，C/C++ 都能调）
// ====================================================

extern "C" {

esp_err_t ch32_rx_start(uint32_t stack_size) {
    if (g_task != nullptr) return ESP_OK;    // 重复启动安全

    g_stop.store(false);
    BaseType_t ok = xTaskCreate(task_entry, "ch32_rx",
                                 stack_size, nullptr, 1, &g_task);
    return (ok == pdPASS) ? ESP_OK : ESP_FAIL;
}

void ch32_rx_stop(void) {
    g_stop.store(true);
}

bool ch32_rx_get_latest(int* temp, int* humi) {
    if (!g_data.valid) return false;
    if (temp) *temp = g_data.temp;
    if (humi) *humi = g_data.humi;
    return true;
}

// -------------------- 发送：跟 CH32 同一种格式 --------------------

/**
 * @brief 把温湿度用 CH32 看得懂的格式发回去
 *
 * 关键：用 %02d 让 "5" 输出 "05"，跟 Serial_SendNumber(humi, 2) 对齐
 * 否则 humi=5 会发成 "humi:5temp:23"，CH32 端 sscanf 解析会拿到 52 或失败
 */
esp_err_t ch32_rx_send(int temp, int humi) {
    char buf[32];
    // 关键字段：%02d = 至少 2 位数字，前导 0 补齐
    // 与 CH32 的 Serial_SendNumber(humi, 2) 完全对齐
    int n = snprintf(buf, sizeof(buf), "@humi:%02dtemp:%02d\r\n", humi, temp);
    if (n <= 0 || n >= (int)sizeof(buf)) {
        return ESP_ERR_INVALID_ARG;
    }

    int sent = UartComm::Send(buf, (size_t)n);
    if (sent < 0) {
        ESP_LOGE(TAG, "send fail: %d", sent);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "→ CH32: %.*s", n - 2, buf);   // 打印去掉 \r\n 的内容
    return ESP_OK;
}

/**
 * @brief 通用字符串发送（需自己保证格式）
 */
esp_err_t ch32_rx_send_raw(const char* line) {
    if (line == nullptr) return ESP_ERR_INVALID_ARG;

    int sent = UartComm::Send(line, strlen(line));
    if (sent < 0) return ESP_FAIL;
    return ESP_OK;
}

/**
 * @brief 发"TYPE:text\r\n"格式的显示更新命令
 *        CH32 端解析后显示到自己的屏幕上
 */
esp_err_t ch32_rx_send_command(const char* type, const char* text) {
    if (type == nullptr || text == nullptr) return ESP_ERR_INVALID_ARG;

    char buf[128];
    int n = snprintf(buf, sizeof(buf), "%s:%s\r\n", type, text);
    if (n <= 0 || n >= (int)sizeof(buf)) return ESP_ERR_INVALID_ARG;

    int sent = UartComm::Send(buf, (size_t)n);
    if (sent < 0) {
        ESP_LOGE(TAG, "send command fail: %d", sent);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "-> CH32: %s:%s", type, text);
    return ESP_OK;
}

}  // extern "C"

}  // namespace