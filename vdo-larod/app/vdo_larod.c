/**
 * Copyright (C) 2021, Axis Communications AB, Lund, Sweden
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include "imgprovider.h"
#include "larod.h"
#include "utility-functions.h"
#include "vdo-frame.h"
#include "vdo-types.h"

volatile sig_atomic_t stopRunning = false;

/**
 * brief Invoked on SIGINT. Makes app exit cleanly asap if invoked once, but
 * forces an immediate exit without clean up if invoked at least twice.
 *
 * param sig What signal has been sent.
 */
static void sigintHandler(int sig) {
    if (stopRunning) {
        syslog(LOG_INFO, "Interrupted again, exiting immediately without clean up.");

        signal(sig, SIG_DFL);
        raise(sig);

        return;
    }

    syslog(LOG_INFO, "Interrupted, starting graceful termination of app. Another "
           "interrupt signal will cause a forced exit.");

    // Tell the main thread to stop running inferences asap.
    stopRunning = true;
}

/**
 * brief Sets up and configures a connection to larod, and loads a model.
 *
 * Opens a connection to larod, which is tied to larodConn. After opening a
 * larod connection the chip specified by larodChip is set for the
 * connection. Then the model file specified by larodModelFd is loaded to the
 * chip, and a corresponding larodModel object is tied to model.
 *
 * param larodChip Specifier for which larod chip to use.
 * param larodModelFd Fd for a model file to load.
 * param larodConn Pointer to a larod connection to be opened.
 * param model Pointer to a larodModel to be obtained.
 * return False if error has occurred, otherwise true.
 */
static bool setupLarod(const char* chipString, const int larodModelFd,
                       larodConnection** larodConn, larodModel** model) {
    larodError* error = NULL;
    larodConnection* conn = NULL;
    larodModel* loadedModel = NULL;
    bool ret = false;

    // Set up larod connection.
    if (!larodConnect(&conn, &error)) {
        syslog(LOG_ERR, "%s: Could not connect to larod: %s", __func__, error->msg);
        goto end;
    }

    // List available chip id:s
    size_t numDevices = 0;
    syslog(LOG_INFO, "Available chip IDs:");
    const larodDevice** devices;
    devices = larodListDevices(conn, &numDevices, &error);
    for (size_t i = 0; i < numDevices; ++i) {
            syslog(LOG_INFO, "%s: %s", "Chip", larodGetDeviceName(devices[i], &error));;
        }
    const larodDevice* dev = larodGetDevice(conn, chipString, 0, &error);
    loadedModel = larodLoadModel(conn, larodModelFd, dev, LAROD_ACCESS_PRIVATE,
                                 "Vdo Example App Model", NULL, &error);
    if (!loadedModel) {
        syslog(LOG_ERR, "%s: Unable to load model: %s", __func__, error->msg);
        goto error;
    }
    *larodConn = conn;
    *model = loadedModel;

    ret = true;

    goto end;

error:
    if (conn) {
        larodDisconnect(&conn, NULL);
    }

end:
    if (error) {
        larodClearError(&error);
    }

    return ret;
}

/**
 * brief Main function that starts a stream with different options.
 */
