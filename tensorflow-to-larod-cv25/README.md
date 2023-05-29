*Copyright (C) 2022, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# Training and converting a model to Tensorflow Lite format

In this tutorial, we look at the process of training a model compatible with a CV25 camera and converting it to Tensorflow Lite format. To see an example of running an inference on a camera with a trained model, visit the [vdo-larod](../vdo-larod) example.

This tutorial is somewhat more comprehensive and covers e.g.,
model conversion, model quantization and overall creation of a model with multiple output tensors in greater depth than [vdo-larod](../vdo-larod) example.

## Getting started

These instructions will guide you on how to execute the code. Below is the structure and scripts used in the tutorial:

```sh
tensorflow_to_larod
├── README.md
├── Dockerfile
└── env
    ├── convert_model.py
    └── training
        ├── model.py
        ├── train.py
        └── utils.py

```

- **Dockerfile** - Docker file that builds an environment with the appropriate Tensorflow tools, training scripts and dataset.
- **env/convert_model.py** - A script used to convert Tensorflow models to Tensorflow Lite models.
- **env/training/model.py** - Defines the Tensorflow model used in this tutorial.
- **env/training/train.py** - Defines the model training procedure of this tutorial.
- **env/training/utils.py** - Contains a datagenerator which specifies how data is loaded to the training process.

### Prerequisites

