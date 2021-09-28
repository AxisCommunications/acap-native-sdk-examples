/**
 * Copyright (C) 2018-2021, Axis Communications AB, Lund, Sweden
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
#include <inttypes.h>
#include <libgen.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#include "larod.h"

/**
 * brief Deletes a model from larod.
 *
 * So that the model can no longer be used by calls to larod.
 *
 * param conn An initialized larod connection handle.
 * param model A pointer to a larod model.
 * return False if any errors occur, otherwise true.
 */
bool deleteModel(larodConnection* conn, larodModel* model);

/**
 * brief Load a model into larod.
 *
 * So that the model we can run inferences on the model with larod.
 *
 * param conn An initialized larod connection handle.
 * param modelFile A string containing the path to the model file.
 * param modelName A string containing the preferred name of the model.
 * param model A pointer to a pointer to the model returned by larod after it
 * has been loaded.
 * return False if any errors occur, otherwise true.
 */
bool loadModel(larodConnection* conn, char* modelFile, const char* modelName,
               larodModel** model);

/**
 * brief Run inference on a model with larod.
 *
 * param conn An initialized larod connection handle.
 * param model A pointer to the model to run inference on.
 * param inputFile A string containing the path to the input file for the
 * inference.
 * param outputFile A string containing the preferred path of the file to which
 * the inference output will be written. If NULL is provided a file name based
 * on the input file's name suffixed with ".out" will be produced.
 * return False if any errors occur, otherwise true.
 */
bool runInference(larodConnection* conn, larodModel* model, char* inputFile,
                  char* outputFile);

larodError* error = NULL;

int main(int argc, char** argv) {
    bool ret = false;
    larodConnection* conn = NULL;
    larodModel* model = NULL;

    // Check for valid number of arguments
    if (argc != 3) {
        syslog(LOG_ERR, "ERROR: Invalid number of arguments\n"
                        "Usage: larod_simple_app MODEL_FILE INPUT_FILE\n");
        goto end;
    }

    syslog(LOG_INFO,"Connecting to larod...");
    if (!larodConnect(&conn, &error)) {
        syslog(LOG_ERR, "ERROR: Could not connect to larod (%d): %s\n",
                error->code, error->msg);

        larodClearError(&error);
        goto end;
    }
    syslog(LOG_INFO,"Connected");

    if (!larodSetChip(conn, LAROD_CHIP_TFLITE_CPU, &error)) {
        syslog(LOG_ERR,
                "ERROR: Could not set larod chip to TFLite CPU (%d): %s\n",
                error->code, error->msg);

        larodClearError(&error);
        goto end;
    }

    if (!loadModel(conn, argv[1], NULL, &model)) {
        goto end;
    }

    if (!runInference(conn, model, argv[2], NULL)) {
        goto end;
    }

    if (!deleteModel(conn, model)) {
        goto end;
    }

    ret = true;

end:
    larodDestroyModel(&model);

    if (conn && !larodDisconnect(&conn, &error)) {
        syslog(LOG_ERR, "ERROR: Could not disconnect (%d): %s\n", error->code,
                error->msg);

        larodClearError(&error);
    }

    return ret ? EXIT_SUCCESS : EXIT_FAILURE;
}

bool loadModel(larodConnection* conn, char* modelFile, const char* modelName,
               larodModel** model) {
    syslog(LOG_INFO, "Loading model...");
    larodModel* localModel = NULL;
    FILE* fpModel = fopen(modelFile, "rb");
    if (!fpModel) {
        syslog(LOG_ERR, "ERROR: Could not open model file %s: %s\n", modelFile,
                strerror(errno));
        return false;
    }

    const int fd = fileno(fpModel);
    if (fd < 0) {
        syslog(LOG_ERR,
                "ERROR: Could not get file descriptor for model file: %s\n",
                strerror(errno));
        fclose(fpModel);
        return false;
    }

    if (!modelName) {
        modelName = basename(modelFile);
    }

    localModel =
        larodLoadModel(conn, fd, LAROD_ACCESS_PUBLIC, modelName, &error);
    if (!localModel) {
        syslog(LOG_ERR, "ERROR: Could not load model (%d): %s\n", error->code,
                error->msg);

        larodClearError(&error);
        fclose(fpModel);
        return false;
    }

    if (fclose(fpModel)) {
        syslog(LOG_ERR, "ERROR: Could not close file %s\n", modelFile);
    }

    *model = localModel;
    syslog(LOG_INFO, "Model %s loaded \n", modelFile);

    return true;
}

