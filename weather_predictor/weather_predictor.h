// =====================================================================
//  weather_predictor.h —— 温湿度预测模块（TFLite Micro 版本，安全版）
//
//  关键改动：
//    - 构造时**不**初始化 TFLite（避免 heap 不可用时的崩溃）
//    - 提供显式 Init() 方法，在 main 流程里 heap ready 之后调用
//    - 所有 TFLite 调用都做 nullptr 检查
//
//  数据流：
//    CH32 → ch32_rx (后台解析) → OnCh32DataReceived(t, h) 累积
//                                       ↓
//                                PredictAndSendToCH32()  (每 5 秒一次)
//                                       ↓
//                                Init() → TFLite inference
//                                       ↓
//                                  ch32_rx_send() → CH32
// =====================================================================

#ifndef WEATHER_PREDICTOR_H
#define WEATHER_PREDICTOR_H

#include <cstdint>
#include <vector>
#include <utility>        // std::pair
#include <string>
#include <memory>         // std::unique_ptr

// TfLiteTensor 完整定义（全局命名空间）
#include "tensorflow/lite/core/c/common.h"

namespace tflite {
class MicroInterpreter;
struct Model;
}  // namespace tflite

class WeatherPredictor {
public:
    WeatherPredictor();
    ~WeatherPredictor();

    // ★ 显式初始化 TFLite。**必须在 heap ready 之后调用**（建议在 app_main 里）。
    // 返回 true 表示成功；false 表示失败，Predict() 之后会返回 {0, 0}。
    bool Init();

    // 喂一帧 CH32 回来的 (温度, 湿度)
    void OnCh32DataReceived(int temp, int humi);

    // 累积样本够 LOOK_BACK 帧才能推理
    bool Ready() const;

    // 一次性：预测 + 发回 CH32
    bool PredictAndSendToCH32();

    // LLM 工具入口
    std::string Run(const std::string& args);
    static std::string Description();

private:
    // 内部：仅推理
    std::pair<float, float> Predict();

    // 内部：发到 CH32
    void SendToCH32(float temp, float humi);

    // 历史缓冲
    std::vector<float> temp_history_;
    std::vector<float> humi_history_;
    static constexpr int kMaxHistory = 16;

    // TFLite 相关（懒加载，Init() 后才有效）
    bool                                initialized_   = false;
    std::unique_ptr<tflite::MicroInterpreter> interpreter_;
    const tflite::Model*                 model_ptr_   = nullptr;
    const ::TfLiteTensor*                input_tensor_ = nullptr;
    const ::TfLiteTensor*                output_tensor_ = nullptr;
};

// 模型输入维度：5 步 × (temp, humi) = 10 维
constexpr int kInputDim  = 10;
constexpr int kOutputDim = 2;

// Z-score 归一化参数（与训练脚本一致）
constexpr float kTempMean = 22.491249f;
constexpr float kTempStd  = 6.775634f;
constexpr float kHumiMean = 70.873367f;
constexpr float kHumiStd  = 9.515366f;

#endif  // WEATHER_PREDICTOR_H
