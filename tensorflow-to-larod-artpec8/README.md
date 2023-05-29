*Copyright (C) 2022, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# Training and converting a model to Tensorflow Lite format

In this tutorial, we look at the process of training a model and converting it to Tensorflow Lite format. To see an example of running an inference on a camera with a trained model, visit the [vdo-larod](../vdo-larod) example.

This tutorial is somewhat more comprehensive and covers e.g.,
model conversion, model quantization and overall creation of a model with multiple output tensors in greater depth than [vdo-larod](../vdo-larod) example.

## Getting started

These instructions will guide you on how to execute the code. Below is the structure and scripts used in the example:

```sh
tensorflow_to_larod-artpec8
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

<APP_IMAGE> is the name to tag the image with, e.g., tf-model-training-a8:1.0

Copy the model from the container image to a local directory:

```sh

docker cp $(docker create <APP_IMAGE>):/env/training/models/converted_model.tflite ./<DESIRED_PATH>/converted_model.tflite

```

Note that the MS COCO 2017 validation dataset is downloaded during the building of the environment. This is roughly 1GB in size which means this could take a few minutes to download.

If your machine doesn't have the hardware requisites, like not enough GPU to train or not enough storage to save the training data, you can take a look at [train-tensorflow-model-using-aws](https://github.com/AxisCommunications/acap-sdk-tools/tree/main/train-tensorflow-model-using-aws), a tool to automatise model training with AWS.

### The example model

In this tutorial, we'll train a simple model with one input and two outputs. The input to the model is a FP32 RGB image scaled to the [0, 1] range and of shape `(480, 270, 3)`.
The output of the model are two separate tensors of shape `(1,)`, representing the model's confidences for the presence of `person`  and `car`. The outputs are configured as such, and not as one tensor with a SoftMax activation, in order to demonstrate how to use multiple outputs.
However, the general process of making a camera-compatible model is the same irrespective of the dimensions or number of inputs or outputs.

The model primarily consist of convolutional layers.
These are ordered in a [ResNet architecture](https://en.wikipedia.org/wiki/Residual_neural_network), which eventually feeds into a pooling operation, fully-connected layers and finally the output tensors.
The output values are compressed into the `[0,1]` range by a sigmoid function.

Notable with the model is the specifics of the ResNet blocks.
In order to produce a model with BatchNormalization layers that are fused with the convolutional layers during the conversion to `.tflite` in Tensorflow 1.15, a few modifications need to be made.
Specifically, the convolutional layers need to not use bias, e.g., for Keras Conv2D layers have the `use_bias=False` parameter set, and the layer order needs to be: `convolutional layer -> batch normalization -> relu`.
This will "fold" , or "fuse", the batch normalization, which increases performance.

The pre-trained model is trained on the MS COCO 2017 **training** dataset, which is significantly larger than the supplied MS COCO 2017 **validation** dataset. After training it for 8 epochs and fine-tuning the model with quantization for 4 epochs, it achieves around 85% validation accuracy on both the people output and the car output with 6.6 million parameters. This model is saved in the frozen graph format in the `/env/output_models` directory.

### Model training and quantization

The following commmand in the Dockerfile starts the training process, where the `-i` flag points to the folder containing the images and the `-a` flag points to the annotation `.json`-file:

 ```sh
 python training/train.py -i data/images/val2017/ -a data/annotations/instances_val2017.json
 ```

There are also additional flags `-e` and `-t` that point to number of epochs the model needs to be run for training and fine-tuning respectively. The default values for training and fine-tuning the model are set to 8 and 4 epochs each but could be modified accordingly. If the model is desired to be trained and fine-tuned for 2 epochs each, the above flags can be added as shown below:

 ```sh
 python training/train.py -i data/images/val2017/ -a data/annotations/instances_val2017.json -e 2 -t 2
 ```

If you wish to train the model with the larger COCO 2017 training set and have downloaded it as described in earlier, simply substitute `val2017` for `train2017` in the paths in the command above to use that instead.

To get good machine learning performance on low-power devices,
[quantization](https://www.tensorflow.org/model_optimization/guide/quantization/training)
is commonly employed. While machine learning models are commonly
trained using 32-bit floating-point precision numbers, inference can
be done using less precision. This generally results in significantly lower
inference latency and model size with only a slight penalty to the model's
accuracy.

As noted previously, this tutorial produces a model for an ARTPEC-8 camera.
The camera DLPU uses INT8 precision, which means that the model will need to be quantized from
32-bit floating-point (FP32) precision to 8-bit integer (INT8) precision. The quantization can be done either
during training ([quantization-aware training](https://www.tensorflow.org/model_optimization/guide/quantization/training)) or
after training ([post-training quantization](https://www.tensorflow.org/lite/performance/post_training_quantization)).

The ARTPEC-8 DLPU uses per-tensor quantization, as opposed to per-axis quantization. This needs to be taken into account when
choosing how to train the model, as the per-tensor quantization scheme strongly favours quantization-aware training. Because
of this, quantization-aware training is used in the [training script](env/training/train.py) for this tutorial.
Currently, per-tensor quantization is not supported in Tensorflow 2.x, which is why the `tf.contrib.quantize` module from Tensorflow 1.15 is used to perform the quantization-aware training.

Per-axis quantized models can be used, but at a penalty.
Using a model that is per-axis quantized will perform inference significantly slower than an equivalent per-tensor quantized model.
For this reason, it is not advised to use such a per-axis quantized model if inference latency is important to the application.

The training used in this tutorial is divided into three steps. First, the model is initiated and trained
without any quantization. Next, quantization nodes that learn the quantization parameters for each tensor are added to the model
and the model is fine-tuned with these nodes present, which is the quantization-aware training part of the training process.
Finally, to configure the model for inference, the quantization nodes are set up to perform evaluation, as opposed to training.
With this done, the model is exported in its frozen graph state and ready to be converted to `.tflite`.

### Model conversion

To use the model on a camera, it needs to be in the `.tflite` format. The model that was trained in this tutorial was
saved in the SavedModel format. This is Tensorflow's recommended format, but other formats can be
converted to `.tflite` as well. The conversion to `.tflite` is done in Python
using the Tensorflow package's
[Tensorflow Lite converter](https://www.tensorflow.org/lite/guide/get_started#tensorflow_lite_converter).

A script used for converting with the Tensorflow Lite converter is located in [env/convert_model.py](env/convert_model.py).
The following command in the Dockerfile is the second step, which converts the model to Tensorflow Lite format (`.tflite`):

```sh
python convert_model.py -i models/trained_model.pb -o training/models/converted_model.tflite
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
