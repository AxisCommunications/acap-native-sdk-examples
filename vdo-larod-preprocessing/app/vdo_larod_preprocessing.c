/**
 * Copyright (C) 2021, Axis Communications AB, Lund, Sweden
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

#include "imgprovider.h"
#include "larod.h"
#include "vdo-frame.h"
#include "vdo-types.h"

#define INFERENCE_INPUT_HEIGHT 224
#define INFERENCE_INPUT_WIDTH 224
#define NUM_ROUNDS 5


/**
 * brief Invoked on SIGINT. Makes app exit cleanly asap if invoked once, but
 * forces an immediate exit without clean up if invoked at least twice.
 *
 * param sig What signal has been sent.
 */
static void sigintHandler(int sig);

/**
 * brief Creates a temporary fd truncated to correct size and mapped.
 *
 * This convenience function creates temp files to be used for input and output.
 *
 * param fileName Pattern for how the temp file will be named in file system.
 * param fileSize How much space needed to be allocated (truncated) in fd.
 * param mappedAddr Pointer to the address of the fd mapped for this process.
 * param Pointer to the generated fd.
 * return False if any errors occur, otherwise true.
 */
static bool createAndMapTmpFile(char* fileName, size_t fileSize,
                                void** mappedAddr, int* convFd);

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
                       larodConnection** larodConn, larodModel** model);

/**
 * brief Free up resources held by an array of labels.
 *
 * param labels An array of label string pointers.
 * param labelFileBuffer Heap buffer containing the actual string data.
 */
static void freeLabels(char** labelsArray, char* labelFileBuffer);

/**
 * brief Reads a file of labels into an array.
 *
 * An array filled by this function should be freed using freeLabels.
 *
 * param labelsPtr Pointer to a string array.
 * param labelFileBuffer Pointer to the labels file contents.
 * param labelsPath String containing the path to the labels file to be read.
 * param numLabelsPtr Pointer to number which will store number of labels read.
 * return False if any errors occur, otherwise true.
 */
static bool parseLabels(char*** labelsPtr, char** labelFileBuffer,
                        char* labelsPath, size_t* numLabelsPtr);

/// Set by signal handler if an interrupt signal sent to process.
/// Indicates that app should stop asap and exit gracefully.
volatile sig_atomic_t stopRunning = false;

