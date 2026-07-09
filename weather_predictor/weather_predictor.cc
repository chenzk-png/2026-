// =====================================================================
//  weather_predictor.cc —— 温湿度预测（TFLite Micro 安全版）
//
//  关键安全设计：
//    1. 构造时只初始化空成员，**不加载 TFLite**
//    2. Init() 单独调用，调用前必须 heap ready
//    3. 预测前检查所有指针，失败时返回 {0, 0}
// =====================================================================

#include "weather_predictor.h"
#include "model.h"
#include "ch32_rx.h"

#include <esp_log.h>
#include <cstdio>
#include <cstring>
#include <utility>
#include <vector>
#include <algorithm>

#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"

#define TAG "WeatherPredictor"

// ───────── tensor arena:堆分配,避免 BSS 段过大导致 boot 异常 ─────────
// 新模型 2380 字节 + 16 隐藏单元的中间张量,16KB 完全够用
static constexpr size_t kArenaSize = 16 * 1024;
static std::unique_ptr<uint8_t[]> g_arena_ptr;  // nullptr → Init() 时分配

// ─────────────────────────────────────────────────────────────────
//  构造 / 析构：完全空
// ─────────────────────────────────────────────────────────────────
WeatherPredictor::WeatherPredictor() {
    ESP_LOGI(TAG, "WeatherPredictor constructed (TFLite NOT loaded yet, call Init() later)");
}

WeatherPredictor::~WeatherPredictor() {
    interpreter_.reset();
    ESP_LOGI(TAG, "WeatherPredictor destroyed");
}

// ─────────────────────────────────────────────────────────────────
//  Init() —— 在 heap ready 后调一次
// ─────────────────────────────────────────────────────────────────
bool WeatherPredictor::Init() {
    if (initialized_) return true;

    ESP_LOGI(TAG, "Init TFLite: model=%d bytes, arena=%d bytes",
             (int)model_len, (int)kArenaSize);

    // 0) ★ 堆分配 tensor arena（不在 BSS 段占 64KB）
    g_arena_ptr.reset(new (std::nothrow) uint8_t[kArenaSize]);
    if (!g_arena_ptr) {
        ESP_LOGE(TAG, "Failed to allocate %d bytes for tensor arena (OOM)", (int)kArenaSize);
        return false;
    }

    // 1) 拿 model
    model_ptr_ = tflite::GetModel(model);
    if (model_ptr_ == nullptr) {
        ESP_LOGE(TAG, "GetModel returned NULL");
        return false;
    }
    if (model_ptr_->version() != TFLITE_SCHEMA_VERSION) {
        ESP_LOGE(TAG, "Schema mismatch: model=%d, runtime=%d",
                 model_ptr_->version(), TFLITE_SCHEMA_VERSION);
        return false;
    }

    // 2) Op resolver:新模型只用 MatMul + BiasAdd + Relu,只注册这三个
    //    (注意:AddFullyConnected 内部已带 bias add;但 Keras 转出来的 tflite
    //     把 BiasAdd 拆成独立 Add 节点,所以这里也要 Add)
    static tflite::MicroMutableOpResolver<3> resolver;
    resolver.AddFullyConnected();  // dense_1:MatMul
    resolver.AddAdd();             // dense_1 + output_0:BiasAdd
    resolver.AddRelu();            // dense_1:Relu

    // 3) Interpreter（用堆分配的 arena）
    interpreter_ = std::make_unique<tflite::MicroInterpreter>(
        model_ptr_, resolver, g_arena_ptr.get(), kArenaSize);
    if (!interpreter_) {
        ESP_LOGE(TAG, "make_unique<MicroInterpreter> failed");
        return false;
    }

    // 4) 分配张量
    if (interpreter_->AllocateTensors() != kTfLiteOk) {
        ESP_LOGE(TAG, "AllocateTensors failed (arena too small? need > %d?)", (int)kArenaSize);
        interpreter_.reset();
        return false;
    }

    // 5) 取输入/输出指针
    input_tensor_  = interpreter_->input(0);
    output_tensor_ = interpreter_->output(0);
    if (input_tensor_ == nullptr || output_tensor_ == nullptr) {
        ESP_LOGE(TAG, "input/output tensor is NULL");
        interpreter_.reset();
        return false;
    }

    // 6) Shape 校验:防止 model 跟 C++ 端 kInputDim/kOutputDim 对不上
    const int in_dim  = (int)(input_tensor_->bytes  / sizeof(float));
    const int out_dim = (int)(output_tensor_->bytes / sizeof(float));
    if (input_tensor_->type != kTfLiteFloat32 || output_tensor_->type != kTfLiteFloat32) {
        ESP_LOGE(TAG, "Tensor type must be float32 (in=%d, out=%d)",
                 (int)input_tensor_->type, (int)output_tensor_->type);
        interpreter_.reset();
        return false;
    }
    if (in_dim != kInputDim || out_dim != kOutputDim) {
        ESP_LOGE(TAG, "Tensor shape mismatch: model in=%d out=%d, expect in=%d out=%d",
                 in_dim, out_dim, kInputDim, kOutputDim);
        interpreter_.reset();
        return false;
    }

    ESP_LOGI(TAG, "TFLite ready: in=%d floats, out=%d floats, arena=%d bytes",
             in_dim, out_dim, (int)kArenaSize);
    initialized_ = true;
    return true;
}