int main(int argc, char** argv) {
    // Hardcode to use three image "color" channels (eg. RGB).
    const unsigned int CHANNELS = 3;

    // Name patterns for the temp file we will create.
    char CONV_PP_FILE_PATTERN[] = "/tmp/larod.pp.test-XXXXXX";
    char CONV_INP_FILE_PATTERN[] = "/tmp/larod.in.test-XXXXXX";
    char CONV_OUT1_FILE_PATTERN[] = "/tmp/larod.out1.test-XXXXXX";
    char CONV_OUT2_FILE_PATTERN[] = "/tmp/larod.out2.test-XXXXXX";

    bool ret = false;
    ImgProvider_t* provider = NULL;
    larodError* error = NULL;
    larodConnection* conn = NULL;
    larodMap* ppMap = NULL;
    larodMap* cropMap = NULL;
    larodModel* ppModel = NULL;
    larodModel* model = NULL;
    larodTensor** ppInputTensors = NULL;
    size_t ppNumInputs = 0;
    larodTensor** ppOutputTensors = NULL;
    size_t ppNumOutputs = 0;
    larodTensor** inputTensors = NULL;
    size_t numInputs = 0;
    larodTensor** outputTensors = NULL;
    size_t numOutputs = 0;
    larodJobRequest* ppReq = NULL;
    larodJobRequest* infReq = NULL;
    void* ppInputAddr = MAP_FAILED;
    void* larodInputAddr = MAP_FAILED;
    void* larodOutput1Addr = MAP_FAILED;
    void* larodOutput2Addr = MAP_FAILED;
    size_t outputBufferSize = 0;
    int larodModelFd = -1;
    int ppInputFd = -1;
    int larodInputFd = -1;
    int larodOutput1Fd = -1;
    int larodOutput2Fd = -1;
    const char* chipString = argv[1];
    const char* modelFile = argv[2];
    const int inputWidth = atoi(argv[3]);
    const int inputHeight = atoi(argv[4]);
    const int numRounds = atoi(argv[5]);

    // Open the syslog to report messages for "vdo_larod"
    openlog("vdo_larod", LOG_PID|LOG_CONS, LOG_USER);
    syslog(LOG_INFO, "Starting %s", argv[0]);
    // Register an interrupt handler which tries to exit cleanly if invoked once
    // but exits immediately if further invoked.
    signal(SIGINT, sigintHandler);

    if (argc != 6) {
        syslog(LOG_ERR, "Invalid number of arguments. Required arguments are: "
                        "INF_CHIP MODEL_PATH WIDTH HEIGHT NUM_ROUNDS");
        goto end;
    }

    // Create video stream provider
    unsigned int streamWidth = 0;
    unsigned int streamHeight = 0;
    if (!chooseStreamResolution(inputWidth, inputHeight, &streamWidth,
                                &streamHeight)) {
        syslog(LOG_ERR, "%s: Failed choosing stream resolution", __func__);
        goto end;
    }

    syslog(LOG_INFO, "Creating VDO image provider and creating stream %d x %d",
           streamWidth, streamHeight);
    provider = createImgProvider(streamWidth, streamHeight, 2, VDO_FORMAT_YUV);
    if (!provider) {
        syslog(LOG_ERR, "%s: Could not create image provider", __func__);
        goto end;
    }

    // Calculate crop image
    // 1. The crop area shall fill the input image either horizontally or
    //    vertically.
    // 2. The crop area shall have the same aspect ratio as the output image.
    syslog(LOG_INFO, "Calculate crop image");
    float destWHratio = (float) inputWidth / (float) inputHeight;
    float cropW = (float) streamWidth;
    float cropH = cropW / destWHratio;
    if (cropH > (float) streamHeight) {
        cropH = (float) streamHeight;
        cropW = cropH * destWHratio;
    }
    unsigned int clipW = (unsigned int)cropW;
    unsigned int clipH = (unsigned int)cropH;
    unsigned int clipX = (streamWidth - clipW) / 2;
    unsigned int clipY = (streamHeight - clipH) / 2;
    syslog(LOG_INFO, "Crop VDO image X=%d Y=%d (%d x %d)", clipX, clipY, clipW, clipH);

    // Create preprocessing maps
    syslog(LOG_INFO, "Create preprocessing maps");
    ppMap = larodCreateMap(&error);
    if (!ppMap) {
        syslog(LOG_ERR, "Could not create preprocessing larodMap %s", error->msg);
        goto end;
    }
    if (!larodMapSetStr(ppMap, "image.input.format", "nv12", &error)) {
        syslog(LOG_ERR, "Failed setting preprocessing parameters: %s", error->msg);
        goto end;
    }
    if (!larodMapSetIntArr2(ppMap, "image.input.size", streamWidth, streamHeight, &error)) {
        syslog(LOG_ERR, "Failed setting preprocessing parameters: %s", error->msg);
        goto end;
    }
    if(chipString != "ambarella-cvflow"){
        if (!larodMapSetStr(ppMap, "image.output.format", "rgb-interleaved", &error)) {
            syslog(LOG_ERR, "Failed setting preprocessing parameters: %s", error->msg);
            goto end;
        }
    } else {
        if (!larodMapSetStr(ppMap, "image.output.format", "rgb-planar", &error)) {
            syslog(LOG_ERR, "Failed setting preprocessing parameters: %s", error->msg);
            goto end;
        }
    }
    if (!larodMapSetIntArr2(ppMap, "image.output.size", inputWidth, inputHeight, &error)) {
        syslog(LOG_ERR, "Failed setting preprocessing parameters: %s", error->msg);
        goto end;
    }
    cropMap = larodCreateMap(&error);
    if (!cropMap) {
        syslog(LOG_ERR, "Could not create preprocessing crop larodMap %s", error->msg);
        goto end;
    }
    if (!larodMapSetIntArr4(cropMap, "image.input.crop", clipX, clipY, clipW, clipH, &error)) {
        syslog(LOG_ERR, "Failed setting preprocessing parameters: %s", error->msg);
        goto end;
    }

    // Create larod models
    syslog(LOG_INFO, "Create larod models");
    larodModelFd = open(modelFile, O_RDONLY);
    if (larodModelFd < 0) {
        syslog(LOG_ERR, "Unable to open model file %s: %s", modelFile,
               strerror(errno));
        goto end;
    }


    syslog(LOG_INFO, "Setting up larod connection with chip %s and model file %s", chipString, modelFile);
    if (!setupLarod(chipString, larodModelFd, &conn, &model)) {
        goto end;
    }

    // Use libyuv as image preprocessing backend
    const char* larodLibyuvPP = "cpu-proc";
    const larodDevice* dev_pp;
    dev_pp = larodGetDevice(conn, larodLibyuvPP, 0, &error);
    ppModel = larodLoadModel(conn, -1, dev_pp, LAROD_ACCESS_PRIVATE, "", ppMap, &error);
    if (!ppModel) {
            syslog(LOG_ERR, "Unable to load preprocessing model with chip %s: %s", larodLibyuvPP, error->msg);
            goto end;
    } else {
           syslog(LOG_INFO, "Loading preprocessing model with chip %s", larodLibyuvPP);
    }

    // Create input/output tensors
    syslog(LOG_INFO, "Create input/output tensors");
    ppInputTensors = larodCreateModelInputs(ppModel, &ppNumInputs, &error);
    if (!ppInputTensors) {
        syslog(LOG_ERR, "Failed retrieving input tensors: %s", error->msg);
        goto end;
    }
    ppOutputTensors = larodCreateModelOutputs(ppModel, &ppNumOutputs, &error);
    if (!ppOutputTensors) {
        syslog(LOG_ERR, "Failed retrieving output tensors: %s", error->msg);
        goto end;
    }
    inputTensors = larodCreateModelInputs(model, &numInputs, &error);
    if (!inputTensors) {
        syslog(LOG_ERR, "Failed retrieving input tensors: %s", error->msg);
        goto end;
    }
    // This app only supports 1 input tensor right now.
    if (numInputs != 1) {
        syslog(LOG_ERR, "Model has %zu inputs, app only supports 1 input tensor.",
               numInputs);
        goto end;
    }
    outputTensors = larodCreateModelOutputs(model, &numOutputs, &error);
    if (!outputTensors) {
        syslog(LOG_ERR, "Failed retrieving output tensors: %s", error->msg);
        goto end;
    }
    // This app only supports 1 output tensor right now.
    if (numOutputs != 2) {
        syslog(LOG_ERR, "Model has %zu outputs, app only supports 2 output tensors.",
               numOutputs);
        goto end;
    }

    // Determine tensor buffer sizes
    syslog(LOG_INFO, "Determine tensor buffer sizes");
    const larodTensorPitches* ppInputPitches = larodGetTensorPitches(ppInputTensors[0], &error);
    if (!ppInputPitches) {
        syslog(LOG_ERR, "Could not get pitches of tensor: %s", error->msg);
        goto end;
    }
    size_t yuyvBufferSize = ppInputPitches->pitches[0];
    const larodTensorPitches* ppOutputPitches = larodGetTensorPitches(ppOutputTensors[0], &error);
    if (!ppOutputPitches) {
        syslog(LOG_ERR, "Could not get pitches of tensor: %s", error->msg);
        goto end;
    }
    size_t rgbBufferSize = ppOutputPitches->pitches[0];
    size_t expectedSize = inputWidth * inputHeight * CHANNELS;
    if (expectedSize != rgbBufferSize) {
        syslog(LOG_ERR, "Expected video output size %d, actual %d", expectedSize, rgbBufferSize);
        goto end;
    }
    const larodTensorPitches* outputPitches = larodGetTensorPitches(outputTensors[0], &error);
    if (!outputPitches) {
        syslog(LOG_ERR, "Could not get pitches of tensor: %s", error->msg);
        goto end;
    }
    outputBufferSize = outputPitches->pitches[0];

    // Allocate memory for input/output buffers
    syslog(LOG_INFO, "Allocate memory for input/output buffers");
    if (!createAndMapTmpFile(CONV_PP_FILE_PATTERN, yuyvBufferSize,
                             &ppInputAddr, &ppInputFd)) {
        goto end;
    }
    if (!createAndMapTmpFile(CONV_INP_FILE_PATTERN,
                             inputWidth * inputHeight * CHANNELS,
                             &larodInputAddr, &larodInputFd)) {
        goto end;
    }
    if (!createAndMapTmpFile(CONV_OUT1_FILE_PATTERN, 4,
                             &larodOutput1Addr, &larodOutput1Fd)) {
        goto end;
    }
    if (!createAndMapTmpFile(CONV_OUT2_FILE_PATTERN, 4,
                             &larodOutput2Addr, &larodOutput2Fd)) {
        goto end;
    }

    // Connect tensors to file descriptors
    syslog(LOG_INFO, "Connect tensors to file descriptors");
    if (!larodSetTensorFd(ppInputTensors[0], ppInputFd, &error)) {
        syslog(LOG_ERR, "Failed setting input tensor fd: %s", error->msg);
        goto end;
    }
    if (!larodSetTensorFd(ppOutputTensors[0], larodInputFd, &error)) {
        syslog(LOG_ERR, "Failed setting input tensor fd: %s", error->msg);
        goto end;
    }
    if (!larodSetTensorFd(inputTensors[0], larodInputFd, &error)) {
        syslog(LOG_ERR, "Failed setting input tensor fd: %s", error->msg);
        goto end;
    }
    if (!larodSetTensorFd(outputTensors[0], larodOutput1Fd, &error)) {
        syslog(LOG_ERR, "Failed setting output tensor fd: %s", error->msg);
        goto end;
    }

    if (!larodSetTensorFd(outputTensors[1], larodOutput2Fd, &error)) {
        syslog(LOG_ERR, "Failed setting output tensor fd: %s", error->msg);
        goto end;
    }

    // Create job requests
    syslog(LOG_INFO, "Create job requests");
    ppReq = larodCreateJobRequest(ppModel, ppInputTensors, ppNumInputs,
        ppOutputTensors, ppNumOutputs, cropMap, &error);
    if (!ppReq) {
        syslog(LOG_ERR, "Failed creating preprocessing job request: %s", error->msg);
        goto end;
    }

    // App supports only one input/output tensor.
    infReq = larodCreateJobRequest(model, inputTensors, 1, outputTensors,
                                         2, NULL, &error);
    if (!infReq) {
        syslog(LOG_ERR, "Failed creating inference request: %s", error->msg);
        goto end;
    }

    syslog(LOG_INFO, "Start fetching video frames from VDO");
    if (!startFrameFetch(provider)) {
        goto end;
    }

    for (unsigned int i = 0; i < numRounds && !stopRunning; i++) {
        struct timeval startTs, endTs;
        unsigned int elapsedMs = 0;

        // Get latest frame from image pipeline.
        VdoBuffer* buf = getLastFrameBlocking(provider);
        if (!buf) {
            goto end;
        }

        // Get data from latest frame.
        uint8_t* nv12Data = (uint8_t*) vdo_buffer_get_data(buf);

        // Covert image data from NV12 format to interleaved uint8_t RGB format
        gettimeofday(&startTs, NULL);
        memcpy(ppInputAddr, nv12Data, yuyvBufferSize);
        if (!larodRunJob(conn, ppReq, &error)) {
            syslog(LOG_ERR, "Unable to run job to preprocess model: %s (%d)",
                   error->msg, error->code);
            goto end;
        }
        gettimeofday(&endTs, NULL);

        elapsedMs = (unsigned int) (((endTs.tv_sec - startTs.tv_sec) * 1000) +
                                    ((endTs.tv_usec - startTs.tv_usec) / 1000));
        syslog(LOG_INFO, "Converted image in %u ms", elapsedMs);

        // Save the RGB image as a PPM file
        const char* filename = "/tmp/output.ppm";
        saveRgbImageAsPpm(larodInputAddr, inputWidth, inputHeight, filename);

        // Since larodOutputAddr points to the beginning of the fd we should
        // rewind the file position before each job.
        if (lseek(larodOutput1Fd, 0, SEEK_SET) == -1) {
            syslog(LOG_ERR, "Unable to rewind output file position: %s",
                   strerror(errno));
            goto end;
        }

        if (lseek(larodOutput2Fd, 0, SEEK_SET) == -1) {
            syslog(LOG_ERR, "Unable to rewind output file position: %s",
                   strerror(errno));
            goto end;
        }

        gettimeofday(&startTs, NULL);
        if (!larodRunJob(conn, infReq, &error)) {
            syslog(LOG_ERR, "Unable to run inference on model %s: %s (%d)",
                   modelFile, error->msg, error->code);
            goto end;
        }
        gettimeofday(&endTs, NULL);

        elapsedMs = (unsigned int) (((endTs.tv_sec - startTs.tv_sec) * 1000) +
                                    ((endTs.tv_usec - startTs.tv_usec) / 1000));
        syslog(LOG_INFO, "Ran inference for %u ms", elapsedMs);

        if (strcmp(chipString, "ambarella-cvflow") != 0){
            uint8_t* person_pred = (uint8_t*) larodOutput1Addr;
            uint8_t* car_pred = (uint8_t*) larodOutput2Addr;

            syslog(LOG_INFO, "Person detected: %.2f%% - Car detected: %.2f%%",
                (float) person_pred[0] / 2.55f, (float) car_pred[0]  / 2.55f);
        } else {
            uint8_t* car_pred = (uint8_t*) larodOutput1Addr;
            uint8_t* person_pred = (uint8_t*) larodOutput2Addr;
            float float_score_car = *((float*) car_pred);
            float float_score_person  = *((float*) person_pred);
            syslog(LOG_INFO, "Person detected: %.2f%% - Car detected: %.2f%%",
                float_score_person*100, float_score_car*100);
        }

        // Release frame reference to provider.
        returnFrame(provider, buf);
    }

    syslog(LOG_INFO, "Stop streaming video from VDO");
    if (!stopFrameFetch(provider)) {
        goto end;
    }

    ret = true;

end:
    if (provider) {
        destroyImgProvider(provider);
    }
    // Only the model handle is released here. We count on larod service to
    // release the privately loaded model when the session is disconnected in
    // larodDisconnect().
    larodDestroyMap(&ppMap);
    larodDestroyMap(&cropMap);
    larodDestroyModel(&ppModel);
    larodDestroyModel(&model);
    if (conn) {
        larodDisconnect(&conn, NULL);
    }
    if (larodModelFd >= 0) {
        close(larodModelFd);
    }
    if (ppInputAddr != MAP_FAILED) {
        munmap(ppInputAddr, inputWidth * inputHeight * CHANNELS);
    }
    if (ppInputFd >= 0) {
        close(ppInputFd);
    }
    if (larodInputAddr != MAP_FAILED) {
        munmap(larodInputAddr, inputWidth * inputHeight * CHANNELS);
    }
    if (larodInputFd >= 0) {
        close(larodInputFd);
    }
    if (larodOutput1Addr != MAP_FAILED) {
        munmap(larodOutput1Addr, 4);
    }

    if (larodOutput2Addr != MAP_FAILED) {
        munmap(larodOutput2Addr, 4);
    }
    if (larodOutput1Fd >= 0) {
        close(larodOutput1Fd);
    }

    if (larodOutput2Fd >= 0) {
        close(larodOutput2Fd);
    }

    larodDestroyJobRequest(&ppReq);
    larodDestroyJobRequest(&infReq);
    larodDestroyTensors(conn, &inputTensors, numInputs, &error);
    larodDestroyTensors(conn, &outputTensors, numOutputs, &error);
    larodClearError(&error);

    syslog(LOG_INFO, "Exit %s", argv[0]);
    return ret ? EXIT_SUCCESS : EXIT_FAILURE;
}
