*Copyright (C) 2021, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# From Tensorflow model to larod inference on camera
## Overview
In this example, we look at the process of running a Tensorflow model on
an Axis camera equipped with an Edge TPU. We go through the steps needed from the training of the model
to actually running inference on a camera by interfacing with the
[larod API](https://www.axis.com/techsup/developer_doc/acap3/3.1/api/larod/html/larod_8h.html).
This example is somewhat more comprehensive and covers e.g.,
model conversion, model quantization, image formats and models with multiple output tensors in
greater depth than the
[larod](https://github.com/AxisCommunications/acap4-native-sdk-examples/tree/master/larod)
and [vdo-larod](https://github.com/AxisCommunications/acap4-native-sdk-examples/tree/master/vdo-larod) examples.

## Table of contents
1. [Prerequisites](#prerequisites)
2. [Structure of this example](#structure-of-this-example)
3. [Quickstart](#quickstart)
4. [Environment for building and training](#environment-for-building-and-training)
5. [The example model](#the-example-model)
6. [Model quantization](#model-quantization)
7. [Model conversion](#model-conversion)
8. [Building the algorithm's application](#building-the-algorithms-application)
9. [Installing the algorithm's application](#installing-the-algorithms-application)
10. [Running the algorithm](#running-the-algorithm)

## Prerequisites
- Axis camera equipped with an [Edge TPU](https://coral.ai/docs/edgetpu/faq/)
- NVIDIA GPU and drivers [compatible with Tensorflow r2.3](https://www.tensorflow.org/install/source#gpu)
- [Docker](https://docs.docker.com/get-docker/)
- [NVIDIA container toolkit](https://docs.nvidia.com/datacenter/cloud-native/container-toolkit/install-guide.html#installing-on-ubuntu-and-debian)

## Structure of this example
```bash
tensorflow_to_larod
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
│   |   ├── manifest.json
│   │   └── tensorflow_to_larod.c
│   ├── .dockerignore
│   ├── build_acap.sh
│   ├── convert_model.py
│   ├── Dockerfile
│   ├── training
│   │   ├── model.py
│   │   ├── train.py
│   │   └── utils.py
│   └── yuv
│       └── 0001-Create-a-shared-library.patch
├── README.md
└── run_env.sh
```
- **build_env.sh** - Builds the environment in which this example is run.
- **Dockerfile** - Docker file with the specified Axis toolchain and API container to build the example specified.
- **env/app/argparse.c/h** - Implementation of argument parser, written in C.
- **env/app/imgconverter.c/h** - Implementation of libyuv parts, written in C.
- **env/app/imgprovider.c/h** - Implementation of vdo parts, written in C.
- **env/app/Makefile** - Makefile containing the build and link instructions for building the ACAP4 Native application.
* **env/app/manifest.json** - Defines the application and its configuration.
- **env/app/tensorflow_to_larod.c** - The file implementing the core functionality of the ACAP.
- **env/build_acap.sh** - Builds the ACAP and the .eap file.
- **env/convert_model.py** - A script used to convert Tensorflow models to quantized Tensorflow Lite models.
- **env/Dockerfile** - Docker file with the specified Axis toolchain and API container to build the example specified.
- **env/training/model.py** - Defines the Tensorflow model used in this example.
- **env/training/train.py** - Defines the model training procedure of this example.
- **env/training/utils.py** - Contains a datagenerator which specifies how data is loaded to the training process.
- **env/yuv/** - Folder containing patch for building libyuv.
- **run_env.sh** - Runs the environment in which this example is run.


## Quickstart
The following instructions can be executed to simply run the example. Each step is described in greater detail in the following sections.

1. Build and run the example environment:
```sh
./build_env.sh
./run_env.sh <a_name_for_your_env>
```
2. Train a Tensorflow model _(optional, pre-trained models available)_:
```sh
python training/train.py -i /env/data/images/val2017/ -a /env/data/annotations/instances_val2017.json
```
3. Quantize the Tensorflow model and convert it to `.tflite`:
```sh
python convert_model.py -i /env/models/saved_model -d /env/data/images/val2017 -o /env/models/converted_model.tflite
```
4. Compile the converted model to work on the Edge TPU:
```sh
edgetpu_compiler -s -o models models/converted_model.tflite
```
5. Copy the compiled converted model to the `app` directory:
```sh
cp models/converted_model_edgetpu.tflite app/
```

6. Compile the ACAP:
```sh
./build_acap.sh tensorflow-to-larod-acap:1.0
```

7. Open a new terminal

8. In the new terminal, copy the ACAP `.eap` file from the example environment:
```sh
docker cp <a_name_for_your_env>:/env/build/tensorflow_to_larod_app_1_0_0_armv7hf.eap tensorflow_to_larod.eap
```
9. Install and start the ACAP on your camera through the GUI

10. SSH to the camera

11. View its log to see the ACAP output:
```sh
tail -f /var/volatile/log/info.log | grep tensorflow_to_larod
```

## Environment for building and training
In this example, we're going to be working within a Docker container environment. This is done as to get the correct version of Tensorflow installed, as well as the needed tools. To start the environment, run the build script and then the run script with what you want to name the environment as an argument. The build script forwards the environment variables `http_proxy` and `https_proxy` to the environment to allow proxy setups. The scripts are run as seen below:
```sh
./build_env.sh
./run_env.sh <a_name_for_your_env>
```
The environment can be started without GPU support by supplying the `--no-gpu` flag to the `run_env.sh` script after the environment name.

Note that the MS COCO 2017 validation dataset is downloaded during the building of the environment. This is roughly 1GB in size which means this could take a few minutes to download.

## The example model
In this example, we'll train a simple model with one input and two outputs. The input to the model is a scaled FP32 RGB image of shape (256, 256, 3), while both outputs are scalar values. However, the process is the same irrespective of the dimensions or number of inputs or outputs.
The first output corresponds to the probability of there being people in the image and the second
output to the probability of there being cars in the image. __Currently (TF2.3), there is a bug in the `.tflite` conversion which orders the model outputs alphabetically based on their name. For this reason, our outputs are named with A and B prefixes, as to retain them in the order our ACAP expects.__

The pre-trained model is trained on the MS COCO 2017 **training** dataset, which is significantly larger than the supplied MS COCO 2017 **validation** dataset. After training it for 10 epochs, it achieves something like 80% validation accuracy on the people output and 90% validation accuracy on the car output with 1.6 million parameters, which results in a model file size of 22 MB. This model is saved in Tensorflow's SavedModel format, which is the recommended option, in the `/env/models` directory.

Either use the pre-trained model or train the model yourself. If you wish to perform the training process yourself, this can be done using the supplied validation dataset, or by downloading and using the larger training dataset from [the MS COCO site](https://cocodataset.org/#download). Training the model on the larger dataset will give better results.

Running the training process yourself is done by executing the following command, where the `-i` flag points to the folder containing the images and the `-a` flag points to the annotation `.json`-file:

 ```sh
 python training/train.py -i /env/data/images/val2017/ -a /env/data/annotations/instances_val2017.json
 ```


While this example looks at the process from model creation to inference on a camera, other pre-trained models
are available at e.g., https://www.tensorflow.org/lite/models and https://coral.ai/models/. The models from [coral.ai](https://coral.ai) are pre-compiled to run on the Edge TPU.

When designing your model for an Edge TPU device, you should only use operations that have an Edge TPU implementation. The full list of such operations is available at https://coral.ai/docs/edgetpu/models-intro/#supported-operations.

## Model quantization
To get good machine learning performance on low-power devices,
[quantization](https://www.tensorflow.org/model_optimization/guide/quantization/training)
is commonly employed. While machine learning models are commonly
trained using 32-bit floating-point precision numbers, inference can
be done using less precision. This generally results in significantly lower
inference latency and model size with only a slight penalty to the model's
accuracy.


| Chip     	| Supported precision 	|
|----------	|------------------	    |
| Edge TPU 	| INT8                 	|
| Common CPUs  	    | FP32, INT8            |
| Common GPUs       | FP32, FP16, INT8      |

As noted in the first chapter, this example uses a camera equipped with an Edge TPU.
As the Edge TPU chip __only__ uses INT8 precision, the model will need to be quantized from
32-bit floating-point (FP32) precision to 8-bit integer (INT8) precision. The quantization
will be done during the conversion described in the next section.

## Model conversion
To use the model on a camera, it needs to be converted. The conversion from the `SavedModel` model to the camera ready format is divided into three steps:
1. Convert to Tensorflow Lite format (`.tflite`) with Edge TPU compatible data types, e.g., by using the supplied `convert_model.py` script
2. Compile the `.tflite` model with the Edge TPU compiler to add Edge TPU compatibility

__All the resulting pre-trained original, converted and compiled models are available in the `/env/models` directory, so any step in the process can be skipped.__

#### From SavedModel to .tflite
The model in this example is saved in the SavedModel format. This is Tensorflow's recommended option, but other formats can be converted to `.tflite` as well. The conversion to `.tflite` is done in Python
using the Tensorflow package's [Tensorflow Lite converter](https://www.tensorflow.org/lite/guide/get_started#tensorflow_lite_converter).

For quantization to 8-bit integer precision, measurements on the network during inference needs to be performed.
Thus, the conversion process does not only require the model but also input data samples, which are provided using
a data generator. These input data samples need to be of the FP32 data type. Running the script below converts a specified SavedModel to a Tensorflow Lite model, given that
the dataset generator function in the script is defined such that it yields data samples that fits our model's inputs.

This script is located in [/env/convert_model.py](env/convert_model.py). We use it to convert our model by executing it with our model path, dataset path and output path supplied as arguments:
```sh
python convert_model.py -i /env/models/saved_model -d /env/data/images/val2017 -o /env/models/converted_model.tflite
```

This process can take a few minutes as the validation dataset is quite large.

#### Compiling for Edge TPU
After conversion to the `.tflite` format, the `.tflite` model needs to be compiled for the Edge TPU. This is done by installing and running the [Edge TPU compiler](https://coral.ai/docs/edgetpu/compiler/#download), as described below. The Edge TPU compilation tools are already installed in the example environment.

**Installation**
```sh
curl https://packages.cloud.google.com/apt/doc/apt-key.gpg | sudo apt-key add -
echo "deb https://packages.cloud.google.com/apt coral-edgetpu-stable main" | sudo tee /etc/apt/sources.list.d/coral-edgetpu.list
sudo apt-get update
sudo apt-get install edgetpu-compiler
```

**Usage**

With the compiler installed, the model can be compiled by running `edgetpu_compiler -s <model_path>`. By default, this will create a new `.tflite` model with the same name as the original model but with a `_edgetpu` suffix. The `-s` flag makes the compiler output additional information on what operations were compiled to be run on the Edge TPU and which, if any, were compiled to run on CPU. If you are seeing the QUANTIZATION and DEQUANTIZATION ops being mapped to the CPU, ensure that you are running a recent version of Tensorflow as newer versions, e.g. 2.3.0, converts these operations to be performed on the Edge TPU.

Our `.tflite` model is compiled for the Edge TPU by running:
```sh
edgetpu_compiler -s -o models models/converted_model.tflite
```

Now there should be a compiled model called `converted_model_edgetpu.tflite` in the `models` directory. We copy this to the `app` directory in order for the model to be packaged with the ACAP when it is built:

```sh
cp models/converted_model_edgetpu.tflite app/
```

## Designing the algorithm's application

To upload the algorithm to the camera, it needs to be packaged as an [ACAP](https://www.axis.com/products/analytics/acap) and compiled. As this ACAP is going to perform inference on images captured by the camera and output a prediction on if there are any persons or cars present in the image, some C code is needed. The ACAP code that is relevant to this example is located in [/env/app/tensorflow_to_larod.c](env/app/tensorflow_to_larod.c). This code is similar to the [vdo-larod](https://github.com/AxisCommunications/acap4-native-sdk-examples/tree/master/vdo-larod) example, with emphasis put on the differences, such as input to and multiple outputs from the model as well as handling these predictions.

In this section we will go over the _rough outline_ of what needs to be done to run inference for our model, but again, the full code is available in [/env/app/tensorflow_to_larod.c](env/app/tensorflow_to_larod.c).

#### Setting up a video stream
First of all, the frame to perform our analytics operation on needs to be retrieved. We do this using the [libvdo](https://www.axis.com/techsup/developer_doc/acap3/3.1/api/vdostream/html/index.html) library. First, the most appropriate available stream for our needs is chosen with the [chooseStreamResolution](env/app/imgprovider.c#L221) method. While any stream could be used, selecting the smallest stream which is still greater or equal to our requested resolution ensures that a minimal amount of time is spent on resizing.

```c
unsigned int streamWidth = 0;
unsigned int streamHeight = 0;
chooseStreamResolution(args.width, args.height, &streamWidth, &streamHeight);
```

With our stream settings known, a stream can be set up. This is done by creating an ImgProvider, which enables fetching frames from our stream. The stream resolution is fed into the [createImgProvider](env/app/imgprovider.c#L95) method, along with the number of frames to have available to the application and the selected [output format](https://www.axis.com/techsup/developer_doc/acap3/3.1/api/vdostream/html/vdo-types_8h.html#a5ed136c302573571bf325c39d6d36246), which in turn returns an ImgProvider.

```c
provider = createImgProvider(streamWidth, streamHeight, 2, VDO_FORMAT_YUV);
```

#### Setting up the larod interface
Next, the [larod](https://www.axis.com/techsup/developer_doc/acap3/3.1/api/larod/html/larod_8h.html) interface needs to be set up. It is through this interface that the model is loaded and inference is performed. The setting up of larod is in part done in the [setupLarod](env/app/tensorflow_to_larod.c#L138) method. This method creates a connection to larod, selects the hardware to use and loads the model.

```c
int larodModelFd = -1;
larodConnection* conn = NULL;
larodModel* model = NULL;
setupLarod(args.chip, larodModelFd, &conn, &model);
```

The tensors inputted to and outputted from larod needs to be stored. To accomplish this, a temporary file will be created for each such tensor. The input to our model is a single 256x256x3 tensor, and as the data type is now INT8, each such value is one byte in size. Thus, with the [createAndMapTmpFile](env/app/tensorflow_to_larod.c#L75) method, a file descriptor with 256x256x3 bytes of allocated space is produced to hold this input tensor. The two outputs of the model are in the same manner allocated a single byte each, as they both output one INT8 value, using the same method.

If some non-Edge TPU operation is included in the model, the associated tensors might be of the e.g., the FP32 data type instead. In this case, this will have to be factored in when choosing how much memory to allocate.

```c
char CONV_INP_FILE_PATTERN[] = "/tmp/larod.in.test-XXXXXX";
char CONV_OUT1_FILE_PATTERN[] = "/tmp/larod.out1.test-XXXXXX";
char CONV_OUT2_FILE_PATTERN[] = "/tmp/larod.out2.test-XXXXXX";
void* larodInputAddr = MAP_FAILED;
void* larodOutput1Addr = MAP_FAILED;
void* larodOutput2Addr = MAP_FAILED;
int larodInputFd = -1;
int larodOutput1Fd = -1;
int larodOutput2Fd = -1;

createAndMapTmpFile(CONV_INP_FILE_PATTERN,  args.width * args.height * CHANNELS, &larodInputAddr, &larodInputFd);
createAndMapTmpFile(CONV_OUT1_FILE_PATTERN, args.outputBytes, &larodOutput1Addr, &larodOutput1Fd);
createAndMapTmpFile(CONV_OUT2_FILE_PATTERN, args.outputBytes, &larodOutput2Addr, &larodOutput2Fd);
```

The input and output tensors to use need to be mapped to the model. This is done using
`larodCreateModelInputs` and `larodCreateModelOutputs` methods. The variables specifying the number of inputs and outputs
to the model are automatically configured according to the inputted model.

```c
size_t numInputs = 0;
size_t numOutputs = 0;
inputTensors = larodCreateModelInputs(model, &numInputs, &error);
outputTensors = larodCreateModelOutputs(model, &numOutputs, &error);
```

Each tensor needs to be mapped to a file descriptor to allow IO. With the file descriptors produced previously,
each tensor is given a mapping to a file descriptor using the `larodSetTensorFd` method.

```c
larodSetTensorFd(inputTensors[0], larodInputFd, &error);
larodSetTensorFd(outputTensors[0], larodOutput1Fd, &error);
larodSetTensorFd(outputTensors[1], larodOutput2Fd, &error);
```

The final stage before inference is creating an inference request for our task.
This is done using the `larodCreateInferenceRequest` method, which is given information on the task
to perform through the model, our tensors and information regarding the number of inputs and outputs.

```c
infReq = larodCreateInferenceRequest(model, inputTensors, numInputs, outputTensors,numOutputs, &error);
```

#### Fetching a frame and performing inference
To get a frame, the `ImgProvider` created earlier is used. A buffer containing the latest image from the pipeline is retrieved by using the `getLastFrameBlocking` method with the created provider. The NV12 data from the buffer is then extracted with the `vdo_buffer_get_data` method.

```c
VdoBuffer* buf = getLastFrameBlocking(provider);
uint8_t* nv12Data = (uint8_t*) vdo_buffer_get_data(buf);
```

Axis cameras output images on the NV12 YUV format. As this is not normally used as input format to deep learning models,
conversion to e.g., RGB might be needed. This can be done using ```libyuv```. However, if performance is a primary objective, training the model to use the YUV format directly should be considered, as each frame conversion takes a few milliseconds. To convert the NV12 stream to RGB, the `convertCropScaleU8yuvToRGB` from `imgconverter` is used, which in turn uses the `libyuv` library.

```c
convertCropScaleU8yuvToRGB(nv12Data, streamWidth, streamHeight, (uint8_t*) larodInputAddr, args.width, args.height);
```


Any other preprocessing steps should be done now, as the inference is next. Using the larod interface, inference is run with the `larodRunInference` method, which outputs the results to the specified output addresses.

```c
larodRunInference(conn, infReq, &error);
```

As we're using multiple outputs, with one file descriptor per output, the respective output will be available at the addresses we associated with the file descriptor. In the case of this example, our two output tensors from the inference can be read at `larodOutput1Addr` and `larodOutput2Addr` respectively. In this example, the resulting probabilities is outputted to the application's log with the `syslog` function.

```c
uint8_t* person_pred = (uint8_t*) larodOutput1Addr;
uint8_t* car_pred = (uint8_t*) larodOutput2Addr;

syslog(LOG_INFO, "Person detected: %.2f%% - Car detected: %.2f%%",
       (float) person_pred[0] / 2.55f, (float) car_pred[0]  / 2.55f);
```

## Building the algorithm's application
A packaging file is needed to compile the ACAP. This is found in [app/manifest.json](app/manifest.json). The noteworthy attribute for this tutorial is the `runOptions` attribute. `runOptions` allows arguments to be given to the ACAP, which in this case is handled by the `argparse` lib. The argument order, defined by [app/argparse.c](app/argparse.c), is `<model_path input_resolution_width input_resolution_height output_size_in_bytes>`. We also need to copy our .tflite model file to the ACAP, and this is done by using the -a flag in the acap-build command in the Dockerfile. The -a flag simply tells the compiler what files to copy to the ACAP.

The ACAP is built to specification by the `Makefile` in [app/Makefile](env/app/Makefile). With the [Makefile](env/app/Makefile) and [manifest.json](env/app/manifest.json) files set up, the ACAP can be built by running the build script in the example environment:

```sh
./build_acap.sh tensorflow-to-larod-acap:1.0
```

After running this script, the `build` directory should have been populated. Inside it is an `.eap` file, which is your stand-alone ACAP build.

## Installing the algorithm's application
To install an ACAP, the `.eap` file in the `build` directory needs to be uploaded to the camera and installed. This can be done through the camera GUI.

Outside of the example environment, extract the `.eap` from the environment by running:
```sh
docker cp <a_name_for_your_env>:/env/build/tensorflow_to_larod_app_1_0_0_armv7hf.eap tensorflow_to_larod.eap
```

where `<a_name_for_your_env>` is the same name as you used to start your environment with the `./run_env.sh` script.
Then go to your camera -> Settings -> Apps -> Add -> Browse to `tensorflow_to_larod.eap` and press Install.


## Running the algorithm
In the Apps view of the camera, press the icon for your ACAP. A window will pop up which allows you to start the application. Press the Start icon to run the algorithm.

With the algorithm started, we can view the output by either pressing "App log" in the same window, or by SSHing into the camera and viewing the log as below:
```sh
tail -f /var/volatile/log/info.log | grep tensorflow_to_larod
```

Placing yourself in the middle of the cameras field of view should ideally make the log read something like:
```sh
[ INFO    ] tensorflow_to_larod[1893]: Person detected: 98.2% - Car detected: 1.98%
```
## License
**[Apache License 2.0](../LICENSE)**


# Future possible improvements
* Interaction with non-neural network operations (eg NMS)
* Custom objects
* 2D output for showing overlay of e.g., pixel classifications
* Different output tensor dimensions for a more realistic use case
* Usage of the @tf.function decorator
* Quantization-aware training
