// =====================================================================
//  serial.h —— UART 串口配置头文件
//
//  本文件声明 UartComm 类，只做"引脚 + 参数 + 驱动安装"配置。
//  不包含发送 / 接收函数（调用方直接用 ESP-IDF 的 uart_write_bytes /
//  uart_read_bytes，端口常量见 kUartPort）。
//
//  默认配置：
//    - 端口：UART_NUM_2（避开默认 console 用的 UART_NUM_0）
//    - 引脚：TX = GPIO39,  RX = GPIO40
//    - 参数：115200 波特 / 8 数据位 / 无校验 / 1 停止位 / 无硬件流控
//    - 缓冲：发送 1024B、接收 1024B（DMA 内部搬运）
//
//  用法：
//   #include "serial.h"
//   UartComm::Init();                                 // 用默认参数
//   uart_write_bytes(UartComm::kUartPort, "hi\r\n", 4);  // 直接发
//   uart_read_bytes (UartComm::kUartPort, buf, n, pdMS_TO_TICKS(1000));
// =====================================================================

// 头文件保护宏：防止同一个 .h 被多次 include 导致类重复定义
// 命名用 UART_COMM_H（虽然文件名是 serial.h，但 include guard 用
// uart_comm 命名以兼容之前可能存在的引用，避免冲突）
#ifndef UART_COMM_H
#define UART_COMM_H

// -------------------- 依赖头文件 --------------------

// ESP-IDF 标准错误码（esp_err_t / ESP_OK / ESP_FAIL 等）
#include <esp_err.h>

// UART 驱动 API：uart_param_config / uart_set_pin / uart_driver_install 等
#include <driver/uart.h>

// gpio_num_t 枚举类型（GPIO_NUM_39 / GPIO_NUM_40 等宏定义在这里）
#include <soc/gpio_num.h>

// -------------------- 类声明 --------------------

// UartComm：纯静态方法的"无状态"类（所有成员都是 static constexpr）
// 设计意图：调用方不创建对象，直接 UartComm::Init() 即可。
// 优点：单例模式零开销、不用关心生命周期、不用考虑多实例。
class UartComm {
public:
    // ====== 默认引脚与端口常量 ======
    // 用 static constexpr 而非 #define：
    //   - 有类型（编译期类型检查）
    //   - 在类作用域里，命名更清晰（UartComm::kUartPort）
    //   - 可以取地址（虽然这里不需要）

    // UART 端口号：UART_NUM_2 避开默认 console（UART_NUM_0）
    // ESP32-S3 共有 UART0/1/2 三路：
    //   - UART0：默认 console（烧录口），用来看 log
    //   - UART1：常被 PSRAM 占用或保留
    //   - UART2：完全可用，我们用这个
    static constexpr uart_port_t kUartPort = UART_NUM_2;

    // TX 引脚：GPIO39（在 ESP32-S3-WROOM-1/N16R8 上属于安全 GPIO）
    // 不在 flash（33-37）/ PSRAM（26-32）/ USB（19-20）/ strapping（0/3/45/46）范围
    static constexpr gpio_num_t  kTxPin    = GPIO_NUM_39;

    // RX 引脚：GPIO40（同上，安全）
    static constexpr gpio_num_t  kRxPin    = GPIO_NUM_40;

    // ====== 默认串口参数常量 ======

    // 波特率 115200：常用值，CH32/Arduino/PC 串口助手都默认支持
    static constexpr int                   kBaudRate  = 115200;

    // 发送 DMA 缓冲区 1024 字节
    // 调用 uart_write_bytes 时数据先复制到这里，再由 DMA 搬到 UART 外设
    // 越大越不容易丢数据，但占内存（每个字节都是堆分配）
    static constexpr int                   kTxBufSize = 1024;

    // 接收 DMA 缓冲区 1024 字节
    // UART 收到的数据先堆到这里，uart_read_bytes 从这里拷出去
    static constexpr int                   kRxBufSize = 1024;

    // 数据位 8：标准 ASCII / 二进制都用 8 位
    static constexpr uart_word_length_t    kDataBits  = UART_DATA_8_BITS;

    // 校验位 NONE：无校验（最简单也最常用）
    // 其他选项：UART_PARITY_EVEN / UART_PARITY_ODD
    static constexpr uart_parity_t         kParity    = UART_PARITY_DISABLE;

    // 停止位 1：标准配置
    static constexpr uart_stop_bits_t      kStopBits  = UART_STOP_BITS_1;

    // 硬件流控 DISABLE：不接 RTS/CTS 引脚
    // 如果要接流控，改成 UART_HW_FLOWCTRL_CTS_RTS
    static constexpr uart_hw_flowcontrol_t kFlowCtrl  = UART_HW_FLOWCTRL_DISABLE;

    // ====== 方法声明 ======

    /**
     * @brief 初始化 UART 外设、引脚与 DMA 缓冲区
     *
     * 三步走：① 配置参数 ② 设置引脚 ③ 安装驱动
     * 重复调用是安全的（已初始化则直接返回 ESP_OK）。
     *
     * @param baud_rate  波特率，默认 kBaudRate (115200)
     * @param tx_pin     TX 引脚，默认 GPIO39
     * @param rx_pin     RX 引脚，默认 GPIO40
     * @return ESP_OK 成功；其他 esp_err_t 表示失败
     */
    static esp_err_t Init(int baud_rate = kBaudRate,                  // 默认参数
                          gpio_num_t tx_pin = kTxPin,
                          gpio_num_t rx_pin = kRxPin);

