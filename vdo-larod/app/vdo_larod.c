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
 * - vdo_larod -
 *
 * This application loads an image classification model to larod and
 * then uses vdo to fetch frames of size WIDTH x HEIGHT in yuv
 * format which are converted to interleaved rgb format and then sent to
 * larod for inference on MODEL.
 *
 * The application expects four arguments on the command line in the following
 * order: MODEL WIDTH HEIGHT OUTPUT_SIZE.
 *
 * First argument, MODEL, is a string describing path to the model.
 *
 * Second argument, WIDTH, is an integer for width size.
 *
 * Third argument, HEIGHT, is an integer for height size.
 *
 * Fourth argument, OUTPUT_SIZE, denotes the size in bytes of
 * the tensor output by model.
 *
 * The application has two optional arguments on the command line in the following
 * order: CHIP NUM_FRAMES.
 *
 * First optional argument, CHIP, is an enum describing the selected chip
 * e.g. 4 is used for Google TPU.
 *
 * Second optional argument, LABEL, is path to a file labelling classifications.
 *
 * Finally, third optional argument, NUM_FRAMES, is an integer for number of
 * captured frames.
 *
 * Then you could run the application with Google TPU with command:
 *     ./usr/local/packages/vdo_larod/vdo_larod \
 *     /usr/local/packages/vdo_larod/model/mobilenet_v2_1.0_224_quant_edgetpu.tflite \
 *     224 224 1001 -c 4 \
 *     -l /usr/local/packages/vdo_larod/label/imagenet_labels.txt
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

