"""
verify_inference.py
===================
端到端模拟 weather_predictor.cc 的运行流程,验证:
  1) 加载新 model.h(从字节数组)
  2) 喂入 5 帧合成 (temp, humi),做 Z-score
  3) 跑 inference
  4) 反归一化
  5) 钳位到物理合理范围

跟 C++ 端 kTempMean/kTempStd/kHumiMean/kHumiStd 完全一致。
"""
import re
import sys
import numpy as np

try:
    import tflite_runtime.interpreter as tflite
except ImportError:
    from tensorflow import lite as tflite

# 跟 weather_predictor.h 完全对齐
TEMP_MEAN, TEMP_STD = 22.491249, 6.775634
HUMI_MEAN, HUMI_STD = 70.873367, 9.515366
LOOK_BACK = 5

# ── 1) 从 model.h 抽出字节数组 ──
model_h = "main/weather_predictor/model.h"
with open(model_h, "r", encoding="utf-8") as f:
    src = f.read()
m = re.search(r"const\s+unsigned\s+char\s+model\s*\[\s*\]\s*=\s*\{([^}]+)\}", src, re.DOTALL)
assert m, "model[] not found"
data = bytes(int(t.strip(), 16) for t in m.group(1).split(",") if t.strip())
print(f"Loaded model from {model_h}: {len(data)} bytes")

# ── 2) 加载 + 跑推理 ──
import tempfile, os
tmp = tempfile.NamedTemporaryFile(suffix=".tflite", delete=False)
tmp.write(data); tmp.close()
try:
    interp = tflite.Interpreter(model_path=tmp.name)
    interp.allocate_tensors()  # ★ 这步老模型会崩,新模型应该通过
    print("allocate_tensors OK")
finally:
    pass

inp = interp.get_input_details()[0]
out = interp.get_output_details()[0]
print(f"Input : shape={inp['shape']} dtype={inp['dtype']}")
print(f"Output: shape={out['shape']} dtype={out['dtype']}")
assert tuple(inp["shape"]) == (1, LOOK_BACK * 2), f"expect (1,10), got {inp['shape']}"
assert tuple(out["shape"]) == (1, 2), f"expect (1,2), got {out['shape']}"

# ── 3) 模拟 OnCh32DataReceived 喂 5 帧 ──
np.random.seed(42)
temps = [20 + i * 0.3 + np.random.normal(0, 0.1) for i in range(LOOK_BACK)]
humis = [65 + i * 0.2 + np.random.normal(0, 0.1) for i in range(LOOK_BACK)]
print(f"\nHistory: temp={[round(t,2) for t in temps]}  humi={[round(h,2) for h in humis]}")

# ── 4) 模拟 Predict():Z-score + 推理 + 反归一化 + 钳位 ──
x = np.zeros((1, LOOK_BACK * 2), dtype=np.float32)
for i in range(LOOK_BACK):
    x[0, i*2 + 0] = (temps[i] - TEMP_MEAN) / TEMP_STD
    x[0, i*2 + 1] = (humis[i] - HUMI_MEAN) / HUMI_STD

interp.set_tensor(inp["index"], x)
interp.invoke()
y = interp.get_tensor(out["index"])[0]

pred_t = y[0] * TEMP_STD + TEMP_MEAN
pred_h = y[1] * HUMI_STD + HUMI_MEAN
pred_t = max(-40.0, min(80.0, pred_t))
pred_h = max(  0.0, min(100.0, pred_h))

print(f"\nRaw Z-score output: temp={y[0]:.4f}  humi={y[1]:.4f}")
print(f"Predicted (next step): temp={pred_t:.2f}C  humi={pred_h:.2f}%")

# 合理性:预测值应该在最近观测附近小幅波动
assert -20 <= pred_t <= 60, f"temp {pred_t} out of expected band"
assert 30  <= pred_h <= 95, f"humi {pred_h} out of expected band"
print("\n[OK] End-to-end inference pipeline works on new model.")
