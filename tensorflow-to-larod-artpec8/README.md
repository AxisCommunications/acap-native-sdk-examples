*Copyright (C) 2022, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# From Tensorflow model to larod inference on camera

## Overview

In this example, we look at the process of running a Tensorflow model on
an AXIS ARTPEC-8 camera. We go through the steps needed from the training of the model
to actually running inference on a camera by interfacing with the
[larod API](https://axiscommunications.github.io/acap-documentation/docs/api/4.1/api/larod/html/index.html).
This example is somewhat more comprehensive and covers e.g.,
model conversion, model quantization, image formats and creation and use of a model with multiple output tensors in
greater depth than the [larod](../larod) and [vdo-larod](../vdo-larod) examples.

## Table of contents

1. [Prerequisites](#prerequisites)
2. [Structure of this example](#structure-of-this-example)
3. [Quickstart](#quickstart)
4. [Environment for building and training](#environment-for-building-and-training)
5. [The example model](#the-example-model)
6. [Model training and quantization](#model-training-and-quantization)
7. [Model conversion](#model-conversion)
8. [Designing the application](#designing-the-application)
9. [Building the algorithm's application](#building-the-algorithms-application)
10. [Installing the algorithm's application](#installing-the-algorithms-application)
11. [Running the algorithm](#running-the-algorithm)

## Prerequisites

- An AXIS ARTPEC-8 camera, e.g., the Q1656
- NVIDIA GPU and drivers [compatible with Tensorflow r1.15](https://www.tensorflow.org/install/source#gpu)
- [Docker](https://docs.docker.com/get-docker/)
- [NVIDIA container toolkit](https://docs.nvidia.com/datacenter/cloud-native/container-toolkit/install-guide.html#installing-on-ubuntu-and-debian)

## Structure of this example

```bash
tensorflow_to_larod-artpec8
├── build_env.sh
├── Dockerfile
├── env
│   ├── app
│   │   ├── argparse.c
│   │   ├── argparse.h
│   │   ├── imgconverter.c
│   │   ├── imgconverter.h
│   │   ├── imgprovider.c
│   │   ├── imgprovider.h
│   │   ├── LICENSE
│   │   ├── Makefile
│   │   ├── manifest.json
│   │   ├── tensorflow_to_larod_a8.c
│   ├── .dockerignore
│   ├── build_acap.sh
│   ├── convert_model.py
│   ├── Dockerfile
│   ├── training
│   │   ├── model.py
│   │   ├── train.py
│   │   ├── utils.py
│   ├── yuv
│   │   ├── 0001-Create-a-shared-library.patch
├── README.md
└── run_env.sh
└── models.sha512
```

- **build_env.sh** - Builds the environment in which this example is run.
- **Dockerfile** - Docker file with the specified Axis toolchain and API container to build the example specified.
- **env/app/argparse.c/h** - Implementation of argument parser, written in C.
- **env/app/imgconverter.c/h** - Implementation of libyuv parts, written in C.
- **env/app/imgprovider.c/h** - Implementation of vdo parts, written in C.
- **env/app/Makefile** - Makefile containing the build and link instructions for building the ACAP application.
- **env/app/manifest.json** - Defines the ACAP and its configuration.
- **env/app/tensorflow_to_larod_a8.c** - The file implementing the core functionality of the ACAP.
- **env/build_acap.sh** - Builds the ACAP and the .eap file.
- **env/convert_model.py** - A script used to convert Tensorflow models to Tensorflow Lite models.
- **env/Dockerfile** - Docker file with the specified Axis toolchain and API container to build the example specified.
- **env/training/model.py** - Defines the Tensorflow model used in this example.
- **env/training/train.py** - Defines the model training procedure of this example.
- **env/training/utils.py** - Contains a datagenerator which specifies how data is loaded to the training process.
- **env/yuv/** - Folder containing patch for building libyuv.
- **run_env.sh** - Runs the environment in which this example is run.
- **models.sha512** - The file necessary to download a pretrained model.

## Quickstart

The following instructions can be executed to simply run the example. Each step is described in greater detail in the following sections.

1. Build and run the example environment. This mounts the `env` directory into the container:

   ```sh
   ./build_env.sh
   ./run_env.sh <a_name_for_your_env>
   ```

2. Train a Tensorflow model _(optional, pre-trained models available in `/env/models`)_:

   ```sh
   python training/train.py -i data/images/val2017/ -a data/annotations/instances_val2017.json
   ```

3. Convert the trained model to `.tflite`:

   ```sh
   python convert_model.py -i models/trained_model.pb -o app/converted_model.tflite
   ```

4. Compile the ACAP:

   ```sh
   ./build_acap.sh tensorflow_to_larod_a8_acap:1.0
   ```

5. In a new terminal, copy the ACAP `.eap` file from the example environment:

   ```sh
   docker cp <a_name_for_your_env>:/env/build/tensorflow_to_larod_a8_1_0_0_aarch64.eap tensorflow_to_larod_a8.eap
   ```

6. Install and start the ACAP on your camera through the GUI
7. SSH to the camera
8. View its log to see the ACAP output:

    ```sh
    tail -f /var/volatile/log/info.log | grep tensorflow_to_larod
    ```

## Environment for building and training

In this example, we're going to be working within a Docker container environment. This is done as to get the correct version of Tensorflow installed, as well as the needed tools. The `run_env.sh` script also mounts the `env` directory to allow for easier interaction with the container. To start the environment, run the build script and then the run script with what you want to name the environment as an argument. The build script forwards the environment variables `http_proxy` and `https_proxy` to the environment to allow proxy setups. The scripts are run as seen below:

```sh
./build_env.sh
./run_env.sh <a_name_for_your_env>
```

The environment can be started without GPU support by supplying the `--no-gpu` flag to the `run_env.sh` script after the environment name.

Note that the MS COCO 2017 validation dataset is downloaded during the building of the environment. This is roughly 1GB in size which means this could take a few minutes to download.

## The example model

In this example, we'll train a simple model with one input and two outputs. The input to the model is a FP32 RGB image scaled to the [0, 1] range and of shape `(480, 270, 3)`.
The output of the model are two separate tensors of shape `(1,)`, representing the model's confidences for the presence of `person`  and `car`. The outputs are configured as such, and not as one tensor with a SoftMax activation, in order to demonstrate how to use multiple outputs.
However, the general process of making a camera-compatible model is the same irrespective of the dimensions or number of inputs or outputs.

The model primarily consist of convolutional layers.
These are ordered in a [ResNet architecture](https://en.wikipedia.org/wiki/Residual_neural_network), which eventually feeds into a pooling operation, fully-connected layers and finally the output tensors.
The output values are compressed into the `[0,1]` range by a sigmoid function.

Notable with the model is the specifics of the ResNet blocks.
In order to produce a model with BatchNormalization layers that are fused with the convolutional layers during the conversion to `.tflite` in Tensorflow 1.15, a few modifications need to be made.
Specifically, the convolutional layers need to not use bias, e.g., for Keras Conv2D layers have the `use_bias=False` parameter set, and the layer order needs to be: `convolutional layer -> batch normalization -> relu`.
This will "fold" , or "fuse", the batch normalization, which increases performance.

The pre-trained model is trained on the MS COCO 2017 **training** dataset, which is significantly larger than the supplied MS COCO 2017 **validation** dataset. After training it for 8 epochs and finetuning the model with quantization for 4 epochs, it achieves around 85% validation accuracy on both the people output and the car output with 6.6 million parameters. This model is saved in the frozen graph format in the `/env/models` directory.

## Model training and quantization

Running the training process yourself is done by executing the following command, where the `-i` flag points to the folder containing the images and the `-a` flag points to the annotation `.json`-file:

 ```sh
 python training/train.py -i data/images/val2017/ -a data/annotations/instances_val2017.json
 ```

If you wish to train the model with the larger COCO 2017 training set and have downloaded it as described in [a previous section](#environment-for-building-and-training), simply substitute `val2017` for `train2017` in the paths in the command above to use that instead.

To get good machine learning performance on low-power devices,
[quantization](https://www.tensorflow.org/model_optimization/guide/quantization/training)
is commonly employed. While machine learning models are commonly
trained using 32-bit floating-point precision numbers, inference can
be done using less precision. This generally results in significantly lower
inference latency and model size with only a slight penalty to the model's
accuracy.

As noted previously, this example uses an ARTPEC-8 camera.
The camera DLPU uses INT8 precision, which means that the model will need to be quantized from
32-bit floating-point (FP32) precision to 8-bit integer (INT8) precision. The quantization can be done either
during training ([quantization-aware training](https://www.tensorflow.org/model_optimization/guide/quantization/training)) or
after training ([post-training quantization](https://www.tensorflow.org/lite/performance/post_training_quantization)).

The ARTPEC-8 DLPU uses per-tensor quantization, as opposed to per-axis quantization. This needs to be taken into account when
choosing how to train the model, as the per-tensor quantization scheme strongly favours quantization-aware training. Because
of this, quantization-aware training is used in the [training script](env/training/train.py) for this example.
Currently, per-tensor quantization is not supported in Tensorflow 2.x, which is why the `tf.contrib.quantize` module from Tensorflow 1.15 is used to perform the quantization-aware training.

Per-axis quantized models can be used, but at a penalty.
Using a model that is per-axis quantized will perform inference significantly slower than an equivalent per-tensor quantized model.
For this reason, it is not advised to use such a per-axis quantized model if inference latency is important to the application.

The training used in this example is divided into three steps. First, the model is initiated and trained
without any quantization. Next, quantization nodes that learn the quantization parameters for each tensor are added to the model
and the model is fine-tuned with these nodes present, which is the quantization-aware training part of the training process.
Finally, to configure the model for inference, the quantization nodes are set up to perform evaluation, as opposed to training.
With this done, the model is exported in its frozen graph state and ready to be converted to `.tflite`.

## Model conversion

To use the model on a camera, it needs to be in the `.tflite` format. The model that was trained in this example was
saved in the SavedModel format. This is Tensorflow's recommended format, but other formats can be
converted to `.tflite` as well. The conversion to `.tflite` is done in Python
using the Tensorflow package's
[Tensorflow Lite converter](https://www.tensorflow.org/lite/guide/get_started#tensorflow_lite_converter).

A script used for converting with the Tensorflow Lite converter is located in [/env/convert_model.py](env/convert_model.py).
We use it to convert our model by executing it with our model path, and, optionally, output path supplied as arguments:

```sh
python convert_model.py -i models/trained_model.pb -o app/converted_model.tflite
```

Now there should be a converted model called `converted_model.tflite` in the `app` directory.

__All the resulting pre-trained original, converted and compiled models are available in the `env/models` directory, so any step in the process can be skipped.__

## Designing the application

The application is based on the [vdo-larod-preprocessing](../vdo-larod-preprocessing) example. This example uses larod for both preprocessing and inference. Slight modifications have been made to accommodate the
multiple outputs of the model in the same manner as described in the [tensorflow-to-larod](../tensorflow-to-larod) example for the ARTPEC-7 platform. In that section, we go over the _rough outline_ of what needs to be done to run inference for our model, but again, the full code is available in [/env/app/tensorflow_to_larod_a8.c](env/app/tensorflow_to_larod_a8.c).

## Building the algorithm's application

A packaging file is needed to compile the ACAP. This is found in [app/manifest.json](app/manifest.json). The noteworthy attribute for this tutorial is the `runOptions` attribute. `runOptions` allows arguments to be given to the ACAP, which in this case is handled by the `argparse` lib. The argument order, defined by [app/argparse.c](app/argparse.c), is `<model_path input_resolution_width input_resolution_height output_size_in_bytes>`. We also need to copy our .tflite model file to the ACAP, and this is done by using the -a flag in the acap-build command in the Dockerfile. The -a flag simply tells the compiler what files to copy to the ACAP.

The ACAP is built to specification by the `Makefile` in [app/Makefile](env/app/Makefile). With the [Makefile](env/app/Makefile) and [manifest.json](env/app/manifest.json) files set up, the ACAP can be built by running the build script in the example environment:

```sh
./build_acap.sh tensorflow-to-larod-a8-acap:1.0
```

After running this script, the `build` directory should have been populated. Inside it is an `.eap` file, which is your stand-alone ACAP build.

## Installing the algorithm's application

To install an ACAP, the `.eap` file in the `build` directory needs to be uploaded to the camera and installed. This can be done through the camera GUI.

Outside of the example environment, extract the `.eap` from the environment by running:

```sh
docker cp <a_name_for_your_env>:/env/build/tensorflow_to_larod_a8_1_0_0_aarch64.eap tensorflow_to_larod_a8.eap
```

where `<a_name_for_your_env>` is the same name as you used to start your environment with the `./run_env.sh` script.
Then go to your camera -> Settings -> Apps -> Add -> Browse to `tensorflow_to_larod_a8.eap` and press Install.

## Running the algorithm

In the Apps view of the camera, press the icon for your ACAP. A window will pop up which allows you to start the application. Press the Start icon to run the algorithm.

With the algorithm started, we can view the output by either pressing "App log" in the same window, or by SSHing into the camera and viewing the log as below:

```sh
tail -f /var/volatile/log/info.log | grep tensorflow_to_larod_a8
```

Placing yourself in the middle of the cameras field of view should ideally make the log read something like:

```sh
[ INFO    ] tensorflow_to_larod_a8[1893]: Person detected: 98.2% - Car detected: 1.98%
```

## License

**[Apache License 2.0](../LICENSE)**

# Future possible improvements

- Interaction with non-neural network operations (eg NMS)
- Custom objects
- 2D output for showing overlay of e.g., pixel classifications
- Different output tensor dimensions for a more realistic use case
- Usage of the @tf.function decorator
- Quantization-aware training