// ─────────────────────────────────────────────────────────────────
//  接收：每帧调一次
// ─────────────────────────────────────────────────────────────────
void WeatherPredictor::OnCh32DataReceived(int temp, int humi) {
    // 简单范围检查:丢弃明显异常(CH32 上电/掉线/传感器 bug 常见)
    if (temp < -40 || temp > 80 || humi < 0 || humi > 100) {
        ESP_LOGW(TAG, "drop abnormal frame: temp=%d humi=%d", temp, humi);
        return;
    }
    temp_history_.push_back(static_cast<float>(temp));
    humi_history_.push_back(static_cast<float>(humi));
    if (static_cast<int>(temp_history_.size()) > kMaxHistory) {
        temp_history_.erase(temp_history_.begin());
        humi_history_.erase(humi_history_.begin());
    }
}

bool WeatherPredictor::Ready() const {
    const int needed = kInputDim / 2;   // 5 帧
    return static_cast<int>(temp_history_.size()) >= needed &&
           static_cast<int>(humi_history_.size()) >= needed;
}

// ─────────────────────────────────────────────────────────────────
//  预测
// ─────────────────────────────────────────────────────────────────
std::pair<float, float> WeatherPredictor::Predict() {
    if (!initialized_ || !interpreter_ || !input_tensor_ || !output_tensor_) {
        ESP_LOGE(TAG, "Predict: not initialized (call Init() first)");
        return {0.0f, 0.0f};
    }
    if (!Ready()) {
        ESP_LOGW(TAG, "Not enough data (have %d, need %d)",
                 static_cast<int>(temp_history_.size()), kInputDim / 2);
        return {0.0f, 0.0f};
    }

    // 1) 把历史 (temp, humi) 拷贝进输入张量，做 Z-score
    float* in = input_tensor_->data.f;
    if (in == nullptr) {
        ESP_LOGE(TAG, "input tensor data is NULL");
        return {0.0f, 0.0f};
    }
    const int n = kInputDim / 2;
    int base = static_cast<int>(temp_history_.size()) - n;
    for (int i = 0; i < n; ++i) {
        in[i * 2 + 0] = (temp_history_[base + i] - kTempMean) / kTempStd;
        in[i * 2 + 1] = (humi_history_[base + i] - kHumiMean) / kHumiStd;
    }

    // 2) 推理
    if (interpreter_->Invoke() != kTfLiteOk) {
        ESP_LOGE(TAG, "Invoke failed");
        return {0.0f, 0.0f};
    }

    // 3) 读输出反归一化 + 钳位
    const float* out = output_tensor_->data.f;
    if (out == nullptr) {
        ESP_LOGE(TAG, "output tensor data is NULL");
        return {0.0f, 0.0f};
    }
    float pred_t = out[0] * kTempStd + kTempMean;
    float pred_h = (kOutputDim > 1 ? out[1] : out[0]) * kHumiStd + kHumiMean;
    // 物理合理范围,避免发回 CH32 显示 -273°C / 200%
    pred_t = std::clamp(pred_t, -40.0f, 80.0f);
    pred_h = std::clamp(pred_h,   0.0f, 100.0f);
    ESP_LOGI(TAG, "TFLite predict: temp=%.2fC humi=%.2f%%", pred_t, pred_h);
    return {pred_t, pred_h};
}

// ─────────────────────────────────────────────────────────────────
//  发到 CH32
// ─────────────────────────────────────────────────────────────────
void WeatherPredictor::SendToCH32(float temp, float humi) {
    ch32_rx_send(static_cast<int>(temp), static_cast<int>(humi));
    ESP_LOGI(TAG, "-> CH32: temp=%.2fC humi=%.2f%%", temp, humi);
}

bool WeatherPredictor::PredictAndSendToCH32() {
    if (!Ready()) return false;
    auto [t, h] = Predict();
    SendToCH32(t, h);
    return true;
}

// ─────────────────────────────────────────────────────────────────
//  LLM 工具入口
// ─────────────────────────────────────────────────────────────────
std::string WeatherPredictor::Run(const std::string& args) {
    ESP_LOGI(TAG, "LLM Run: %s", args.c_str());
    if (!Ready()) {
        return R"({"error": "Not enough sensor data yet"})";
    }
    auto [t, h] = Predict();
    SendToCH32(t, h);
    char buf[256];
    std::snprintf(buf, sizeof(buf),
        R"({"next_temperature": %.2f, "next_humidity": %.2f, "unit_temp": "C", "unit_humi": "%%"})",
        t, h);
    return std::string(buf);
}

std::string WeatherPredictor::Description() {
    return "Predict the next temperature and humidity based on recent sensor readings. "
           "Use when the user asks about future weather, like '下一时刻的温度' or '预测一下湿度'.";
}