void sigintHandler(int sig) {
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

static bool createAndMapTmpFile(char* fileName, size_t fileSize,
                                void** mappedAddr, int* convFd) {
                                    syslog(LOG_INFO, "%s: Setting up a temp fd with pattern %s and size %zu", __func__,
           fileName, fileSize);

    int fd = mkstemp(fileName);
    if (fd < 0) {
        syslog(LOG_ERR, "%s: Unable to open temp file %s: %s", __func__, fileName,
               strerror(errno));
        goto error;
    }

    // Allocate enough space in for the fd.
    if (ftruncate(fd, (off_t) fileSize) < 0) {
        syslog(LOG_ERR, "%s: Unable to truncate temp file %s: %s", __func__, fileName,
               strerror(errno));
        goto error;
    }

    // Remove since we don't actually care about writing to the file system.
    if (unlink(fileName)) {
        syslog(LOG_ERR, "%s: Unable to unlink from temp file %s: %s", __func__,
               fileName, strerror(errno));
        goto error;
    }

    // Get an address to fd's memory for this process's memory space.
    void* data =
        mmap(NULL, fileSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (data == MAP_FAILED) {
        syslog(LOG_ERR, "%s: Unable to mmap temp file %s: %s", __func__, fileName,
               strerror(errno));
        goto error;
    }

    *mappedAddr = data;
    *convFd = fd;

    return true;

error:
    if (fd >= 0) {
        close(fd);
    }

    return false;
}

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

void freeLabels(char** labelsArray, char* labelFileBuffer) {
    free(labelsArray);
    free(labelFileBuffer);
}

bool parseLabels(char*** labelsPtr, char** labelFileBuffer, char* labelsPath,
                 size_t* numLabelsPtr) {
    // We cut off every row at 60 characters.
    const size_t LINE_MAX_LEN = 60;
    bool ret = false;
    char* labelsData = NULL;  // Buffer containing the label file contents.
    char** labelArray = NULL; // Pointers to each line in the labels text.

    struct stat fileStats = {0};
    if (stat(labelsPath, &fileStats) < 0) {
        syslog(LOG_ERR, "%s: Unable to get stats for label file %s: %s", __func__,
               labelsPath, strerror(errno));
        return false;
    }

    // Sanity checking on the file size - we use size_t to keep track of file
    // size and to iterate over the contents. off_t is signed and 32-bit or
    // 64-bit depending on architecture. We just check toward 10 MByte as we
    // will not encounter larger label files and both off_t and size_t should be
    // able to represent 10 megabytes on both 32-bit and 64-bit systems.
    if (fileStats.st_size > (10 * 1024 * 1024)) {
        syslog(LOG_ERR, "%s: failed sanity check on labels file size", __func__);
        return false;
    }

    int labelsFd = open(labelsPath, O_RDONLY);
    if (labelsFd < 0) {
        syslog(LOG_ERR, "%s: Could not open labels file %s: %s", __func__, labelsPath,
               strerror(errno));
        return false;
    }

    size_t labelsFileSize = (size_t) fileStats.st_size;
    // Allocate room for a terminating NULL char after the last line.
    labelsData = malloc(labelsFileSize + 1);
    if (labelsData == NULL) {
        syslog(LOG_ERR, "%s: Failed allocating labels text buffer: %s", __func__,
               strerror(errno));
        goto end;
    }

    ssize_t numBytesRead = -1;
    size_t totalBytesRead = 0;
    char* fileReadPtr = labelsData;
    while (totalBytesRead < labelsFileSize) {
        numBytesRead =
            read(labelsFd, fileReadPtr, labelsFileSize - totalBytesRead);

        if (numBytesRead < 1) {
            syslog(LOG_ERR, "%s: Failed reading from labels file: %s", __func__,
                   strerror(errno));
            goto end;
        }
        totalBytesRead += (size_t) numBytesRead;
        fileReadPtr += numBytesRead;
    }

    // Now count number of lines in the file - check all bytes except the last
    // one in the file.
    size_t numLines = 0;
    for (size_t i = 0; i < (labelsFileSize - 1); i++) {
        if (labelsData[i] == '\n') {
            numLines++;
        }
    }

    // We assume that there is always a line at the end of the file, possibly
    // terminated by newline char. Either way add this line as well to the
    // counter.
    numLines++;

    labelArray = malloc(numLines * sizeof(char*));
    if (!labelArray) {
        syslog(LOG_ERR, "%s: Unable to allocate labels array: %s", __func__,
               strerror(errno));
        ret = false;
        goto end;
    }

    size_t labelIdx = 0;
    labelArray[labelIdx] = labelsData;
    labelIdx++;
    for (size_t i = 0; i < labelsFileSize; i++) {
        if (labelsData[i] == '\n') {
            // Register the string start in the list of labels.
            labelArray[labelIdx] = labelsData + i + 1;
            labelIdx++;
            // Replace the newline char with string-ending NULL char.
            labelsData[i] = '\0';
        }
    }

    // If the very last byte in the labels file was a new-line we just
    // replace that with a NULL-char. Refer previous for loop skipping looking
    // for new-line at the end of file.
    if (labelsData[labelsFileSize - 1] == '\n') {
        labelsData[labelsFileSize - 1] = '\0';
    }

    // Make sure we always have a terminating NULL char after the label file
    // contents.
    labelsData[labelsFileSize] = '\0';

    // Now go through the list of strings and cap if strings too long.
    for (size_t i = 0; i < numLines; i++) {
        size_t stringLen = strnlen(labelArray[i], LINE_MAX_LEN);
        if (stringLen >= LINE_MAX_LEN) {
            // Just insert capping NULL terminator to limit the string len.
            *(labelArray[i] + LINE_MAX_LEN + 1) = '\0';
        }
    }

    *labelsPtr = labelArray;
    *numLabelsPtr = numLines;
    *labelFileBuffer = labelsData;

    ret = true;

end:
    if (!ret) {
        freeLabels(labelArray, labelsData);
    }
    close(labelsFd);

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
    char CONV_OUT_FILE_PATTERN[] = "/tmp/larod.out.test-XXXXXX";

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
    void* larodOutputAddr = MAP_FAILED;
    size_t outputBufferSize = 0;
    int larodModelFd = -1;
    int ppInputFd = -1;
    int larodInputFd = -1;
    int larodOutputFd = -1;
    char** labels = NULL; // This is the array of label strings. The label
                          // entries points into the large labelFileData buffer.
    size_t numLabels = 0; // Number of entries in the labels array.
    char* labelFileData =
        NULL; // Buffer holding the complete collection of label strings.
    const char* chipString = argv[1];
    // Open the syslog to report messages for "vdo_larod_preprocessing"
    openlog("vdo_larod_preprocessing", LOG_PID|LOG_CONS, LOG_USER);
    syslog(LOG_INFO, "Starting %s", argv[0]);
    // Register an interrupt handler which tries to exit cleanly if invoked once
    // but exits immediately if further invoked.
    signal(SIGINT, sigintHandler);

    if (argc != 4) {
        syslog(LOG_ERR, "Invalid number of arguments\nArguments are: "
                        "INF_CHIP MODEL_PATH LABELS_PATH\n");
        goto end;
    }

    // Create video stream provider
    unsigned int streamWidth = 0;
    unsigned int streamHeight = 0;
    if (!chooseStreamResolution(INFERENCE_INPUT_WIDTH, INFERENCE_INPUT_HEIGHT, &streamWidth,
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
    float destWHratio = (float) INFERENCE_INPUT_WIDTH / (float) INFERENCE_INPUT_HEIGHT;
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
    if (!larodMapSetIntArr2(ppMap, "image.output.size", INFERENCE_INPUT_WIDTH, INFERENCE_INPUT_HEIGHT, &error)) {
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
    larodModelFd = open(argv[2], O_RDONLY);
    if (larodModelFd < 0) {
        syslog(LOG_ERR, "Unable to open model file %s: %s", argv[2],
               strerror(errno));
        goto end;
    }


    syslog(LOG_INFO, "Setting up larod connection with chip %s, model %s and label file %s", chipString, argv[2], argv[3]);
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
    if (numOutputs != 1) {
        syslog(LOG_ERR, "Model has %zu outputs, app only supports 1 output tensor.",
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
    size_t expectedSize = INFERENCE_INPUT_WIDTH * INFERENCE_INPUT_HEIGHT * CHANNELS;
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
                             INFERENCE_INPUT_WIDTH * INFERENCE_INPUT_HEIGHT * CHANNELS,
                             &larodInputAddr, &larodInputFd)) {
        goto end;
    }
    if (!createAndMapTmpFile(CONV_OUT_FILE_PATTERN, outputBufferSize,
                             &larodOutputAddr, &larodOutputFd)) {
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
    if (!larodSetTensorFd(outputTensors[0], larodOutputFd, &error)) {
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
                                         1, NULL, &error);
    if (!infReq) {
        syslog(LOG_ERR, "Failed creating inference request: %s", error->msg);
        goto end;
    }

    if (argv[2]) {
        if (!parseLabels(&labels, &labelFileData, argv[3],
                         &numLabels)) {
            syslog(LOG_ERR, "Failed creating parsing labels file");
            goto end;
        }
    }

    syslog(LOG_INFO, "Start fetching video frames from VDO");
    if (!startFrameFetch(provider)) {
        goto end;
    }

    for (unsigned int i = 0; i < NUM_ROUNDS && !stopRunning; i++) {
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
            syslog(LOG_ERR, "Unable to run job on model pp: %s (%d)",
                   error->msg, error->code);
            goto end;
        }
        gettimeofday(&endTs, NULL);

        elapsedMs = (unsigned int) (((endTs.tv_sec - startTs.tv_sec) * 1000) +
                                    ((endTs.tv_usec - startTs.tv_usec) / 1000));
        syslog(LOG_INFO, "Converted image in %u ms", elapsedMs);

        // Since larodOutputAddr points to the beginning of the fd we should
        // rewind the file position before each job.
        if (lseek(larodOutputFd, 0, SEEK_SET) == -1) {
            syslog(LOG_ERR, "Unable to rewind output file position: %s",
                   strerror(errno));

            goto end;
        }

        gettimeofday(&startTs, NULL);
        if (!larodRunJob(conn, infReq, &error)) {
            syslog(LOG_ERR, "Unable to run inference on model %s: %s (%d)",
                   argv[2], error->msg, error->code);
            goto end;
        }
        gettimeofday(&endTs, NULL);

        elapsedMs = (unsigned int) (((endTs.tv_sec - startTs.tv_sec) * 1000) +
                                    ((endTs.tv_usec - startTs.tv_usec) / 1000));
        syslog(LOG_INFO, "Ran inference for %u ms", elapsedMs);

        // Compute the most likely index.
        float maxProb = 0;
        uint8_t maxScore = 0;
        size_t maxIdx = 0;
        uint8_t* outputPtr = (uint8_t*) larodOutputAddr;

        // The output has to be read differently depending on chip.
        // In the case of the cv25, the space per element is 32 bytes and the
        // output is a float padded with zeros.
        // In the cases of artpec7 and artpec8 the space per element is 1 byte
        // and the output is an uint8_t that has to be processed with softmax.
        // This part of the code can be improved by using better pointer casting,
        // subject to future changes.
        int spacePerElement;
        if (chipString == "ambarella-cvflow") {
            spacePerElement = 32;
            float score;
            for (size_t j = 0; j < outputBufferSize/spacePerElement; j++) {
                score = *((float*) (outputPtr + (j*spacePerElement)));
                if (score > maxProb) {
                    maxProb = score;
                    maxIdx = j;
                }
        }
        } else {
            spacePerElement = 1;
            uint8_t score;
            for (size_t j = 0; j < outputBufferSize/spacePerElement; j++) {
                score = *((uint8_t*) (outputPtr + (j*spacePerElement)));
                if (score > maxScore) {
                    maxScore = score;
                    maxIdx = j;
                }
            }

            float sum = 0.0;
            for (size_t j = 0; j < outputBufferSize/spacePerElement; j++) {
                score = *((uint8_t*) (outputPtr + (j*spacePerElement)));
                sum += exp(score - maxScore);
            }
            // Simplifying softmax calculation:
            // softmax[i_max] = e^(-log(sum)) = 1/sum
            maxProb = 1/sum;
        }
        maxProb*=100; //To have output int %
        if (labels) {
            if (maxIdx < numLabels) {
                syslog(LOG_INFO, "Top result: %s with score %.2f%%",
                labels[maxIdx], maxProb);
            } else {
                syslog(LOG_INFO, "Top result: index %zu with score %.2f%% (index larger "
                       "than num items in labels file)",
                       maxIdx, maxProb);
            }
        } else {
            syslog(LOG_INFO, "Top result: index %zu with score %.2f%%",
            maxIdx, maxProb);
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
        munmap(ppInputAddr, INFERENCE_INPUT_WIDTH * INFERENCE_INPUT_HEIGHT * CHANNELS);
    }
    if (ppInputFd >= 0) {
        close(ppInputFd);
    }
    if (larodInputAddr != MAP_FAILED) {
        munmap(larodInputAddr, INFERENCE_INPUT_WIDTH * INFERENCE_INPUT_HEIGHT * CHANNELS);
    }
    if (larodInputFd >= 0) {
        close(larodInputFd);
    }
    if (larodOutputAddr != MAP_FAILED) {
        munmap(larodOutputAddr, outputBufferSize);
    }
    if (larodOutputFd >= 0) {
        close(larodOutputFd);
    }

    larodDestroyJobRequest(&ppReq);
    larodDestroyJobRequest(&infReq);
    larodDestroyTensors(conn, &inputTensors, numInputs, &error);
    larodDestroyTensors(conn, &outputTensors, numOutputs, &error);
    larodClearError(&error);

    if (labels) {
        freeLabels(labels, labelFileData);
    }

    syslog(LOG_INFO, "Exit %s", argv[0]);
    return ret ? EXIT_SUCCESS : EXIT_FAILURE;
}
