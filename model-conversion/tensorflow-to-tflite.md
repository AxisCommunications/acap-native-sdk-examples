*Copyright (C) 2026, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# TensorFlow model -> .tflite

This example shows how a model created with the TensorFlow framework can be quantized and converted to the .tflite format.
Note that modifications to the steps may need to be made for more complex models.

## Steps:

1. Export the TensorFlow model to the SavedModel format using [tf.saved_model.save](https://www.tensorflow.org/api_docs/python/tf/saved_model/save).

2. Collect a set of varied images that represent the scenario the model will see when it is deployed on the camera.
Put the images in a directory, e.g., ./representative_images.
Usually around 100-200 images are used for best results, but fewer can be used, with the risk that the model might perform slightly worse.

3. Use the following Python code as a reference for how to quantize and convert the SavedModel to .tflite:

    ```python
    # tried and working dependencies:
    #   tensorflow==2.19.0
    #   numpy==1.26.4
    #   pillow==12.2.0

    import tensorflow as tf
    import numpy as np
    import pathlib
    from typing import Iterable, List


    # CONFIGURE FOR YOUR USE-CASE
    SAVED_MODEL_DIR = "./my_model"
    INPUT_SHAPE = [1, 224, 224, 3]
    REPRESENTATIVE_IMAGES_DIR = "./representative_images"
    QUANTIZATION_METHOD = "per_channel"  # "per_channel" or "per_tensor"
    QUANTIZATION_DTYPE = "uint8"  #  "uint8" or "int8"
    OUTPUT_PATH = "./my_model.tflite"


    def representative_dataset_generator(
        input_shape: List[int],
    ) -> Iterable[List[np.ndarray]]:
        _, height, width, channels = input_shape
        color_mode = "grayscale" if channels == 1 else "rgb"
        image_dir = pathlib.Path(REPRESENTATIVE_IMAGES_DIR)
        image_files = [
            p
            for p in sorted(image_dir.rglob("*"))
            if p.suffix.lower() in {".jpg", ".jpeg", ".png", ".bmp"}
        ]

        for image_path in image_files:
            img = tf.keras.utils.load_img(
                image_path, target_size=(height, width), color_mode=color_mode
            )
            arr = tf.keras.utils.img_to_array(img).astype(np.float32) / 255.0
            yield [np.expand_dims(arr, axis=0)]


    converter = tf.lite.TFLiteConverter.from_saved_model(SAVED_MODEL_DIR)

    converter.optimizations = [tf.lite.Optimize.DEFAULT]
    converter.representative_dataset = lambda: representative_dataset_generator(
        input_shape=INPUT_SHAPE,
    )
    converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]

    if QUANTIZATION_DTYPE == "uint8":
        converter.inference_input_type = tf.uint8
        converter.inference_output_type = tf.uint8
    else:
        converter.inference_input_type = tf.int8
        converter.inference_output_type = tf.int8

    if QUANTIZATION_METHOD == "per_tensor":
        converter._experimental_disable_per_channel = True

    tflite_model = converter.convert()
    output = pathlib.Path(OUTPUT_PATH)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(tflite_model)
    ```
