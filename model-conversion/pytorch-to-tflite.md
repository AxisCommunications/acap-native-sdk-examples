*Copyright (C) 2026, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# PyTorch model -> .tflite

This example shows how a model created with the PyTorch framework can be quantized and converted to the .tflite format.
Note that modifications to the steps may need to be made for more complex models.

## Steps:

1. Load the Pytorch model and weights:

    ```python
    model = MyModel(*args, **kwargs)
    checkpoint = torch.load("best_weights.pth", map_location="cpu")
    model.load_state_dict(checkpoint)
    model.eval()
    ```

2. Collect a set of varied images that represent the scenario the model will see when it is deployed on the camera.
Put the images in a directory, e.g., ./representative_images.
Usually around 100-200 images are used for best results, but fewer can be used, with the risk that the model might perform slightly worse.

3. Use the following Python code as a reference for how to quantize and convert the PyTorch model to .tflite:

    ```python
    # tried and working dependencies:
    #   torch==2.13.0
    #   tensorflow==2.19.0
    #   numpy==2.4.6
    #   pillow==12.2.0
    #   onnx2tf==1.29.24
    #   onnxscript==0.7.1

    import torch
    import tensorflow as tf
    import numpy as np
    import onnx2tf
    import onnx2tf.onnx2tf as onnx2tf_mod
    from typing import Iterable, List
    import pathlib
    import tempfile


    # CONFIGURE FOR YOUR USE-CASE
    INPUT_SHAPE = [1, 3, 224, 224]
    REPRESENTATIVE_IMAGES_DIR = "./representative_images"
    QUANTIZATION_METHOD = "per_channel"  # or "per_tensor"
    QUANTIZATION_DTYPE = "int8"  # or "uint8"
    ONNX_OPSET_VERSION = 13
    OUTPUT_PATH = "./my_model.tflite"


    def representative_dataset_generator(
        input_shape: List[int],
    ) -> Iterable[List[np.ndarray]]:
        _, channels, height, width = input_shape
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


    with tempfile.TemporaryDirectory() as tmp_dir:
        onnx_path = str(pathlib.Path(tmp_dir) / "model.onnx")
        onnx2tf_output_dir = str(pathlib.Path(tmp_dir) / "onnx2tf")

        dummy_input_nhwc = next(representative_dataset_generator(INPUT_SHAPE))[0]
        dummy_input_nchw = np.transpose(dummy_input_nhwc, (0, 3, 1, 2))
        dummy_input = torch.from_numpy(dummy_input_nchw)
        onnx2tf_mod.download_test_image_data = lambda: dummy_input_nhwc
        torch.onnx.export(
            model,
            dummy_input,
            onnx_path,
            input_names=["input"],
            output_names=["output"],
            opset_version=ONNX_OPSET_VERSION,
        )

        onnx2tf.convert(
            input_onnx_file_path=onnx_path,
            output_folder_path=onnx2tf_output_dir,
        )

        converter = tf.lite.TFLiteConverter.from_saved_model(onnx2tf_output_dir)

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
