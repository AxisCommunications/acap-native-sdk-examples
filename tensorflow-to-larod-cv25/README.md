*Copyright (C) 2022, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# From Tensorflow model to larod inference on camera

## Overview

In this example, we look at the process of running a Tensorflow model on
an CV25 AXIS camera. We go through the steps needed from the training of the model
to actually running inference on a camera by interfacing with the
[larod API](https://axiscommunications.github.io/acap-documentation/docs/api/src/api/larod/html/index.html).
This example is somewhat more comprehensive and covers e.g.,
model conversion, model quantization, image formats and creation and use of a model with multiple output tensors in
greater depth than the [larod](../larod) and [vdo-larod](../vdo-larod) examples.

## Table of contents

<!-- ToC GFM -->

- [Prerequisites](#prerequisites)
- [Structure of this example](#structure-of-this-example)
- [Quickstart](#quickstart)
- [Environment for building and training](#environment-for-building-and-training)
- [The example model](#the-example-model)
- [Model training and quantization](#model-training-and-quantization)
- [Model conversion](#model-conversion)
  - [From SavedModel to .tflite](#from-savedmodel-to-tflite)
  - [Compiling for CV25 DLPU](#compiling-for-cv25-dlpu)
- [Designing the application](#designing-the-application)
  - [Setting up a video stream](#setting-up-a-video-stream)
    - [Setting up the larod interface](#setting-up-the-larod-interface)
    - [Fetching a frame and performing inference](#fetching-a-frame-and-performing-inference)
- [Building the algorithm's application](#building-the-algorithms-application)
- [Installing the algorithm's application](#installing-the-algorithms-application)
- [Running the algorithm](#running-the-algorithm)
- [License](#license)
- [Future possible improvements](#future-possible-improvements)

<!-- /ToC -->

## Prerequisites

- An AXIS CV25 camera, e.g., the M3088
- NVIDIA GPU and drivers [compatible with Tensorflow r2.3](https://www.tensorflow.org/install/source#gpu)
- [Docker](https://docs.docker.com/get-docker/)
- [NVIDIA container toolkit](https://docs.nvidia.com/datacenter/cloud-native/container-toolkit/install-guide.html#installing-on-ubuntu-and-debian)
- Access to Ambarella tools

## Structure of this example

```sh
tensorflow_to_larod-cv25
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
│   │   └── tensorflow_to_larod-cv25.c
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
└── models.cv25.sha512
```

- **build_env.sh** - Builds the environment in which this example is run.
- **Dockerfile** - Docker file with the specified Axis toolchain and API container to build the example specified.
- **env/app/argparse.c/h** - Implementation of argument parser, written in C.
- **env/app/imgconverter.c/h** - Implementation of libyuv parts, written in C.
- **env/app/imgprovider.c/h** - Implementation of vdo parts, written in C.
- **env/app/Makefile** - Makefile containing the build and link instructions for building the ACAP application.
- **env/app/manifest.json** - Defines the application and its configuration.
- **env/app/tensorflow_to_larod_cv25.c** - The file implementing the core functionality of the ACAP application.
- **env/build_acap.sh** - Builds the ACAP application and the .eap file.
- **env/convert_model.py** - A script used to convert Tensorflow models to Tensorflow Lite models.
- **env/Dockerfile** - Docker file with the specified Axis toolchain and API container to build the example specified.
- **env/training/model.py** - Defines the Tensorflow model used in this example.
- **env/training/train.py** - Defines the model training procedure of this example.
- **env/training/utils.py** - Contains a datagenerator which specifies how data is loaded to the training process.
- **env/yuv/** - Folder containing patch for building libyuv.
- **run_env.sh** - Runs the environment in which this example is run.
- **models.cv25.sha512** - The file necessary to download a pretrained model.

## Quickstart

The following instructions can be executed to simply run the example. Each step is described in greater detail in the following sections.

1. Build and run the example environment. This mounts the `env` directory into the container:

   ```sh
   ./build_env.sh
   ./run_env.sh <a_name_for_your_env>
   ```

2. Train a Tensorflow model *(optional, pre-trained models available in `/env/models`)*:

   ```sh
   python training/train.py -i data/images/val2017/ -a data/annotations/instances_val2017.json
   ```

3. Quantize the Tensorflow model and convert it to `.tflite`:

   ```sh
   python convert_model.py -i models/saved_model -d data/images/val2017 -o models/converted_model.tflite
   ```

4. Compile the model using the [Ambarella toolchain](https://www.ambarella.com/technology/#cvflow)

   > NOTE > It is required to be an Ambarella partner to access Ambarella tools. Visit [this](https://customer.ambarella.com/ng/login?refid=EAAAALA9%2fIfpWDCn53oQJDd5FKfzsrI0fWXYseVTpgnkJHV1) webpage to access the tools. If you are not an Ambarella partner, you can get login credentials by registering in the same link.

   If you are a partner, visit [this](https://customer.ambarella.com/ng/pre-login) webpage to access the tools.*

5. Compile the ACAP application:

   ```sh
   ./build_acap.sh tensorflow-to-larod-cv25-acap:1.0
   ```

6. In a new terminal, copy the ACAP application `.eap` file from the example environment:

   ```sh
   docker cp <a_name_for_your_env>:/env/build/tensorflow_to_larod_cv25_app_1_0_0_aarch64.eap tensorflow_to_larod_cv25.eap
   ```

7. Install and start the ACAP application on your camera through the GUI
8. SSH to the camera
9. View its log to see the application output:

    ```sh
    tail -f /var/volatile/log/info.log | grep tensorflow_to_larod_cv25
    ```

## Environment for building and training

In this example, we're going to be working within a Docker container environment. This is done to get the correct version of Tensorflow installed, as well as the needed tools. The `run_env.sh` script also mounts the `env` directory to allow for easier interaction with the container. To start the environment, run the build script and then the run script with what you want to name the environment as an argument. The build script forwards the environment variables `http_proxy` and `https_proxy` to the environment to allow proxy setups. The scripts are run as seen below:

```sh
./build_env.sh
./run_env.sh <a_name_for_your_env>
```

The environment can be started without GPU support by supplying the `--no-gpu` flag to the `run_env.sh` script after the environment name.

Note that the MS COCO 2017 validation dataset is downloaded during the building of the environment. This is roughly 1GB in size which means this could take a few minutes to download.

## The example model

In this example, we'll train a simple model with one input and two outputs. The input to the model is a scaled FP32 RGB image of shape (256, 256, 3), while both outputs are scalar values. However, the process is the same regardless of the dimensions or number of inputs or outputs.
The two outputs of the model represent the model's confidences for the presence of `person`  and `car`. **Currently (TF2.3), there is a bug in the `.tflite` conversion which orders the model outputs alphabetically based on their name. For this reason, our outputs are named with A and B prefixes, as to retain them in the order our application expects.**

The model primarily consist of convolutional layers.
These are ordered in a [ResNet architecture](https://en.wikipedia.org/wiki/Residual_neural_network), which eventually feeds into a pooling operation, fully-connected layers and finally the output tensors.
The output values are compressed into the `[0,1]` range by a sigmoid function.

The pre-trained model is trained on the MS COCO 2017 **training** dataset, which is significantly larger than the supplied MS COCO 2017 **validation** dataset. After training it for 10 epochs, it achieves around 80% validation accuracy on the people output and 90% validation accuracy on the car output with 1.6 million parameters, which results in a model file size of 22 MB. This model is saved in Tensorflow's SavedModel format, which is the recommended option, in the `/env/models` directory.

## Model training and quantization

Running the training process yourself is done by executing the following command, where the `-i` flag points to the folder containing the images and the `-a` flag points to the annotation `.json`-file:

 ```sh
 python training/train.py -i /env/data/images/val2017/ -a /env/data/annotations/instances_val2017.json
 ```

If you wish to train the model with the larger COCO 2017 training set and have downloaded it as described in [a previous section](#environment-for-building-and-training), simply substitute `val2017` for `train2017` in the paths in the command above to use that instead.

To get good machine learning performance on low-power devices,
[quantization](https://www.tensorflow.org/model_optimization/guide/quantization/training)
is commonly employed. While machine learning models are commonly
trained using 32-bit floating-point precision numbers, inference can
be done using less precision. This generally results in significantly lower
inference latency and model size with only a slight penalty to the model's
accuracy.

## Model conversion

To use the model on a camera, it needs to be converted. The conversion from the `SavedModel` model to the camera ready format is divided into two steps:

**All the resulting pre-trained original, converted and compiled models are available in the `/env/models` directory, so any step in the process can be skipped.**

### From SavedModel to .tflite

The model in this example is saved in the SavedModel format. This is Tensorflow's recommended option, but other formats can be converted to `.tflite` as well. The conversion to `.tflite` is done in Python
using the Tensorflow package's [Tensorflow Lite converter](https://www.tensorflow.org/lite/guide/get_started#tensorflow_lite_converter).

For quantization to 8-bit integer precision, measurements on the network during inference needs to be performed.
Thus, the conversion process does not only require the model but also input data samples, which are provided using
a data generator. These input data samples need to be of the FP32 data type. Running the script below converts a specified SavedModel to a Tensorflow Lite model, given that the dataset generator function in the script is defined such that it yields data samples that fits our model's inputs.

This script is located in [/env/convert_model.py](env/convert_model.py). We use it to convert our model by executing it with our model path, dataset path and output path supplied as arguments:

```sh
python convert_model.py -i models/saved_model -d data/images/val2017 -o models/converted_model.tflite
```

This process can take a few minutes as the validation dataset is quite large.

### Compiling for CV25 DLPU

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

## Designing the application

To upload the algorithm to the camera, it needs to be packaged as an [ACAP application](https://www.axis.com/products/analytics/acap) and compiled. As this ACAP application is going to perform inference on images captured by the camera and output a prediction on if there are any persons or cars present in the image, some C code is needed. The application code that is relevant to this example is located in [/env/app/tensorflow_to_larod_cv25.c](env/app/tensorflow_to_larod_cv25.c). This code is similar to the [vdo-larod](../vdo-larod) example, with emphasis put on the differences, such as input to and multiple outputs from the model as well as handling these predictions.

In this section we will go over the *rough outline* of what needs to be done to run inference for our model, but again, the full code is available in [/env/app/tensorflow_to_larod_cv25.c](env/app/tensorflow_to_larod_cv25.c).

### Setting up a video stream

First of all, the frame to perform our analytics operation on needs to be retrieved. We do this using the [libvdo](https://axiscommunications.github.io/acap-documentation/docs/api/src/api/vdostream/html/index.html) library. First, the most appropriate available stream for our needs is chosen with the [chooseStreamResolution](env/app/imgprovider.c#L221) method. While any stream could be used, selecting the smallest stream which is still greater or equal to our requested resolution ensures that a minimal amount of time is spent on resizing.

```c
unsigned int streamWidth = 0;
unsigned int streamHeight = 0;
chooseStreamResolution(args.width, args.height, &streamWidth, &streamHeight);
```

With our stream settings known, a stream can be set up. This is done by creating an ImgProvider, which enables fetching frames from our stream. The stream resolution is fed into the [createImgProvider](env/app/imgprovider.c#L95) method, along with the number of frames to have available to the application and the selected [output format](https://axiscommunications.github.io/acap-documentation/docs/api/src/api/vdostream/html/vdo-types_8h.html#a5ed136c302573571bf325c39d6d36246), which in turn returns an ImgProvider.

```c
provider = createImgProvider(streamWidth, streamHeight, 2, VDO_FORMAT_YUV);
```

#### Setting up the larod interface

Next, the [larod](https://axiscommunications.github.io/acap-documentation/docs/api/src/api/larod/html/index.html) interface needs to be set up. It is through this interface that the model is loaded and inference is performed. The setting up of larod is in part done in the [setupLarod](env/app/tensorflow_to_larod.c#L138) method. This method creates a connection to larod, selects the hardware to use and loads the model.

```c
int larodModelFd = -1;
larodConnection* conn = NULL;
larodModel* model = NULL;
setupLarod(args.chip, larodModelFd, &conn, &model);
```

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
convertCropScaleU8yuvToRGB(nv12Data, streamWidth, streamHeight, (uint8_t*) larodInputAddr, args.width, args.height, "RGB-planar");
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

A packaging file is needed to compile the ACAP application. This is found in [app/manifest.json](app/manifest.json). The noteworthy attribute for this tutorial is the `runOptions` attribute. `runOptions` allows arguments to be given to the application, which in this case is handled by the `argparse` lib. The argument order, defined by [app/argparse.c](app/argparse.c), is `<model_path input_resolution_width input_resolution_height output_size_in_bytes>`. We also need to copy our .tflite model file to the ACAP application, and this is done by using the -a flag in the acap-build command in the Dockerfile. The -a flag simply tells the compiler what files to copy to the application.

The ACAP application is built to specification by the `Makefile` in [app/Makefile](env/app/Makefile). With the [Makefile](env/app/Makefile) and [manifest.json](env/app/manifest.json) files set up, the ACAP application can be built by running the build script in the example environment:

```sh
./build_acap.sh tensorflow-to-larod-cv25-acap:1.0
```

After running this script, the `build` directory should have been populated. Inside it is an `.eap` file, which is your stand-alone ACAP application build.

## Installing the algorithm's application

To install an ACAP application, the `.eap` file in the `build` directory needs to be uploaded to the camera and installed. This can be done through the camera GUI.

Outside of the example environment, extract the `.eap` from the environment by running:

```sh
docker cp <a_name_for_your_env>:/env/build/tensorflow_to_larod_cv25_app_1_0_0_aarch64.eap tensorflow_to_larod_cv25.eap
```

where `<a_name_for_your_env>` is the same name as you used to start your environment with the `./run_env.sh` script.
Then go to your camera -> Settings -> Apps -> Add -> Browse to `tensorflow_to_larod_cv25.eap` and press Install.

## Running the algorithm

In the Apps view of the camera, press the icon for your application. A window will pop up which allows you to start the application. Press the Start icon to run the algorithm.

With the algorithm started, we can view the output by either pressing "App log" in the same window, or by SSHing into the camera and viewing the log as below:

```sh
tail -f /var/volatile/log/info.log | grep tensorflow_to_larod_cv25
```

Placing yourself in the middle of the cameras field of view should ideally make the log read something like:

```sh
[ INFO    ] tensorflow_to_larod_cv25[1893]: Person detected: 98.2% - Car detected: 1.98%
```

## License

**[Apache License 2.0](../LICENSE)**

## Future possible improvements

- Interaction with non-neural network operations (eg NMS)
- Custom objects
- 2D output for showing overlay of e.g., pixel classifications
- Different output tensor dimensions for a more realistic use case
- Usage of the @tf.function decorator
- Quantization-aware training
