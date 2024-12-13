*Copyright (C) 2022, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# Object Detection Example

## Overview

This example focuses on the application of object detection on an CV25 Axis camera. A pretrained model [MobileNet SSD v2 (COCO)] is used to detect the location of 90 types of different objects. The model is downloaded through from Tensorflow Hub, and converted using the Ambarella toolchain. The detected objects are saved in /tmp folder for further usage.

## Prerequisites

- CV25 Axis camera equipped with DLPU e.g. M3085
- [Docker](https://docs.docker.com/get-docker/)

## Quickstart

The following instructions can be executed to simply run the example.

1. Compile the ACAP application:

    ```sh
    docker build --tag obj_detect:1.0 .
    docker cp $(docker create obj_detect:1.0):/opt/app ./build
    ```

2. Find the ACAP application `.eap` file

    ```sh
    build/object_detection_app_1_0_0_cv25.eap
    ```

3. Install and start the ACAP application on your camera through the camera web GUI

4. SSH to the camera

5. View its log to see the ACAP application output:

    ```sh
    tail -f /var/volatile/log/info.log | grep object_detection
    ```

## Designing the application

The whole principle is similar to the [vdo-larod-cv25](../vdo-larod-cv25). In this example, the original video stream has a resolution of 640x360, while MobileNet SSD COCO requires an input size of 300x300, so we set up two different streams: one is for MobileNet model, another is used to crop a higher resolution jpg image.
Although the model takes an input of 300x300, the CV25 accelerator expects an input of size multiple of 32. This means that the input needs to be converted to a resolution of 300x320 to satisfy the chip requirements. This is done by adding 20 bytes of padding to each row of the image.
In general, it would be easier to use a model that has already by design an input of size multiple of 32 (typically 320x320 or 640x640).

### Setting up the MobileNet stream

> [!NOTE]
> This example is designed to post-process the output of this specific model. If you want to use your own model, you'll have to adapt the [post-processing](https://github.com/AxisCommunications/acap-native-sdk-examples/blob/main/object-detection-cv25/app/object_detection.c#L891)

There are two methods used to obtain a proper resolution. The [chooseStreamResolution](app/imgprovider.c#L221) method is used to select the smallest stream and assign them into streamWidth and streamHeight.

```c
unsigned int streamWidth = 0;
unsigned int streamHeight = 0;
chooseStreamResolution(inputWidth, inputHeight, &streamWidth, &streamHeight);
```

Then, the [createImgProvider](app/imgprovider.c#L95) method is used to return an ImgProvider with the selected [output format](https://axiscommunications.github.io/acap-documentation/docs/api/src/api/vdostream/html/vdo-types_8h.html#a5ed136c302573571bf325c39d6d36246).

```c
provider = createImgProvider(streamWidth, streamHeight, 2, VDO_FORMAT_YUV);
```

#### Setting up the crop stream

The original resolution `args.raw_width` x `args.raw_height` is used to crop a higher resolution image.

```c
provider_raw = createImgProvider(rawWidth, rawHeight, 2, VDO_FORMAT_YUV);
```

#### Setting up the larod interface

Then similar with [tensorflow-to-larod-cv25](../tensorflow-to-larod-cv25), the [larod](https://axiscommunications.github.io/acap-documentation/docs/api/src/api/larod/html/index.html) interface needs to be set up. The [setupLarod](app/object_detection.c#L329) method is used to create a conncection to larod and select the hardware to use the model.

```c
int larodModelFd = -1;
const char* chipString;
larodConnection* conn = NULL;
larodModel* model = NULL;
setupLarod(chipString, larodModelFd, &conn, &model);
```

The [createAndMapTmpFile](app/object_detection.c#L266) method is used to create temporary files to store the input and output tensors.

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

createAndMapTmpFile(CONV_INP_FILE_PATTERN,(inputWidth + padding) * inputHeight * CHANNELS,
                    &larodInputAddr, &larodInputFd);
createAndMapTmpFile(CONV_PP_FILE_PATTERN, yuyvBufferSize, &ppInputAddr, &ppInputFd);
createAndMapTmpFile(CONV_OUT1_FILE_PATTERN, TENSOR1SIZE, &larodOutput1Addr, &larodOutput1Fd);
createAndMapTmpFile(CONV_OUT2_FILE_PATTERN, TENSOR2SIZE, &larodOutput2Addr, &larodOutput2Fd);
```

In terms of the crop part, another temporary file is created.

```c
char CROP_FILE_PATTERN[] = "/tmp/crop.test-XXXXXX";
void* cropAddr = MAP_FAILED;
int cropFd = -1;

createAndMapTmpFile(CROP_FILE_PATTERN, rawWidth * rawHeight * CHANNELS, &cropAddr, &cropFd);
```

The `larodCreateModelInputs` and `larodCreateModelOutputs` methods map the input and output tensors with the model.

```c
size_t ppInputs = 0;
size_t ppOutputs = 0;
ppInputTensors = larodCreateModelInputs(ppModel, &ppInputs, &error);
ppOutputTensors = larodCreateModelOutputs(ppModel, &ppOutputs, &error);
```

The `larodSetTensorFd` method then maps each tensor to the corresponding file descriptor to allow IO.

```c
larodSetTensorFd(ppInputTensors[0], larodInputFd, &error);
larodSetTensorFd(ppOutputTensors[0], larodOutput1Fd, &error);
larodSetTensorFd(ppOutputTensors[1], larodOutput2Fd, &error);
```

Finally, the `larodCreateJobRequest` method creates an inference request to use the model.

```c
infReq = larodCreateJobRequest(ppModel, ppInputTensors, ppNumInputs, ppOutputTensors, ppNumOutputs, cropMap, &error);
```

#### Fetching a frame and performing inference

By using the `getLastFrameBlocking` method, a  buffer containing the latest image is retrieved from the `ImgProvider` created earlier. Then `vdo_buffer_get_data` method is used to extract NV12 data from the buffer.

```c
VdoBuffer* buf = getLastFrameBlocking(provider);
uint8_t* nv12Data = (uint8_t*) vdo_buffer_get_data(buf);
```

Axis cameras outputs frames on the NV12 YUV format. As this is not normally used as input format to deep learning models,
conversion to e.g., RGB might be needed. This is done by creating a pre-processing job request `ppReq` using the function `larodCreateJobRequest`.

```c
ppReq = larodCreateJobRequest(ppModel, ppInputTensors, ppNumInputs, ppOutputTensors, ppNumOutputs, cropMap, &error);
```

The image data is then converted from NV12 format to interleaved uint8_t RGB format by running the `larodRunJob` function on the above defined pre-processing job request `ppReq`.

```c
larodRunJob(conn, ppReq, &error)
```

As mentioned before, to ensure compatibility with the CV25 device, we apply 20 bytes of padding to make sure that our input has an size multiple of 32 bytes.

```c
padImageWidth(rgb_image, larodInputAddr, inputWidth, inputHeight, padding)
```

By using the `larodRunJob` function on `infReq`, the predictions from the MobileNet model are saved into the specified addresses.

```c
larodRunJob(conn, infReq, &error);
```

There are four outputs from the Object Detection model, and each object's location are described in the form of \[top, left, bottom, right\].

```c
float* locations = (float*) larodOutput1Addr;
float* classes = (float*) larodOutput2Addr;
```

Unlike ARTPEC, the CV25 accelerator lacks the capability to perform bounding-box post-processing independently. Therefore, after the inference, we call the custom `postProcessing`function to execute the post-processing steps.

```c
 postProcessing(locations, classes, numberOfDetections, anchorFile, numberOfClasses,
                       confidenceThreshold, iouThreshold, yScale, xScale, hScale, wScale, boxes);
```

- The post-processing consists of the conversion of `locations` using anchor boxes into bounding boxes with the format `[y_min, x_min, y_max, x_max]`
- The anchor boxes constitute a list of N boxes used as references for the detections.
- The `location` array is represented as a vector with dimensions N*4.
  - Here, N denotes the total number of detections, and the 4 values are `[dy, dx, dh, dw]`.
    - In this context, `dy` and `dx` signify the vertical and horizontal shifts relative to the corresponding anchor box, while `dh` and `dw` represent the scaling of height and width in relation to the anchor box.

After creating the bounding box using the locations and the anchor boxes, non-maxima suppression is applied so that overlapping boxes with lower scores are removed.

If the score is higher than a threshold `args.threshold/100.0`, the results are outputted by the `syslog` function, and the object is cropped and saved into jpg form by `crop_interleaved`, `set_jpeg_configuration`, `buffer_to_jpeg`, `jpeg_to_file` methods.

```c
syslog(LOG_INFO, "Object %d: Classes: %s - Scores: %f - Locations: [%f,%f,%f,%f]",
i, class_name[(int) classes[i]], scores[i], top, left, bottom, right);

unsigned char* crop_buffer = crop_interleaved(cropAddr, args.raw_width, args.raw_height, CHANNELS,
                                          crop_x, crop_y, crop_w, crop_h);

buffer_to_jpeg(crop_buffer, &jpeg_conf, &jpeg_size, &jpeg_buffer);

jpeg_to_file(file_name, jpeg_buffer, jpeg_size);
```

## Building the application

An ACAP application contains a manifest file defining the package configuration.
The file is named `manifest.json.cv25` and can be found in the [app](app)
directory. The Dockerfile will depending on the chip type(see below) copy the
file to the required name format `manifest.json`. The noteworthy attribute for
this tutorial is the `runOptions` attribute which allows arguments to be given
to the application and here is handled by the `argparse` lib. The
argument order, defined by [app/argparse.c](app/argparse.c).

In the Dockerfile a `.bin` model file is downloaded
and added to the ACAP application via the -a flag in the
`acap-build` command.

The application is built to specification by the `Makefile` and `manifest.json`
in the [app](app) directory. Standing in the application directory, run:

> [!NOTE]
>
> Depending on the network your local build machine is connected to, you may need to add proxy
> settings for Docker. See
> [Proxy in build time](https://axiscommunications.github.io/acap-documentation/docs/develop/proxy#proxy-in-build-time).

```sh
docker build --tag obj_detect:1.0 .
docker cp $(docker create obj_detect:1.0):/opt/app ./build
```

The installable `.eap` file is found under:

```sh
build/object_detection_app_1_0_0_cv25.eap
```

## Install and start the application

Browse to the application page of the Axis device:

```sh
http://<AXIS_DEVICE_IP>/index.html#apps
```

- Click on the tab `Apps` in the device GUI
- Enable `Allow unsigned apps` toggle
- Click `(+ Add app)` button to upload the application file
- Browse to the newly built ACAP application `object_detection_app_1_0_0_cv25.eap`
- Click `Install`
- Run the application by enabling the `Start` switch

## Running the application

In the Apps view of the camera, press the icon for your ACAP application. A
window will pop up which allows you to start the application. Press the Start
icon to run the algorithm.

With the algorithm started, we can view the output by either pressing `App log`
in the same window, or connect with SSH into the device and view the log with
the following command:

```sh
tail -f /var/volatile/log/info.log | grep object_detection
```

Depending on selected chip, different output is received. The label file is used for identifying objects.

In the system log the chip is sometimes only mentioned as a string, they are mapped as follows:

| Chips | Larod 1 (int) | Larod 3 (string) |
|-------|--------------|------------------|
| CPU with TensorFlow Lite | 2 | cpu-tflite |
| Google TPU | 4 | google-edge-tpu-tflite |
| Ambarella CVFlow (NN) | 6 | ambarella-cvflow |
| ARTPEC-8 DLPU | 12 | axis-a8-dlpu-tflite |

There are four outputs from MobileNet SSD v2 (COCO) model. The number of detections, cLasses, scores, and locations are shown as below. The four location numbers stand for \[top, left, bottom, right\]. By the way, currently the saved images will be overwritten continuously, so those saved images might not all from the detections of the last frame, if the number of detections is less than previous detection numbers.

```sh
[ INFO    ] object_detection[645]: Object 1: Classes: 2 car - Scores: 0.769531 - Locations: [0.750146,0.086451,0.894765,0.299347]
[ INFO    ] object_detection[645]: Object 2: Classes: 2 car - Scores: 0.335938 - Locations: [0.005453,0.101417,0.045346,0.144171]
[ INFO    ] object_detection[645]: Object 3: Classes: 2 car - Scores: 0.308594 - Locations: [0.109673,0.005128,0.162298,0.050947]
```

The detected objects with a score higher than a threshold are saved into /tmp folder in .jpg form as well.

## License

**[Apache License 2.0](../LICENSE)**