- NVIDIA GPU and drivers [compatible with Tensorflow r1.15](https://www.tensorflow.org/install/source#gpu)
- [Docker](https://docs.docker.com/get-docker/)
- [NVIDIA container toolkit](https://docs.nvidia.com/datacenter/cloud-native/container-toolkit/install-guide.html#installing-on-ubuntu-and-debian)

### Limitations

- The tutorial illustrates how to train a model and convert it to tflite format. It does not show how to run the inference on an Axis device.

### How to run the code

In this tutorial, we are going to be working within a Docker container environment. This is done as to get the correct version of Tensorflow installed, as well as the needed tools. The model training is part of the container image build. To start the build and training, run:

```sh

docker build --tag <APP_IMAGE> .

```

<APP_IMAGE> is the name to tag the image with, e.g., tf-model-training-cv25:1.0

Copy the model from the container image to a local directory:

```sh

docker cp $(docker create <APP_IMAGE>):/env/training/models/converted_model.tflite ./<DESIRED_PATH>/converted_model.tflite

```

Compile the model using the [Ambarella toolchain](https://www.ambarella.com/technology/#cvflow)

> **Note**
> It is required to be an Ambarella partner to access Ambarella tools. Visit
> [this](https://customer.ambarella.com/ng/login?refid=EAAAALA9%2fIfpWDCn53oQJDd5FKfzsrI0fWXYseVTpgnkJHV1)
> webpage to access the tools. If you are not an Ambarella partner, you can
> get login credentials by registering in the same link.

Note that the MS COCO 2017 validation dataset is downloaded during the building of the environment. This is roughly 1GB in size which means this could take a few minutes to download.

To get good machine learning performance on low-power devices,
[quantization](https://www.tensorflow.org/model_optimization/guide/quantization/training)
is commonly employed. While machine learning models are commonly
trained using 32-bit floating-point precision numbers, inference can
be done using less precision. This generally results in significantly lower
inference latency and model size with only a slight penalty to the model's
accuracy.

If your machine doesn't have the hardware requisites, like not enough GPU to train or not enough storage to save the training data, you can take a look at [train-tensorflow-model-using-aws](https://github.com/AxisCommunications/acap-sdk-tools/tree/main/train-tensorflow-model-using-aws), a tool to automatise model training with AWS.

### The example model

In this tutorial, we'll train a simple model with one input and two outputs. The input to the model is a scaled FP32 RGB image of shape (256, 256, 3), while both outputs are scalar values. However, the process is the same regardless of the dimensions or number of inputs or outputs.
The two outputs of the model represent the model's confidences for the presence of `person`  and `car`. **Currently (TF2.3), there is a bug in the `.tflite` conversion which orders the model outputs alphabetically based on their name. For this reason, our outputs are named with A and B prefixes, as to retain them in the order our application expects.**

The model primarily consist of convolutional layers.
These are ordered in a [ResNet architecture](https://en.wikipedia.org/wiki/Residual_neural_network), which eventually feeds into a pooling operation, fully-connected layers and finally the output tensors.
The output values are compressed into the `[0,1]` range by a sigmoid function.

The pre-trained model is trained on the MS COCO 2017 **training** dataset, which is significantly larger than the supplied MS COCO 2017 **validation** dataset. After training it for 10 epochs, it achieves around 80% validation accuracy on the people output and 90% validation accuracy on the car output with 1.6 million parameters, which results in a model file size of 22 MB. This model is saved in Tensorflow's SavedModel format, which is the recommended option, in the `/env/models` directory.

### Model training and quantization

The following commmand in the Dockerfile starts the training process, where the `-i` flag points to the folder containing the images and the `-a` flag points to the annotation `.json`-file:

 ```sh
 python training/train.py -i /env/data/images/val2017/ -a /env/data/annotations/instances_val2017.json
 ```

There is an additional flag `-e` that points to number of epochs the model needs to be run for training. The default value for training the model is set to 10 epoch but could be modified accordingly. If the model is desired to be trained for 2 epochs, the above flag can be added as shown below:

 ```sh
 python training/train.py -i data/images/val2017/ -a data/annotations/instances_val2017.json -e 2
 ```

If you wish to train the model with the larger COCO 2017 training set and have downloaded it as described earlier, simply substitute `val2017` for `train2017` in the paths in the command above to use that instead.

To get good machine learning performance on low-power devices,
[quantization](https://www.tensorflow.org/model_optimization/guide/quantization/training)
is commonly employed. While machine learning models are commonly
trained using 32-bit floating-point precision numbers, inference can
be done using less precision. This generally results in significantly lower
inference latency and model size with only a slight penalty to the model's
accuracy.

### Model conversion

To use the model on a camera, it needs to be converted. The conversion from the `SavedModel` model to the camera ready format is divided into two steps:

1. Convert to Tensorflow Lite format (`.tflite`) with Edge TPU compatible data types, e.g., by using the supplied `convert_model.py` script

#### From SavedModel to .tflite

The model in this tutorial is saved in the SavedModel format. This is Tensorflow's recommended option, but other formats can be converted to `.tflite` as well. The conversion to `.tflite` is done in Python
using the Tensorflow package's [Tensorflow Lite converter](https://www.tensorflow.org/lite/guide/get_started#tensorflow_lite_converter).

For quantization to 8-bit integer precision, measurements on the network during inference needs to be performed.
Thus, the conversion process does not only require the model but also input data samples, which are provided using
a data generator. These input data samples need to be of the FP32 data type. Running the script below converts a specified SavedModel to a Tensorflow Lite model, given that the dataset generator function in the script is defined such that it yields data samples that fits our model's inputs.

This script is located in [training/env/convert_model.py](env/convert_model.py). We use it to convert our model by executing it with our model path, dataset path and output path supplied as arguments:

```sh
python convert_model.py -i models/saved_model -d data/images/val2017 -o models/converted_model.tflite
```

This process can take a few minutes as the validation dataset is quite large.

#### Compiling for CV25 DLPU

After conversion to the `.tflite` format, the `.tflite` model needs to be compiled for the CV25 chip using the [Ambarella toolchain](https://customer.ambarella.com/ng/).
Together with the tools, you should receive some documents that explain how to compile your model with the toolchain.
To convert our `.tflite` model first we will need an subsample of the train dataset to use for quantization. We can run the following command:

```sh
mkdir dra_image_bin
gen_image_list.py \
  -f pics \
  -o dra_image_bin/img_list.txt \
  -ns \
  -e .jpg \
  -c 0 \
  -d 0,0 \
  -r 256,256 \
  -bf dra_image_bin \
  -bo dra_image_bin/dra_bin_list.txt
```

There are first some options to define the input: `-f` is to set the folder that contains our image dataset, `-o` to set the image file list filename, `-e` to define the extension of the image files; and some options to define the output: `-ns` to disable random shuffle, `-c` to set the color format (RGB), `-d` to set the output data format (unsigned 8bit fixed-point), `-r` to set the image resolution, `-bf` to set the output folder to store binary files and `-bo` to set the binary files list filename.

Once it is done, we proceed parsing our model. This command reads the `.tflite` file and reconstructs the model. Then, it optionally adds pre/post processing nodes (such as reshape, transpose, scale, subtract), quantizes ("DRA") or generates VAS ("VP assembler") code.

```sh
tfparser.py -p converted_model.tflite \
   -o car_human_model \
   -of ./out_car_human_model \
   -isrc "is:1,3,256,256|iq|i:input_1=./dra_image_bin/dra_bin_list.txt|ic:255.00446258|idf:0,0,0,0"
   -odst "o:Identity|odf:fp32" \
   -odst "o:Identity_1|odf:fp32" \
   -c act-force-fx16,coeff-force-fx16

```

The options here are the following: `-o` sets the output filename, `-of` sets the output folder, `-isrc` defines the source string used during compilation, `-odst` sets the multiple output list, `-c` defines the configuration options used during compilation. The values for `-isrc` and `-c` are dependant on the previous step. Specifically for `-isrc`, the format here is `is:<input_shape>|iq|i:<input_list>|ic:<image_scaling>|idf:<input_file_data_format>`. As for the `-c` parameters, they are used to force the activation outputs and coefficients (weights and biases) to 16bit fixed-point.

This next command compiles VAS code generated by `tfparser.py`:

```sh
  vas \
  -auto \
  -show-progress \
  out_car_human_model/car_human_model.vas
```

Finally, we produce the cavalry file `car_human_model_cavalry.bin`, that can be deployed in the camera.

```sh
 cavalry_gen \
  -d vas_output \
  -f car_human_model_cavalry.bin
```

For more information about how to convert your specific model, please refer to the [Ambarella documentation](https://customer.ambarella.com/ng/).

### The expected output

A converted model named `converted_model.tflite` is created in the `training/models` directory.

## License

**[Apache License 2.0](../LICENSE)**

## Future possible improvements

- Interaction with non-neural network operations (eg NMS)
- Custom objects
- 2D output for showing overlay of e.g., pixel classifications
- Different output tensor dimensions for a more realistic use case
- Usage of the @tf.function decorator
- Quantization-aware training
