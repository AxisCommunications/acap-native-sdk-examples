*Copyright (C) 2026, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# onnx model -> cv25 native model

This example shows how to convert an .onnx model to a native cv25 model for execution on cv25 devices.
Note that modifications to the steps are needed based on the model.

## Steps:

1. Collect a set of varied images that represent the scenario the model will see when it is deployed on the camera.
Put the images in a directory, e.g., ./representative_images.
Usually around 100-200 images are used for best results, but fewer can be used, with the risk that the model might perform slightly worse.

2. Inside the CNNGen Toolchain container, modify and run the following commands to generate images for quantization and to convert the model to the cv25 native format:

    ```bash
    #!/bin/bash
    INPUT_MODEL=my_model.onnx
    OUTPUT_MODEL=my_model_cv25.bin
    IMAGE_SIZE=(224 224)

    image_height=${IMAGE_SIZE[0]}
    image_width=${IMAGE_SIZE[1]}

    gen_image_list.py -f ./representative_images -o img_list.txt -ns -e .jpg -c 0 -d 0,0 -r $image_height,$image_width -bf dra_image_bin -bo dra_image_bin/dra_bin_list.txt

    onnxparser.py -m $INPUT_MODEL -isrc "is:1,3,$image_height,$image_width|iq|idf:0,0,8,0|i:images=dra_image_bin/dra_bin_list.txt" -o quantized -of quantized -odst "o:output0|odf:fp32"

    vas -auto -show-progress quantized/quantized.vas

    cavalry_gen -d vas_output/ -f $OUTPUT_MODEL
    ```
