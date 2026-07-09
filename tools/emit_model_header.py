"""
emit_model_header.py
====================
把 tools/dummy_model.tflite 转成 main/weather_predictor/model.h,
让 TFLite Micro 能在编译期把它嵌入固件。

用法:
    python tools/train_dummy.py          # 训练得到 dummy_model.tflite
    python tools/emit_model_header.py    # 写新 model.h
"""

import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
SRC  = os.path.join(HERE, "dummy_model.tflite")
DEST = os.path.join(HERE, "..", "main", "weather_predictor", "model.h")


def to_c_array(data: bytes) -> str:
    """把 bytes 转成 16 个一行、0xXX 逗号分隔的 C 数组字面量。"""
    lines = []
    row = []
    for i, b in enumerate(data):
        row.append(f"0x{b:02x}")
        if (i + 1) % 16 == 0:
            lines.append("    " + ", ".join(row) + ",")
            row = []
    if row:
        lines.append("    " + ", ".join(row) + ",")
    return "\n".join(lines)


def main():
    if not os.path.exists(SRC):
        print(f"ERROR: {SRC} not found. Run train_dummy.py first.", file=sys.stderr)
        sys.exit(1)

    with open(SRC, "rb") as f:
        data = f.read()
    print(f"Read {SRC} ({len(data)} bytes)")

    body = to_c_array(data)

    header = f'''#ifndef MODEL_H
#define MODEL_H

// TFLite model ({len(data)} bytes) — 由 tools/train_dummy.py + tools/emit_model_header.py 生成
// 网络结构: input[10] -> Dense(16, ReLU) -> Dense(2)
// 输入: 5 步 × (temp, humi) Z-score
// 输出: 下一时刻 (temp, humi) Z-score
const unsigned char model[] = {{
{body}
}};
const int model_len = {len(data)};

#endif  // MODEL_H
'''

    with open(DEST, "w", encoding="utf-8") as f:
        f.write(header)
    print(f"Wrote {DEST}")


if __name__ == "__main__":
    main()
