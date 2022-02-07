"""
 * Copyright (C) 2020 Axis Communications AB, Lund, Sweden
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
"""


"""
Description of convert_model.py
Converts a saved Tensorflow frozen graph to the Tensorflow Lite
format.

    Usage: python convert_model.py -i <frozen graph path> \
                                   [-o <.tflite output path>]
"""

import argparse
import tensorflow as tf


parser = argparse.ArgumentParser(description='Converts a frozen graph to \
                                              .tflite')
parser.add_argument('-i', '--input', type=str, required=True,
                    help='path to the frozen graph to convert')
parser.add_argument('-o', '--output', type=str,
                    default='converted_model.tflite',
                    help='path to output the .tflite model to')

args = parser.parse_args()

# Create the converter. As the model to convert is on the
# frozen graph format, the from_frozen_graph function is used.
# This function requires us to specify the names of the input
# and output tensors.
converter = tf.lite.TFLiteConverter.from_frozen_graph(args.input,
             input_arrays=['image_input'],
             output_arrays=['A_person_pred/Sigmoid', 'B_car_pred/Sigmoid'])

converter.experimental_new_converter = False
converter.experimental_new_quantizer = False

converter.quantized_input_stats = {'image_input': (0, 255)}
converter.inference_type = tf.uint8
converter.inference_input_type = tf.uint8

tflite_model = converter.convert()
open(args.output, "wb").write(tflite_model)
