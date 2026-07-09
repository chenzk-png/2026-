"""
从 main/weather_predictor/model.h 的 model[] 字节数组里
还原出 .tflite 二进制，再列出所有张量（权重、bias 等）
"""
import re
import sys
import os
import tempfile

try:
    import tflite_runtime.interpreter as tflite
except ImportError:
    from tensorflow import lite as tflite

model_h_path = sys.argv[1] if len(sys.argv) > 1 else "main/weather_predictor/model.h"
print(f"Reading {model_h_path} ...")

with open(model_h_path, "r", encoding="utf-8") as f:
    content = f.read()

match = re.search(
    r'const\s+(?:unsigned\s+)?char\s+model\s*\[\s*\]\s*=\s*\{([^}]+)\}',
    content, re.DOTALL,
)
if not match:
    print("ERROR: can't find model[] in", model_h_path)
    sys.exit(1)

hex_str = match.group(1)
bytes_list = []
for tok in hex_str.split(","):
    tok = tok.strip()
    if not tok:
        continue
    if tok.startswith("0x") or tok.startswith("0X"):
        bytes_list.append(int(tok, 16))
    else:
        bytes_list.append(int(tok))
model_bytes = bytes(bytes_list)
print(f"Got {len(model_bytes)} bytes\n")

tmp = tempfile.NamedTemporaryFile(suffix=".tflite", delete=False)
tmp.write(model_bytes)
tmp.close()
tmpfile = tmp.name

try:
    interp = tflite.Interpreter(model_path=tmpfile)
    interp.allocate_tensors()

    print("===== Input/Output =====")
    for d in interp.get_input_details():
        print(f"  Input : name={d['name']!r}, shape={d['shape']}, dtype={d['dtype']}")
    for d in interp.get_output_details():
        print(f"  Output: name={d['name']!r}, shape={d['shape']}, dtype={d['dtype']}")

    print("\n===== All Tensors (weights & biases) =====")
    for d in interp.get_tensor_details():
        name = d['name']
        tensor = interp.get_tensor(d['index'])
        print(f"\n[{name}] shape={tensor.shape}, dtype={tensor.dtype}")
        if tensor.size <= 200:
            print("  " + str(tensor.flatten().tolist()))
        else:
            print(f"  (size={tensor.size}, too large to print all)")
finally:
    os.unlink(tmpfile)
