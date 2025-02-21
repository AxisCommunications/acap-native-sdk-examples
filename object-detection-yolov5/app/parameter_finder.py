"""
Copyright (C) 2025, Axis Communications AB, Lund, Sweden

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    https://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
"""

"""
Check your model quantization parameters and save them to file
"""
import tensorflow as tf
import sys

if len(sys.argv) > 1:
    model_path = sys.argv[1]
else:
    print("Error: No model path provided as parameter. Please provide a path "
          "as a command-line argument.")
    exit(1)

output_file = "model_params.h"
interpreter = tf.lite.Interpreter(model_path)
interpreter.allocate_tensors()
output_details = interpreter.get_output_details()
input_details  = interpreter.get_input_details()

# The input format should be (batch, height, width, channel) but better verify
# with a test in case width and height are flipped.
model_input_height = input_details[0]["shape"][1]
model_input_width  = input_details[0]["shape"][2]

quantization_scale, quantization_zero_point = output_details[0]['quantization']
num_classes    = output_details[0]['shape'][2] - 5 # Removing 5 values that are
                                                   # x,y,w,h,obj_conf
num_detections = output_details[0]['shape'][1]

with open(output_file, "w") as f:
    f.write(f"#ifndef MODEL_PARAMS_H\n")
    f.write(f"#define MODEL_PARAMS_H\n\n")
    f.write(f"#define MODEL_INPUT_HEIGHT {model_input_height}\n")
    f.write(f"#define MODEL_INPUT_WIDTH {model_input_width}\n\n")
    f.write(f"#define QUANTIZATION_SCALE {quantization_scale}f\n")
    f.write(f"#define QUANTIZATION_ZERO_POINT {quantization_zero_point}\n\n")
    f.write(f"#define NUM_CLASSES {num_classes}\n")
    f.write(f"#define NUM_DETECTIONS {num_detections}\n\n")
    f.write(f"#endif // MODEL_PARAMS_H\n")

print(f"Model parameters have been saved to {output_file}.")