#include "argparse.h"
#include "imgconverter.h"
#include "imgprovider.h"
#include "larod.h"
#include "vdo-frame.h"
#include "vdo-types.h"

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
static bool setupLarod(const larodChip larodChip, const int larodModelFd,
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

static bool setupLarod(const larodChip larodChip, const int larodModelFd,
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

    // Set chip if user has specified non-default action.
    if (larodChip != 0) {
        if (!larodSetChip(conn, larodChip, &error)) {
            syslog(LOG_ERR, "%s: Could not select chip %d: %s", __func__, larodChip,
                   error->msg);
            goto error;
        }
    }

    loadedModel = larodLoadModel(conn, larodModelFd, LAROD_ACCESS_PRIVATE,
                                 "Vdo Example App Model", &error);
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
    char CONV_INP_FILE_PATTERN[] = "/tmp/larod.in.test-XXXXXX";
    char CONV_OUT_FILE_PATTERN[] = "/tmp/larod.out.test-XXXXXX";

    bool ret = false;
    ImgProvider_t* provider = NULL;
    larodError* error = NULL;
    larodConnection* conn = NULL;
    larodTensor** inputTensors = NULL;
    size_t numInputs = 0;
    larodTensor** outputTensors = NULL;
    size_t numOutputs = 0;
    larodInferenceRequest* infReq = NULL;
    void* larodInputAddr = MAP_FAILED;
    void* larodOutputAddr = MAP_FAILED;
    int larodModelFd = -1;
    int larodInputFd = -1;
    int larodOutputFd = -1;
    char** labels = NULL; // This is the array of label strings. The label
                          // entries points into the large labelFileData buffer.
    size_t numLabels = 0; // Number of entries in the labels array.
    char* labelFileData =
        NULL; // Buffer holding the complete collection of label strings.
    args_t args;

    // Open the syslog to report messages for "vdo_larod"
    openlog("vdo_larod", LOG_PID|LOG_CONS, LOG_USER);

    syslog(LOG_INFO, "Starting ...");

    // Register an interrupt handler which tries to exit cleanly if invoked once
    // but exits immediately if further invoked.
    signal(SIGINT, sigintHandler);

    if (!parseArgs(argc, argv, &args)) {
        goto end;
    }

    unsigned int streamWidth = 0;
    unsigned int streamHeight = 0;
    if (!chooseStreamResolution(args.width, args.height, &streamWidth,
                                &streamHeight)) {
        syslog(LOG_ERR, "%s: Failed choosing stream resolution", __func__);
        goto end;
    }

    syslog(LOG_INFO, "Creating VDO image provider and creating stream %d x %d",
           streamWidth, streamHeight);
    provider = createImgProvider(streamWidth, streamHeight, 2, VDO_FORMAT_YUV);
    if (!provider) {
        goto end;
    }

    larodModelFd = open(args.modelFile, O_RDONLY);
    if (larodModelFd < 0) {
        syslog(LOG_ERR, "Unable to open model file %s: %s", args.modelFile,
               strerror(errno));
        goto end;
    }

    syslog(LOG_INFO, "Setting up larod connection with chip %d and model %s", args.chip,
           args.modelFile);
    larodModel* model = NULL;
    if (!setupLarod(args.chip, larodModelFd, &conn, &model)) {
        goto end;
    }

    syslog(LOG_INFO, "Creating temporary files and memmaps for inference input and "
           "output tensors");

    if (!createAndMapTmpFile(CONV_INP_FILE_PATTERN,
                             args.width * args.height * CHANNELS,
                             &larodInputAddr, &larodInputFd)) {
        goto end;
    }

    if (!createAndMapTmpFile(CONV_OUT_FILE_PATTERN, args.outputBytes,
                             &larodOutputAddr, &larodOutputFd)) {
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
    if (!larodSetTensorFd(inputTensors[0], larodInputFd, &error)) {
        syslog(LOG_ERR, "Failed setting input tensor fd: %s", error->msg);
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
    if (!larodSetTensorFd(outputTensors[0], larodOutputFd, &error)) {
        syslog(LOG_ERR, "Failed setting output tensor fd: %s", error->msg);
        goto end;
    }

    // App supports only one input/output tensor.
    infReq = larodCreateInferenceRequest(model, inputTensors, 1, outputTensors,
                                         1, &error);
    if (!infReq) {
        syslog(LOG_ERR, "Failed creating inference request: %s", error->msg);
        goto end;
    }

    if (args.labelsFile) {
        if (!parseLabels(&labels, &labelFileData, args.labelsFile,
                         &numLabels)) {
            syslog(LOG_ERR, "Failed creating parsing labels file");
            goto end;
        }
    }

    syslog(LOG_INFO, "Start fetching video frames from VDO");
    if (!startFrameFetch(provider)) {
        goto end;
    }

    for (unsigned int i = 0; i < args.numFrames && !stopRunning; i++) {
        struct timeval startTs, endTs;
        unsigned int elapsedMs = 0;

        // Get latest frame from image pipeline.
        VdoBuffer* buf = getLastFrameBlocking(provider);
        if (!buf) {
            goto end;
        }

        // Get data from latest frame.
        uint8_t* nv12Data = (uint8_t*) vdo_buffer_get_data(buf);

        // Covert image data from NV12 format to interleaved uint8_t RGB format.
        gettimeofday(&startTs, NULL);
        if (!convertCropScaleU8yuvToRGB(nv12Data, streamWidth, streamHeight,
                                        (uint8_t*) larodInputAddr, args.width,
                                        args.height)) {
            syslog(LOG_ERR, "%s: Failed img scale/convert in "
                   "convertCropScaleU8yuvToRGB() (continue anyway)",
                   __func__);
        }
        gettimeofday(&endTs, NULL);

        elapsedMs = (unsigned int) (((endTs.tv_sec - startTs.tv_sec) * 1000) +
                                    ((endTs.tv_usec - startTs.tv_usec) / 1000));
        syslog(LOG_INFO, "Converted image in %u ms", elapsedMs);

        // Since larodOutputAddr points to the beginning of the fd we should
        // rewind the file position before each inference.
        if (lseek(larodOutputFd, 0, SEEK_SET) == -1) {
            syslog(LOG_ERR, "Unable to rewind output file position: %s",
                   strerror(errno));

            goto end;
        }

        gettimeofday(&startTs, NULL);
        if (!larodRunInference(conn, infReq, &error)) {
            syslog(LOG_ERR, "Unable to run inference on model %s: %s (%d)",
                   args.modelFile, error->msg, error->code);
            goto end;
        }
        gettimeofday(&endTs, NULL);

        elapsedMs = (unsigned int) (((endTs.tv_sec - startTs.tv_sec) * 1000) +
                                    ((endTs.tv_usec - startTs.tv_usec) / 1000));
        syslog(LOG_INFO, "Ran inference for %u ms", elapsedMs);

        // Compute the most likely index.
        uint8_t maxProb = 0;
        size_t maxIdx = 0;
        uint8_t* outputPtr = (uint8_t*) larodOutputAddr;
        for (size_t j = 0; j < args.outputBytes; j++) {
            if (outputPtr[j] > maxProb) {
                maxProb = outputPtr[j];
                maxIdx = j;
            }
        }
        if (labels) {
            if (maxIdx < numLabels) {
                syslog(LOG_INFO, "Top result: %s with score %.2f%%", labels[maxIdx],
                       (float) maxProb / 2.5f);
            } else {
                syslog(LOG_INFO, "Top result: index %zu with score %.2f%% (index larger "
                       "than num items in labels file)",
                       maxIdx, (float) maxProb / 2.5f);
            }
        } else {
            syslog(LOG_INFO, "Top result: index %zu with score %.2f%%", maxIdx,
                   (float) maxProb / 2.5f);
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
    larodDestroyModel(&model);
    if (conn) {
        larodDisconnect(&conn, NULL);
    }
    if (larodModelFd >= 0) {
        close(larodModelFd);
    }
    if (larodInputAddr != MAP_FAILED) {
        munmap(larodInputAddr, args.width * args.height * CHANNELS);
    }
    if (larodInputFd >= 0) {
        close(larodInputFd);
    }
    if (larodOutputAddr != MAP_FAILED) {
        munmap(larodOutputAddr, args.outputBytes);
    }
    if (larodOutputFd >= 0) {
        close(larodOutputFd);
    }

    larodDestroyInferenceRequest(&infReq);
    larodDestroyTensors(&inputTensors, numInputs);
    larodDestroyTensors(&outputTensors, numOutputs);
    larodClearError(&error);

    if (labels) {
        freeLabels(labels, labelFileData);
    }

    return ret ? EXIT_SUCCESS : EXIT_FAILURE;
}
