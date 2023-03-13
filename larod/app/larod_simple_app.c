/**
 * Copyright (C) 2018, Axis Communications AB, Lund, Sweden
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * This application connects to larod and loads a model, runs inference
 * on it and then finally deletes the loaded model from larod.
 *
 * The application expects its parameters on the command line in the following
 * order: MODEL_FILE INPUT_FILE.
 *
 * The first two parameters are expected to be file names and the output tensor
 * size is expected to be a positive integer (given in either decimal,
 * hexadecimal or octal notation). The output will be written to current
 * directory with suffix ".out".
 *
 * Suppose that you have done through the steps of installation. Then with the
 * sample input and model that comes shipped with this app you would go to
 * /usr/local/packages/larod_simple_app on your device and then for
 * example run:
 *     ./larod_simple_app \
 *         model/mobilenet_v1_1.0_224_quant.tflite \
 *         input/goldfish_224x224_uint8_RGB.bin
 *
 * To interpret the output you could (off device) run the command:
 * od -A d -t u1 -v -w1 <name of output file> | sort -n -k 2
 *
 * The highest matched classes will be on the bottom of the list that's printed.
 * You can match these classes with
 * /usr/local/packages/larod_simple_app/model/labels_mobilenet_quant_v1_224.txt
 * to see that indeed a goldfish was recognized.
 *
 * Please note that this app only supports models with one input and one output
 * tensor, whereas larod itself supports any number of either.
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

#include "larod.h"

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
    bool ret = false;
    larodError* error = NULL;
    larodConnection* conn = NULL;
    larodModel* model = NULL;
    larodTensor** inputTensors = NULL;
    size_t numInputs = 0;
    larodTensor** outputTensors = NULL;
    size_t numOutputs = 0;
    larodJobRequest* infReq = NULL;
    int larodModelFd = -1;
    FILE* fpInput = NULL;
    FILE* fpOutput = NULL;
    char * outputFile = NULL;

    if (argc != 3) {
        syslog(LOG_ERR, "ERROR: Invalid number of arguments\n"
                        "Usage: larod_simple_app MODEL_FILE INPUT_FILE\n");
        goto end;
    }

    const char* chipString = "cpu-tflite";
    const char* modelFile = argv[1];
    const char* inputFile = argv[2];

    // Create larod models
    syslog(LOG_INFO, "Create larod models");
    larodModelFd = open(modelFile, O_RDONLY);
    if (larodModelFd < 0) {
        syslog(LOG_ERR, "Unable to open model file %s: %s", inputFile,
               strerror(errno));
        goto end;
    }


    syslog(LOG_INFO, "Setting up larod connection with chip %s, model %s", chipString, inputFile);
    if (!setupLarod(chipString, larodModelFd, &conn, &model)) {
        goto end;
    }

    fpInput = fopen(inputFile, "rb");
    if (!fpInput) {
        syslog(LOG_ERR, "ERROR: Could not open input file %s: %s\n", inputFile,
                strerror(errno));
        goto end;
    }

    const int larodInputFd = fileno(fpInput);
    if (larodInputFd < 0) {
        syslog(LOG_ERR,
                "ERROR: Could not get file descriptor for input file: %s\n",
                strerror(errno));
        goto end;
    }

    if (!outputFile) {
        outputFile = strcat(inputFile, ".out");
    }
    fpOutput = fopen(outputFile, "wb");
    if (!fpOutput) {
        syslog(LOG_ERR, "ERROR: Could not open output file %s: %s\n",
                outputFile, strerror(errno));
        goto end;
    }

    const int larodOutputFd = fileno(fpOutput);
    if (larodOutputFd < 0) {
        syslog(LOG_ERR,
                "ERROR: Could not get file descriptor for output file: %s\n",
                strerror(errno));
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
    if (numOutputs != 1) {
        syslog(LOG_ERR, "Model has %zu outputs, app only supports 1 output tensor.",
               numOutputs);
        goto end;
    }
    // Connect tensors to file descriptors
    syslog(LOG_INFO, "Connect tensors to file descriptors");
    if (!larodSetTensorFd(inputTensors[0], larodInputFd, &error)) {
        syslog(LOG_ERR, "Failed setting input tensor fd: %s", error->msg);
        goto end;
    }
    if (!larodSetTensorFd(outputTensors[0], larodOutputFd, &error)) {
        syslog(LOG_ERR, "Failed setting output tensor fd: %s", error->msg);
        goto end;
    }

    // Create job requests
    syslog(LOG_INFO, "Create job requests");
    infReq = larodCreateJobRequest(model, inputTensors, 1, outputTensors,
                                   1, NULL, &error);
    if (!infReq)
    {
        syslog(LOG_ERR, "Failed creating inference request: %s", error->msg);
        goto end;
    }

    if (!larodRunJob(conn, infReq, &error)) {
          syslog(LOG_ERR, "Unable to run inference on model %s: %s (%d)",
                   modelFile, error->msg, error->code);
            goto end;
    }

    syslog(LOG_INFO,"Output written to %s\n", outputFile);

    ret = true;

end:
    larodDestroyModel(&model);
    if (conn) {
        larodDisconnect(&conn, NULL);
    }
    if (fpInput && fclose(fpInput)) {
        syslog(LOG_ERR, "ERROR: Could not close file %s\n", inputFile);
    }
    if (fpOutput && fclose(fpOutput)) {
        syslog(LOG_ERR, "ERROR: Could not close file %s\n", outputFile);
    }

    larodDestroyJobRequest(&infReq);
    larodDestroyTensors(conn, &inputTensors, numInputs, &error);
    larodDestroyTensors(conn, &outputTensors, numOutputs, &error);
    larodClearError(&error);

    syslog(LOG_INFO, "Exit %s", argv[0]);
    return ret ? EXIT_SUCCESS : EXIT_FAILURE;
}