    /**
     * @brief 反初始化：卸载驱动、释放 DMA 缓冲、解除 GPIO 占用
     *        幂等（多次调用安全）
     */
    static void Deinit();

    /**
     * @brief 当前是否已初始化（供调用方在调 uart_read/write 前做检查）
     */
    static bool IsInitialized();

    /**
     * @brief 阻塞发送一段二进制数据
     *
     * @param data        数据指针
     * @param len         字节数
     * @param timeout_ms  等待 DMA 收尾的最长时间（毫秒）
     * @return >=0 实际写入 DMA 的字节数；<0 错误
     */
    static int Send(const void* data, size_t len, uint32_t timeout_ms = 100);

    /**
     * @brief 便捷接口：发送 C 字符串（不含末尾 '\0'）
     */
    static int Send(const char* str);

    // =================================================================
    //  三个面向"串口助手"的语义化发送函数
    // -----------------------------------------------------------------
    //  下面的 SendString / SendArray / SendNumber 都是在上面那个
    //  Send() 之上做的"人类可读"包装：末尾自动追加 "\r\n"，串口助手
    //  直接以文本模式打开就能看到整齐的一行，特别适合打印调试。
    //
    //  如果你要发二进制协议帧（比如 Modbus、自定义协议），请用上面
    //  的 Send(const void*, size_t)，不要用下面这三个。
    // =================================================================

    /**
     * @brief 【1/3】发送字符串到串口助手（末尾自动追加 "\r\n"）
     *
     * 用法：
     *   UartComm::SendString("Hello World");        // 串口助手看到：Hello World\r\n
     *   std::string s = "temp=" + std::to_string(t);
     *   UartComm::SendString(s.c_str());            // 发 std::string 也 OK
     *
     * @param str 以 '\0' 结尾的 C 字符串（函数会自动补 \r\n，无需自带）
     * @return >=0 实际写入 DMA 的字节数（含 \r\n）；<0 错误
     */
    static int SendString(const char* str);

    /**
     * @brief 【2/3】发送字节数组到串口助手，按 HEX 文本格式显示
     *                 （末尾自动追加 "\r\n"）
     *
     * 每个字节转成 2 位大写十六进制，字节之间用空格分隔。
     *   例：{0x01, 0xAB, 0xFF} → "01 AB FF\r\n"
     *
     * 用法：
     *   uint8_t buf[] = {0xAA, 0x55, 0x00, 0xFF};
     *   UartComm::SendArray(buf, sizeof(buf));      // 串口助手看到：AA 55 00 FF\r\n
     *
     * 备注：单次最多显示 256 字节（防栈溢出）；超过会截断前 256 字节。
     *
     * @param arr 数组首地址
     * @param len 元素个数（字节数）
     * @return >=0 实际写入 DMA 的字节数；<0 错误
     */
    static int SendArray(const uint8_t* arr, size_t len);

    /**
     * @brief 【3/3】发送整型数值到串口助手（末尾自动追加 "\r\n"）
     *
     * 把整数转成十进制字符串输出。
     *   例：12345 → "12345\r\n"
     *   例：-7    → "-7\r\n"
     *
     * 用法：
     *   UartComm::SendNumber(123);                  // 串口助手看到：123\r\n
     *   int temperature = 25;
     *   UartComm::SendNumber(temperature);         // 25\r\n
     *
     * @param value 要发送的整数
     * @return >=0 实际写入 DMA 的字节数；<0 错误
     */
    static int SendNumber(int value);

    /**
     * @brief 【3/3 重载】发送浮点数值到串口助手（末尾自动追加 "\r\n"）
     *
     * 按指定小数位数格式化后输出。
     *   例：(3.14159f, 2) → "3.14\r\n"
     *   例：(25.0f,  1)   → "25.0\r\n"
     *
     * 用法：
     *   float voltage = 3.75f;
     *   UartComm::SendNumber(voltage, 2);          // 串口助手看到：3.75\r\n
     *
     * @param value    要发送的浮点数
     * @param decimals 保留的小数位数（默认 2，范围 0~6，越界自动夹紧）
     * @return >=0 实际写入 DMA 的字节数；<0 错误
     */
    static int SendNumber(float value, int decimals = 2);

    /**
     * @brief 阻塞接收一段原始字节
     *
     * @param buf         数据缓冲
     * @param buf_size    缓冲字节数
     * @param timeout_ms  等待超时（毫秒），0 = 立即返回
     * @return >0 实际收到的字节数；0 超时（无数据）；<0 错误
     */
    static int Receive(void* buf, size_t buf_size, uint32_t timeout_ms = 100);

private:
    // 静态成员：标记是否已初始化
    // 必须在 .cc 文件里提供类外定义（bool UartComm::initialized_ = false;）
    // 用 static 是因为整个类共享一个状态（UART 硬件只有一份）
    static bool initialized_;
};

// 头文件保护宏结束标记
#endif