bool deleteModel(larodConnection* conn, larodModel* model) {
    if (!larodDeleteModel(conn, model, &error)) {
        syslog(LOG_ERR, "ERROR: Could not delete model (%d): %s\n", error->code,
                error->msg);

        larodClearError(&error);
        return false;
    }

    return true;
}

bool runInference(larodConnection* conn, larodModel* model, char* inputFile,
                  char* outputFile) {
    syslog(LOG_INFO,"Running inference...");

    bool ret = false;
    larodInferenceRequest* infReq = NULL;
    FILE* fpInput = NULL;
    FILE* fpOutput = NULL;
    larodTensor** inputTensors = NULL;
    size_t numInputs = 0;
    larodTensor** outputTensors = NULL;
    size_t numOutputs = 0;

    fpInput = fopen(inputFile, "rb");
    if (!fpInput) {
        syslog(LOG_ERR, "ERROR: Could not open input file %s: %s\n", inputFile,
                strerror(errno));
        goto end;
    }

    const int inFd = fileno(fpInput);
    if (inFd < 0) {
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

    const int outFd = fileno(fpOutput);
    if (outFd < 0) {
        syslog(LOG_ERR,
                "ERROR: Could not get file descriptor for output file: %s\n",
                strerror(errno));
        goto end;
    }

    inputTensors = larodCreateModelInputs(model, &numInputs, &error);
    if (!inputTensors) {
        syslog(LOG_ERR, "ERROR: Failed retrieving input tensors (%d): %s\n",
                error->code, error->msg);
        goto end;
    }
    if (numInputs != 1) {
        syslog(LOG_ERR,
                "ERROR: This model has %zu input tensors, but example only "
                "supports 1 input tensor.\n",
                numInputs);
        goto end;
    }

    if (!larodSetTensorFd(inputTensors[0], inFd, &error)) {
        syslog(LOG_ERR, "ERROR: Could not set input tensor fd (%d): %s\n",
                error->code, error->msg);
        goto end;
    }

    outputTensors = larodCreateModelOutputs(model, &numOutputs, &error);
    if (!outputTensors) {
        syslog(LOG_ERR, "ERROR: Failed retrieving output tensors (%d): %s\n",
                error->code, error->msg);
        goto end;
    }
    if (numOutputs != 1) {
        syslog(LOG_ERR,
                "ERROR: This model has %zu output tensors, but example only "
                "supports 1 output tensor.\n",
                numOutputs);
        goto end;
    }

    if (!larodSetTensorFd(outputTensors[0], outFd, &error)) {
        syslog(LOG_ERR, "ERROR: Could not set output tensor fd (%d): %s\n",
                error->code, error->msg);
        goto end;
    }

    infReq = larodCreateInferenceRequest(model, inputTensors, 1, outputTensors,
                                         1, &error);
    if (!infReq) {
        syslog(LOG_ERR, "ERROR: Could not create inference request (%d): %s\n",
                error->code, error->msg);
        goto end;
    }

    if (!larodRunInference(conn, infReq, &error)) {
        syslog(LOG_ERR, "Could not run inference (%d): %s", error->code,
                error->msg);
        goto end;
    }

    ret = true;

    syslog(LOG_INFO,"Output written to %s\n", outputFile);


end:
    if (fpInput && fclose(fpInput)) {
        syslog(LOG_ERR, "ERROR: Could not close file %s\n", inputFile);
    }
    if (fpOutput && fclose(fpOutput)) {
        syslog(LOG_ERR, "ERROR: Could not close file %s\n", outputFile);
    }

    larodDestroyInferenceRequest(&infReq);
    larodDestroyTensors(&inputTensors, numInputs);
    larodDestroyTensors(&outputTensors, numOutputs);

    larodClearError(&error);

    return ret;
}
