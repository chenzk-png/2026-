"""
train_dummy.py
==============
重训一个能被 TFLite Micro 直接跑的「温湿度预测」小模型,替换
main/weather_predictor/model.h 里的旧 model(旧 model 用了 FlexTensorListReserve
等 Flex op,Micro 跑不动)。

输入: 5 步 × (temp, humi) = 10 维(已经 Z-score)
输出: 2 维(Z-score 之后的 (temp, humi))
网络: 10 → 16 (ReLU) → 2

训练数据: 合成正弦波(温度 24h 周期,湿度 12h 周期) + 噪声,
         用滑动窗口构造 (X, y) 样本,让网络学会"用过去 5 步推下一步"。

注意: 训练/验证时的 Z-score 参数必须跟 C++ 端 weather_predictor.h 里的
       kTempMean/kTempStd/kHumiMean/kHumiStd 一致,否则反归一化后值会乱。

用法:
    python tools/train_dummy.py
    -> 生成 tools/dummy_model.tflite
    -> 再用 tools/emit_model_header.py 把它转成 main/weather_predictor/model.h
"""

import math
import os
import sys
import numpy as np

# 让 Python 找到 Keras / TF
os.environ.setdefault("TF_CPP_MIN_LOG_LEVEL", "2")

import tensorflow as tf
from tensorflow import keras

# ─────────────────────────── 与 C++ 端对齐的归一化参数 ───────────────────────────
TEMP_MEAN = 22.491249
TEMP_STD  = 6.775634
HUMI_MEAN = 70.873367
HUMI_STD  = 9.515366

LOOK_BACK = 5
N_FEATURES = 2
INPUT_DIM  = LOOK_BACK * N_FEATURES  # 10

# ─────────────────────────── 合成数据 ───────────────────────────
def synth_series(n_samples: int):
    """生成 (n_samples,) 的温度与湿度时序,带噪声。"""
    t = np.arange(n_samples)
    # 温度:24 小时周期 + 8h 小周期,加漂移 + 噪声
    temp = (22.0 + 5.0 * np.sin(2 * np.pi * t / 24.0)
                + 1.5 * np.sin(2 * np.pi * t / 8.0)
                + 0.05 * t / n_samples  # 长期漂移
                + np.random.normal(0, 0.3, n_samples))
    # 湿度:12h 周期,反相关于温度
    humi = (70.0 - 8.0 * np.sin(2 * np.pi * t / 12.0)
                + np.random.normal(0, 0.5, n_samples))
    return temp.astype(np.float32), humi.astype(np.float32)


def make_dataset(temp: np.ndarray, humi: np.ndarray, look_back: int):
    """滑动窗口 -> (X, y): X 是过去 look_back 步的 Z-score, y 是下一步的 Z-score."""
    n = len(temp)
    X_list, y_list = [], []
    for i in range(n - look_back):
        window_t = temp[i:i + look_back]
        window_h = humi[i:i + look_back]
        x = np.empty(look_back * 2, dtype=np.float32)
        x[0::2] = (window_t - TEMP_MEAN) / TEMP_STD
        x[1::2] = (window_h - HUMI_MEAN) / HUMI_STD
        y = np.array([(temp[i + look_back] - TEMP_MEAN) / TEMP_STD,
                      (humi[i + look_back] - HUMI_MEAN) / HUMI_STD], dtype=np.float32)
        X_list.append(x)
        y_list.append(y)
    return np.stack(X_list), np.stack(y_list)


# ─────────────────────────── 模型 ───────────────────────────
def build_model() -> keras.Model:
    model = keras.Sequential([
        keras.layers.Input(shape=(INPUT_DIM,), name="input_0"),
        keras.layers.Dense(16, activation="relu", name="dense_1"),
        keras.layers.Dense(2, name="output_0"),
    ])
    model.compile(optimizer=keras.optimizers.Adam(1e-3), loss="mse")
    return model


# ─────────────────────────── 主流程 ───────────────────────────
def main():
    out_dir = os.path.join(os.path.dirname(__file__))
    out_tflite = os.path.join(out_dir, "dummy_model.tflite")

    np.random.seed(0)
    tf.random.set_seed(0)

    print("Generating synthetic data ...")
    temp, humi = synth_series(4000)
    X, y = make_dataset(temp, humi, LOOK_BACK)
    print(f"  dataset: X={X.shape}, y={y.shape}")

    # 8 : 2 划分
    split = int(0.8 * len(X))
    X_train, X_val = X[:split], X[split:]
    y_train, y_val = y[:split], y[split:]

    model = build_model()
    model.summary()

    print("\nTraining ...")
    model.fit(X_train, y_train,
              validation_data=(X_val, y_val),
              epochs=30, batch_size=64, verbose=2)

    val_mse = model.evaluate(X_val, y_val, verbose=0)
    print(f"\nFinal val MSE (on Z-score space): {val_mse:.6f}")

    print(f"\nConverting to TFLite (float32, no Flex) ...")
    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    # ★ 关键:不让 TFLite 引入 select_tf_ops / Flex delegate
    converter.target_spec.supported_ops = [
        tf.lite.OpsSet.TFLITE_BUILTINS,  # 仅 Micro 实现的原生算子
    ]
    tflite_model = converter.convert()

    with open(out_tflite, "wb") as f:
        f.write(tflite_model)
    print(f"Wrote {out_tflite} ({len(tflite_model)} bytes)")

    # 立刻验证一次,确保 Micro 也能 allocate
    print("\nVerifying with tflite runtime (allocate_tensors should NOT throw) ...")
    try:
        import tflite_runtime.interpreter as tflite
    except ImportError:
        from tensorflow import lite as tflite
    interp = tflite.Interpreter(model_path=out_tflite)
    interp.allocate_tensors()
    inp = interp.get_input_details()[0]
    out = interp.get_output_details()[0]
    print(f"  Input : shape={inp['shape']}, dtype={inp['dtype']}")
    print(f"  Output: shape={out['shape']}, dtype={out['dtype']}")
    print("OK — model is TFLite-Micro compatible.")


if __name__ == "__main__":
    main()
