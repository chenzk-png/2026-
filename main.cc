#include <esp_log.h>
#include <esp_err.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <driver/gpio.h>
#include <esp_event.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <cstdint>

#include "application.h"
#include "uart_comm.h"                              // ← UartComm 头文件
#include "ch32_rx.h"                                // ← CH32 协议收发
#include "weather_predictor/weather_predictor.h"    // ← 预测模块

#define TAG "main"

// ============================================================
//  测试任务：每 1 秒
//    1) ESP32 → CH32：发一帧"假数据"（fake_temp++/fake_humi++）
//    2) 喂预测器：把 CH32 回来的真实数据塞进 WeatherPredictor 历史
//    3) 预测并回传：每 5 秒触发一次预测，结果发回 CH32
// ============================================================
static void UartTestTask(void* arg) {
    int tick = 0;
    float val = 0.66f;

    // WeatherPredictor 单例(放到函数里保证 main 流程先跑完再到这)
    static WeatherPredictor g_weather;

    // ★ 第一次进 task 时显式 Init TFLite(heap 已就绪)
    if (!g_weather.Init()) {
        ESP_LOGE(TAG, "WeatherPredictor Init FAILED — predictions will be skipped");
    } else {
        ESP_LOGI(TAG, "WeatherPredictor ready");
    }

    // 上次预测的时间戳
    static int last_pred_ms = 0;
    const int PRED_INTERVAL_MS = 5000;

    // 启动 CH32 接收后台任务
    ch32_rx_start();
    ESP_LOGI(TAG, "CH32 receiver started");

    while (true) {
        int t = 0, h = 0;
        if (ch32_rx_get_latest(&t, &h)) {
            printf("[CH32_RX] temp=%d humi=%d\n", t, h);
            g_weather.OnCh32DataReceived(t, h);
        } else {
            printf("[CH32_RX] waiting for first frame...\n");
        }

        // ★ 启用预测:历史满 5 帧 且 距上次预测 >= 5 秒
        int now_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
        if (g_weather.Ready() && (now_ms - last_pred_ms) >= PRED_INTERVAL_MS) {
            if (g_weather.PredictAndSendToCH32()) {
                last_pred_ms = now_ms;
            }
        }

        tick++;
        val += 0.01f;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

extern "C" void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "Erasing NVS flash to fix corruption");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    auto& app = Application::GetInstance();
    app.Initialize();

    ESP_ERROR_CHECK(UartComm::Init());
    ESP_LOGI(TAG, "UART ready on GPIO39(TX)/GPIO40(RX) @ 115200");

    xTaskCreate(UartTestTask, "uart_test", 4096, nullptr, 1, nullptr);
    ESP_LOGI(TAG, "UART2 test task started");

    ESP_LOGI(TAG, "Starting main event loop...");
    app.Run();
}

