// =====================================================================
//  uart_comm.cc —— UartComm 类的实现（C++ 源文件）
//
//  本文件实现 uart_comm.h 声明的 UartComm 类，提供 5 个 public 静态方法：
//      Init / Deinit / IsInitialized / Send (×2 重载) / Receive
//  整个类是"无状态"设计（全部 static）。
// =====================================================================

#include "uart_comm.h"

#include <cstdio>
#include <cstring>

#include <freertos/FreeRTOS.h>
#include <driver/uart.h>
#include <esp_log.h>

#define TAG "UartComm"

// 静态成员类外唯一定义（必须有，否则链接报 undefined reference）
bool UartComm::initialized_ = false;

// ---------------------------------------------------------------------
//  Init —— 初始化 UART（幂等）
// ---------------------------------------------------------------------
esp_err_t UartComm::Init(int baud_rate, gpio_num_t tx_pin, gpio_num_t rx_pin) {
    if (initialized_) {
        return ESP_OK;                             // 已初始化直接返回（幂等保护）
    }

    // ---------- 步骤 1：配置串口参数 ----------
    // ⚠️ 关键：字段顺序必须和 ESP-IDF 的 uart_config_t 声明顺序一致！
    //    顺序：baud_rate → data_bits → parity → stop_bits → flow_ctrl
    //          → rx_flow_ctrl_thresh → source_clk
    uart_config_t cfg = {};
    cfg.baud_rate              = baud_rate;
    cfg.data_bits              = kDataBits;
    cfg.parity                 = kParity;
    cfg.stop_bits              = kStopBits;
    cfg.flow_ctrl              = kFlowCtrl;
    cfg.rx_flow_ctrl_thresh   = 0;            // 不接硬件流控时填 0
    cfg.source_clk             = UART_SCLK_DEFAULT;

    esp_err_t ret = uart_param_config(kUartPort, &cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "uart_param_config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // ---------- 步骤 2：设置 TX / RX 引脚 ----------
    ret = uart_set_pin(kUartPort, tx_pin, rx_pin,
                       UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "uart_set_pin failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // ---------- 步骤 3：装驱动 ----------
    ret = uart_driver_install(kUartPort, kTxBufSize, kRxBufSize,
                              0, nullptr, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "uart_driver_install failed: %s", esp_err_to_name(ret));
        return ret;
    }

    initialized_ = true;
    ESP_LOGI(TAG, "UART initialized: port=UART%d TX=%d RX=%d baud=%d",
             (int)kUartPort, (int)tx_pin, (int)rx_pin, baud_rate);
    return ESP_OK;
}

// ---------------------------------------------------------------------
//  Deinit —— 反初始化（幂等）
// ---------------------------------------------------------------------
void UartComm::Deinit() {
    if (!initialized_) {
        return;
    }
    uart_driver_delete(kUartPort);              // 释放 DMA 缓冲 + 注销 ISR + 释放 GPIO
    initialized_ = false;
    ESP_LOGI(TAG, "UART deinitialized");
}

// ---------------------------------------------------------------------
//  IsInitialized —— 状态查询
// ---------------------------------------------------------------------
bool UartComm::IsInitialized() {
    return initialized_;
}

// ---------------------------------------------------------------------
//  Send —— 阻塞发送二进制数据
// ---------------------------------------------------------------------
int UartComm::Send(const void* data, size_t len, uint32_t timeout_ms) {
    if (!initialized_ || data == nullptr || len == 0) {
        return -1;
    }
    int written = uart_write_bytes(kUartPort, data, len);
    if (written < 0) {
        ESP_LOGE(TAG, "uart_write_bytes failed: %d", written);
        return -1;
    }
    // 等最后一个字节从 FIFO 真正发出去
    // ★ 关键修复：之前用 timeout_ms（默认 100ms），导致 I2C 总线被抢断
    //   → SSD1306 通信丢失 → OLED 反复刷新 / 翻面
    // 改成最多等 5ms，DMA 早发完了，几乎立即返回
    esp_err_t ret = uart_wait_tx_done(kUartPort, pdMS_TO_TICKS(5));
    if (ret != ESP_OK) {
        // 5ms 还发不完就放弃不等（数据已经在 FIFO/DMA 里，迟早会发完）
    }
    return written;
}

// ---------------------------------------------------------------------
//  Send(const char*) —— 便捷重载
// ---------------------------------------------------------------------
int UartComm::Send(const char* str) {
    if (str == nullptr) return -1;
    return Send(str, strlen(str));
}

// ---------------------------------------------------------------------
//  Receive —— 阻塞接收原始字节
// ---------------------------------------------------------------------
int UartComm::Receive(void* buf, size_t buf_size, uint32_t timeout_ms) {
    if (!initialized_ || buf == nullptr || buf_size == 0) {
        return -1;
    }
    int n = uart_read_bytes(kUartPort, buf, buf_size, pdMS_TO_TICKS(timeout_ms));
    return n;                                    // >=0: 实际字节数；<0: 错误
}

// =====================================================================
//  三个面向"串口助手"的便捷发送函数实现
// ---------------------------------------------------------------------
//  这三个函数都生成"人可读"的文本输出（末尾自动追加 \r\n），
//  适合在串口助手的"文本显示"模式下观察变量 / 数组内容。
// =====================================================================

// 辅助函数：把 1 个字节转成 2 位大写十六进制字符（如 0xAB → "AB"）
static inline void ByteToHex(uint8_t b, char* out2) {
    static const char* kHex = "0123456789ABCDEF";
    out2[0] = kHex[(b >> 4) & 0x0F];
    out2[1] = kHex[b & 0x0F];
}

// ---------------------------------------------------------------------
//  SendString —— 发送字符串（自动追加 "\r\n"）
// ---------------------------------------------------------------------
//  用法：UartComm::SendString("Hello World");
//  串口助手看到：Hello World\r\n
//
//  实现说明：
//   - 短字符串（< 254 字节）直接拼成 [str + "\r\n"] 一次发完，效率高。
//   - 长字符串拆成两次发送（先发原文，再发 "\r\n"），避免栈上开太大。
int UartComm::SendString(const char* str) {
    if (str == nullptr) return -1;
    size_t n = strlen(str);
    constexpr size_t kStackBuf = 256;             // 栈缓冲上限
    char buf[kStackBuf];
    if (n + 2 <= sizeof(buf)) {
        memcpy(buf, str, n);
        buf[n]     = '\r';
        buf[n + 1] = '\n';
        return Send(buf, n + 2);
    }
    // 超长：分两次发
    int w1 = Send(str, n);
    int w2 = Send("\r\n", 2);
    return (w1 < 0 || w2 < 0) ? -1 : (w1 + w2);
}

// ---------------------------------------------------------------------
//  SendArray —— 发送字节数组（按 HEX 文本显示，自动追加 "\r\n"）
// ---------------------------------------------------------------------
//  用法：uint8_t buf[] = {0xAA, 0x55, 0x00, 0xFF};
//        UartComm::SendArray(buf, sizeof(buf));
//  串口助手看到：AA 55 00 FF\r\n
//
//  格式约定：每字节 2 位大写十六进制，字节之间用 1 个空格分隔。
//  单次最多处理 256 字节（防栈溢出），超出自动截断前 256 字节。
int UartComm::SendArray(const uint8_t* arr, size_t len) {
    if (arr == nullptr || len == 0) return -1;
    constexpr size_t kMaxBytes = 256;            // 单次最多显示 256 字节
    if (len > kMaxBytes) len = kMaxBytes;
    // 输出缓冲：每字节 3 字符（"XX "） + 末尾 \r\n + \0 余量
    char buf[kMaxBytes * 3 + 4];
    size_t pos = 0;
    for (size_t i = 0; i < len; ++i) {
        ByteToHex(arr[i], &buf[pos]);
        pos += 2;
        if (i + 1 < len) {
            buf[pos++] = ' ';                     // 字节间空格分隔
        }
    }
    buf[pos++] = '\r';
    buf[pos++] = '\n';
    return Send(buf, pos);
}

// ---------------------------------------------------------------------
//  SendNumber(int) —— 发送整型数值（自动追加 "\r\n"）
// ---------------------------------------------------------------------
//  用法：UartComm::SendNumber(123);
//  串口助手看到：123\r\n
int UartComm::SendNumber(int value) {
    char buf[16];                                 // int 最多 "-2147483648" (11) + "\r\n" (2) + 余量
    int n = snprintf(buf, sizeof(buf), "%d\r\n", value);
    if (n <= 0) return -1;
    if ((size_t)n > sizeof(buf)) n = sizeof(buf);
    return Send(buf, (size_t)n);
}

// ---------------------------------------------------------------------
//  SendNumber(float, decimals) —— 发送浮点数值（自动追加 "\r\n"）
// ---------------------------------------------------------------------
//  用法：float v = 3.75f; UartComm::SendNumber(v, 2);
//  串口助手看到：3.75\r\n
int UartComm::SendNumber(float value, int decimals) {
    if (decimals < 0) decimals = 0;
    if (decimals > 6) decimals = 6;               // 夹紧到 0~6 位小数
    char fmt[8];
    snprintf(fmt, sizeof(fmt), "%%.%df\r\n", decimals);
    char buf[32];                                 // 浮点最长 ≈ -3.141593 + \r\n，32 字节足够
    int n = snprintf(buf, sizeof(buf), fmt, (double)value);
    if (n <= 0) return -1;
    if ((size_t)n > sizeof(buf)) n = sizeof(buf);
    return Send(buf, (size_t)n);
}