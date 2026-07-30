*Copyright (C) 2026, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# TensorFlow model -> .onnx

This example shows how a model created with the TensorFlow framework can be converted to the .onnx format.
Note that modifications to the steps may need to be made for more complex models.

## Steps:

1. Export the TensorFlow model to the SavedModel format using [tf.saved_model.save](https://www.tensorflow.org/api_docs/python/tf/saved_model/save).

2. Use the following Python code as a reference for how to convert the TensorFlow model to .onnx:

    ```python
    # tried and working dependencies:
    #   tensorflow==2.19.0
    #   tf2onnx==1.17.0

    import tensorflow as tf
    import tf2onnx
    import pathlib


    # CONFIGURE FOR YOUR USE-CASE
    SAVED_MODEL_DIR = "./my_model"
    INPUT_SHAPE = [1, 224, 224, 3]
    ONNX_OPSET_VERSION = 17
    OUTPUT_PATH = "./my_model.onnx"


    onnx_path = str(pathlib.Path(OUTPUT_PATH))
    pathlib.Path(OUTPUT_PATH).parent.mkdir(parents=True, exist_ok=True)

    saved_model = tf.saved_model.load(SAVED_MODEL_DIR)
    infer = saved_model.signatures["serving_default"]
    input_items = list(infer.structured_input_signature[1].items())
    input_name, input_tensor = input_items[0]
    input_names = [input_name]
    input_signature = [
        tf.TensorSpec(INPUT_SHAPE, input_tensor.dtype, name=input_name)
    ]

    @tf.function
    def serving_wrapper(*args):
        kwargs = {name: arg for name, arg in zip(input_names, args)}
        return infer(**kwargs)

    tf2onnx.convert.from_function(
        serving_wrapper,
        input_signature=input_signature,
        opset=ONNX_OPSET_VERSION,
        output_path=onnx_path,
    )
    ```
