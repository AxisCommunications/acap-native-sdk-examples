*Copyright (C) 2022, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# Training and converting a model to Tensorflow Lite format

In this tutorial, we look at the process of training a model and converting it to Tensorflow Lite format. To see an example of running an inference on a camera with a trained model, visit the [vdo-larod](../vdo-larod) example.

This tutorial is somewhat more comprehensive and covers e.g.,
model conversion, model quantization and overall creation of a model with multiple output tensors in greater depth than [vdo-larod](../vdo-larod) example.

## Getting started

These instructions will guide you on how to execute the code. Below is the structure and scripts used in the example:

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

<APP_IMAGE> is the name to tag the image with, e.g., tf-model-training-a7:1.0

Copy the model from the container image to a local directory:

```sh

docker cp $(docker create <APP_IMAGE>):/env/training/models/converted_model.tflite ./<DESIRED_PATH>/converted_model.tflite

```

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

In this tutorial, we'll train a simple model with one input and two outputs. The input to the model is a scaled FP32 RGB image of shape (256, 256, 3), while both outputs are scalar values. However, the process is the same irrespective of the dimensions or number of inputs or outputs.
The two outputs of the model represent the model's confidences for the presence of `person`  and `car`. **Currently (TF2.3), there is a bug in the `.tflite` conversion which orders the model outputs alphabetically based on their name. For this reason, our outputs are named with A and B prefixes, as to retain them in the order our application expects.**

The model primarily consist of convolutional layers.
These are ordered in a [ResNet architecture](https://en.wikipedia.org/wiki/Residual_neural_network), which eventually feeds into a pooling operation, fully-connected layers and finally the output tensors.
The output values are compressed into the `[0,1]` range by a sigmoid function.

The pre-trained model is trained on the MS COCO 2017 **training** dataset, which is significantly larger than the supplied MS COCO 2017 **validation** dataset. After training it for 10 epochs, it achieves around 80% validation accuracy on the people output and 90% validation accuracy on the car output with 1.6 million parameters, which results in a model file size of 22 MB. This model is saved in Tensorflow's SavedModel format, which is the recommended option, in the `/env/models` directory.

While this example looks at the process from model creation to inference on a camera, other pre-trained models
are available at e.g., <https://www.tensorflow.org/lite/models> and <https://coral.ai/models/>. The models from [coral.ai](https://coral.ai) are pre-compiled to run on the Edge TPU.

When designing your model for an Edge TPU device, you should only use operations that have an Edge TPU implementation. The full list of such operations is available at <https://coral.ai/docs/edgetpu/models-intro/#supported-operations>.

### Model training and quantization

The following commmand in the Dockerfile starts the training process, where the `-i` flag points to the folder containing the images and the `-a` flag points to the annotation `.json`-file:

 ```sh
 python training/train.py -i data/images/val2017/ -a data/annotations/instances_val2017.json
 ```

There is an additional flag `-e` that points to number of epochs the model needs to be run for training. The default value for training the model is set to 10 epoch but could be modified accordingly. If the model is desired to be trained for 2 epochs, the above flag can be added as shown below:

 ```sh
 python training/train.py -i data/images/val2017/ -a data/annotations/instances_val2017.json -e 2
 ```

If you wish to train the model with the larger COCO 2017 training set and have downloaded it as described in earlier, simply substitute `val2017` for `train2017` in the paths in the command above to use that instead.

To get good machine learning performance on low-power devices,
[quantization](https://www.tensorflow.org/model_optimization/guide/quantization/training)
is commonly employed. While machine learning models are commonly
trained using 32-bit floating-point precision numbers, inference can
be done using less precision. This generally results in significantly lower
inference latency and model size with only a slight penalty to the model's
accuracy.

| Chip      | Supported precision  |
|---------- |------------------     |
| Edge TPU  | INT8                  |
| Common CPUs       | FP32, INT8            |
| Common GPUs       | FP32, FP16, INT8      |

As noted in the first chapter, this example uses a camera equipped with an Edge TPU.
As the Edge TPU chip **only** uses INT8 precision, the model will need to be quantized from
32-bit floating-point (FP32) precision to 8-bit integer (INT8) precision. The quantization
will be done during the conversion described in the next section.

The training used in this tutorial is divided into three steps. First, the model is initiated and trained
without any quantization. Next, quantization nodes that learn the quantization parameters for each tensor are added to the model
and the model is fine-tuned with these nodes present, which is the quantization-aware training part of the training process.
Finally, to configure the model for inference, the quantization nodes are set up to perform evaluation, as opposed to training.
With this done, the model is exported in its frozen graph state and ready to be converted to `.tflite`.

### Model conversion

To use the model on a camera, it needs to be converted. The conversion from the `SavedModel` model to the camera ready format is divided into two steps:

1. Convert to Tensorflow Lite format (`.tflite`) with Edge TPU compatible data types, e.g., by using the supplied `convert_model.py` script
2. Compile the `.tflite` model with the Edge TPU compiler to add Edge TPU compatibility

#### From SavedModel to .tflite

The model in this example is saved in the SavedModel format. This is Tensorflow's recommended option, but other formats can be converted to `.tflite` as well. The conversion to `.tflite` is done in Python
using the Tensorflow package's [Tensorflow Lite converter](https://www.tensorflow.org/lite/guide/get_started#tensorflow_lite_converter).

For quantization to 8-bit integer precision, measurements on the network during inference needs to be performed.
Thus, the conversion process does not only require the model but also input data samples, which are provided using
a data generator. These input data samples need to be of the FP32 data type. Running the script below converts a specified SavedModel to a Tensorflow Lite model, given that the dataset generator function in the script is defined such that it yields data samples that fits our model's inputs.

This script is located in [training/env/convert_model.py](env/convert_model.py). The following command in the Dockerfile is the second step, which converts the model to Tensorflow Lite format (`.tflite`):

```sh
python convert_model.py -i models/saved_model -d data/images/val2017 -o models/converted_model.tflite
```

This process can take a few minutes as the validation dataset is quite large.

#### Compiling for Edge TPU

After conversion to the `.tflite` format, the `.tflite` model needs to be compiled for the Edge TPU. This is done by installing and running the [Edge TPU compiler](https://coral.ai/docs/edgetpu/compiler/#download), as described below. The Edge TPU compilation tools are already installed in the example environment. The following command in the Dockerfile is the third step, which compiles the model for Edge TPU:

```sh
edgetpu_compiler -s -o training/models training/models/converted_model.tflite
```

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
