*Copyright (C) 2026, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# PyTorch model -> .onnx

This example shows how a model created with the PyTorch framework can be converted to the .onnx format.
Note that modifications to the steps may need to be made for more complex models.

## Steps:

1. Load the Pytorch model and weights:

    ```python
    model = MyModel(*args, **kwargs)
    checkpoint = torch.load("best_weights.pth", map_location="cpu")
    model.load_state_dict(checkpoint)
    model.eval()
    ```

2. Use the following Python code as a reference for how to convert the PyTorch model to .onnx:

    ```python
    # tried and working dependencies:
    #   torch==2.13.0
    #   onnx==1.19.1

    import torch
    import onnx
    import os
    import pathlib


    # CONFIGURE FOR YOUR USE-CASE
    INPUT_SHAPE = [1, 3, 224, 224]
    ONNX_OPSET_VERSION = 17
    OUTPUT_PATH = "./my_model.onnx"


    dummy_input = torch.zeros(*INPUT_SHAPE, dtype=torch.float32)
    onnx_path = str(pathlib.Path(OUTPUT_PATH))

    pathlib.Path(OUTPUT_PATH).parent.mkdir(parents=True, exist_ok=True)
    torch.onnx.export(
        model,
        dummy_input,
        onnx_path,
        input_names=["input"],
        output_names=["output"],
        opset_version=ONNX_OPSET_VERSION,
    )

    # Optional
    # Try to save the onnx in a single file without external data
    onnx_model = onnx.load(onnx_path)
    onnx.save(onnx_model, onnx_path, save_as_external_data=False)
    onnx_data_path = onnx_path + ".data"
    if os.path.exists(onnx_data_path):
        os.remove(onnx_data_path)
    ```
