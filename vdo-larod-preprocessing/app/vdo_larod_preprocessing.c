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
#include <larod.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <vdo-buffer.h>
#include <vdo-stream.h>
#include <syslog.h>
#include "larod-vdo-utils.h"
#include "imagenet-labels.h"

#define VDO_OUTPUT_HEIGHT 320
#define VDO_OUTPUT_WIDTH 480
#define VDO_FRAMERATE 30
#define VDO_CHANNEL 1
#define VDO_COLORFORMAT VDO_FORMAT_YUV
#define VDO_BUFFER_STRAT VDO_BUFFER_STRATEGY_INFINITE

#define INFERENCE_INPUT_HEIGHT 224
#define INFERENCE_INPUT_WIDTH 224
#define INFERENCE_NUM_OUTPUT_ELEMS 1001

#define NUM_ROUNDS 5
#define NUM_TOP_SCORES 3

/**
 * @brief Invoked on SIGINT. Makes app exit cleanly asap if invoked once, but
 * forces an immediate exit without clean up if invoked at least twice.
 *
 * @param sig What signal has been sent.
 */
static void sigintHandler(int sig);

/**
 * @brief Translates a string representing a larodChip to an actual larodChip.
 *
 * @param[in] chipString String of an integer representing a larodChip.
 * @param[out] chip Pointer to a larodChip that will be filled in accordingly.
 * @return False if any errors occur, otherwise true.
 */
static bool parseChipArg(const char* chipString, larodChip* const chip);

/**
 * @brief Defines and loads an inference model given a file path.
 *
 * The provided model should represent MobileNetV2 in the format required by @p
 * chip.
 *
 * @param[in] conn Pointer to an active larodConnection.
 * @param[in] chip larodChip the model should be loaded to.
 * @param[in] modelPath Path to the file containing the model data.
 * @param[out] model Double pointer that will be assigned the loaded model.
 * @return False if any errors occur, otherwise true.
 */
static bool setupInferenceModel(larodConnection* const conn,
                                const larodChip chip,
                                const char* const modelPath,
                                larodModel** const model);

/**
 * @brief Creates and populates a larodJobRequest for @p infModel.
 *
 * Allocates the input and output tensors and creates a new larodJobRequest with
 * these. Note that the allocated input tensors will also be used as output
 * tensors for the preprocessing model.
 *
 * @param[in] conn Pointer to an active larodConnection.
 * @param[in] infModel Pointer to model for the request.
 * @param[out] infInputs Triple pointer that will be assigned allocated inputs.
 * @param[out] infOutputs Triple pointer that will be assigned allocated
 * outputs.
 * @param[out] infJobReq Double pointer that will be assigned created request.
 * @return False if any errors occur, otherwise true.
 */
static bool setupInfJobRequest(larodConnection* const conn,
                               const larodModel* const infModel,
                               larodTensor*** const infInputs,
                               larodTensor*** const infOutputs,
                               larodJobRequest** const infJobReq);

/**
 * @brief Creates a mapping of the buffer of @p infOutputTensor.
 *
 * Extracts the file descriptor from @p infOutputTensor and maps it for this
 * process so we can look at output data after running inferences.
 *
 * @param[in] infOutputTensor Pointer to an inference output tensor.
 * @param[out] infOutputFd Pointer that will be assigned the fd of @p
 * infOutputTensor.
 * @param[out] infOutputSize Pointer that will be assigned the size in bytes of
 * the buffer of @p infOutputTensor.
 * @param[out] infOutputBuf Double pointer that will be assigned to a mapping
 * for this process of the buffer of @p infOutputTensor.
 * @return False if any errors occur, otherwise true.
 */
static bool setupInfOutputMapping(larodTensor* const infOutputTensor,
                                  int* const infOutputFd,
                                  size_t* const infOutputSize,
                                  uint8_t** const infOutputBuf);

/**
 * @brief Defines and loads a preprocessing model.
 *
 * The model will do crop-scale from VDO_OUTPUT_HEIGHT x VDO_OUTPUT_WIDTH to
 * INFERENCE_INPUT_HEIGHT x INFERENCE_INPUT_WIDTH and color conversion from
 * VDO_COLORFORMAT to the color format the provided inference model requires.
 *
 * If on a Ambarella platform the model will be loaded onto
 * LAROD_CHIP_CVFLOW_PROC and otherwise LAROD_CHIP_LIBYUV.
 *
 * @param[in] conn Pointer to an active larodConnection.
 * @param[in] infChip Specifying the larodChip used for inference.
 * @param[out] model Double pointer that will be assigned the loaded model.
 * @return False if any errors occur, otherwise true.
 */
static bool setupPreprocModel(larodConnection* const conn,
                              const larodChip infChip,
                              larodModel** const model);

/**
 * @brief Creates and starts a VDO stream.
 *
 * The stream is created according to the settings VDO_OUTPUT_HEIGHT,
 * VDO_OUTPUT_WIDTH, VDO_FRAMERATE, VDO_CHANNEL, VDO_COLORFORMAT and
 * VDO_BUFFER_STRAT.
 *
 * @param[in] vdoStream Double pointer that will be assigned the new VDO
 * stream.
 * @return False if any errors occur, otherwise true.
 */
static bool setupVdoStream(VdoStream** const vdoStream);

/**
 * @brief Creates and populates a larodJobRequest for @p preprocModel.
 *
 * Fetches a new frame from VDO and creates an input tensor based on it. Then
 * creates a new larodJobRequest using the new input tensor and @p
 * preprocOutputs.
 *
 * @param[in] conn Pointer to an active larodConnection.
 * @param[in] preprocModel Pointer to model for the request.
 * @param[in] preprocOutputs Double pointer to valid output tensors.
 * @param[in] vdoStream Pointer to an active VDO stream to fetch image data
 * from.
 * @param[out] preprocJobReq Double pointer that will be assigned a newly
 * created request if @p preprocJobReq points to NULL. Otherwise the existing
 * request will be updated with a new input buffer from VDO output.
 * @param[out] vdoBuffer Double pointer that will be assigned a newly fetched
 * buffer to a video frame.
 * @return False if any errors occur, otherwise true.
 */
static bool setupPreprocJobRequest(larodConnection* const conn,
                                   const larodModel* const preprocModel,
                                   larodTensor** const preprocOutputs,
                                   VdoStream* const vdoStream,
                                   larodJobRequest** const preprocJobReq,
                                   VdoBuffer** const vdoBuffer);

/**
 * @brief Print top classification scores from an inference output.
 *
 * Sort the classification scores of @p infOutputBuf and prints the top
 * NUM_TOP_SCORES scores along with their indices and ImageNet labels.
 *
 * @param[in] infOutputBuf Pointer to inference output data.
 * @param[in] infOutputSize Size in bytes of the @p infOutputBuf buffer.
 * @param[in] infOutputTensor Pointer to an inference output tensor.
 * @return False if any errors occur, otherwise true.
 */
static bool displayInfOutput(const uint8_t* const infOutputBuf,
                             const size_t infOutputSize,
                             const larodTensor* const infOutputTensor);

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

    // Tell the main thread to stop running jobs asap.
    stopRunning = true;
}

int main(int argc, char** argv) {
    int ret = EXIT_FAILURE;
    larodConnection* conn;
    larodError* larodError = NULL;
    larodModel* infModel = NULL;
    larodModel* preprocModel = NULL;
    // The input tensors for the inference model will also be used as output
    // tensors for the preprocessing model.
    larodTensor** infInputs = NULL;
    larodTensor** infOutputs = NULL;
    larodJobRequest* preprocJobReq = NULL;
    larodJobRequest* infJobReq = NULL;
    int infOutputFd = -1;
    size_t infOutputSize = 0;
    uint8_t* infOutputBuf = NULL;
    VdoStream* vdoStream = NULL;
    VdoBuffer* vdoBuffer = NULL;

    // Open the syslog to report messages for "vdo_larod_preprocessing"
    openlog("vdo_larod_preprocessing", LOG_PID|LOG_CONS, LOG_USER);

    // Register an interrupt handler which tries to exit cleanly if invoked once
    // but exits immediately if further invoked.
    signal(SIGINT, sigintHandler);

    if (argc != 3) {
        syslog(LOG_ERR, "Invalid number of arguments\nArguments are: "
                        "INF_CHIP MODEL_PATH\n");
    }

    larodChip infChip;
    if (!parseChipArg(argv[1], &infChip)) {
        syslog(LOG_ERR, "Could not parse INF_CHIP argument %s: %s\n", argv[1],
                strerror(errno));

        return ret;
    }

   syslog(LOG_INFO, "Connecting to larod\n");

    if (!larodConnect(&conn, &larodError)) {
        syslog(LOG_ERR, "Could not connect to larod (%d): %s\n",
                larodError->code, larodError->msg);

        return ret;
    }

   syslog(LOG_INFO, "Setting up inference model\n");

    if (!setupInferenceModel(conn, infChip, argv[2], &infModel)) {
        goto end;
    }

    if (!setupInfJobRequest(conn, infModel, &infInputs, &infOutputs,
                            &infJobReq)) {
        goto end;
    }

    if (!setupInfOutputMapping(infOutputs[0], &infOutputFd, &infOutputSize,
                               &infOutputBuf)) {
        goto end;
    }

   syslog(LOG_INFO, "Setting up preprocessing model\n");

    if (!setupPreprocModel(conn, infChip, &preprocModel)) {
        goto end;
    }

   syslog(LOG_INFO, "Setting up and starting VDO stream\n");

    if (!setupVdoStream(&vdoStream)) {
        goto end;
    }

   syslog(LOG_INFO, "Starting to run larod jobs\n");

    for (size_t i = 0; i < NUM_ROUNDS; i++) {
        if (!setupPreprocJobRequest(conn, preprocModel, infInputs, vdoStream,
                                    &preprocJobReq, &vdoBuffer)) {
            goto end;
        }

        if (!larodRunJob(conn, preprocJobReq, &larodError)) {
            syslog(LOG_ERR, "Could not run preprocessing job (%d): %s\n",
                    larodError->code, larodError->msg);

            goto end;
        }

        if (!larodRunJob(conn, infJobReq, &larodError)) {
            syslog(LOG_ERR, "Could not run inference job (%d): %s\n",
                    larodError->code, larodError->msg);

            goto end;
        }

        if (!displayInfOutput(infOutputBuf, infOutputSize, infOutputs[0])) {
            goto end;
        }

        GError* vdoError;
        // If successful, vdo_stream_buffer_unref() sets buffer to NULL.
        if (!vdo_stream_buffer_unref(vdoStream, &vdoBuffer, &vdoError)) {
            syslog(LOG_ERR, "Could not unref buffer of VDO stream: %s\n",
                    vdoError->message);
            g_clear_error(&vdoError);

            goto end;
        }
    }

    ret = EXIT_SUCCESS;

end:
    if (vdoBuffer) {
        vdo_stream_buffer_unref(vdoStream, &vdoBuffer, NULL);
    }

    if (vdoStream) {
        g_object_unref(vdoStream);
    }

    if (infOutputBuf) {
        munmap(infOutputBuf, infOutputSize);
    }

    if (infOutputFd >= 0) {
        close(infOutputFd);
    }

    larodDestroyJobRequest(&preprocJobReq);
    larodDestroyJobRequest(&infJobReq);

    larodDestroyTensors(&infInputs, 1);
    larodDestroyTensors(&infOutputs, 1);

    larodDestroyModel(&infModel);
    larodDestroyModel(&preprocModel);

    larodClearError(&larodError);

    larodDisconnect(&conn, NULL);

    syslog(LOG_INFO, "Exit %s", argv[0]);
    return ret;
}

bool parseChipArg(const char* chipString, larodChip* chip) {
    char* endPtr;
    unsigned long long i = strtoull(chipString, &endPtr, 0);

    if (*endPtr != '\0') {
        errno = EINVAL;
        return false;
    } else if (chipString[0] == '-' || i == LAROD_CHIP_INVALID) {
        errno = EINVAL;
        return false;
    } else if (i > INT_MAX) {
        errno = ERANGE;
        return false;
    }

    *chip = (larodChip) i;

    return true;
}

bool setupInferenceModel(larodConnection* const conn, const larodChip chip,
                         const char* const modelPath,
                         larodModel** const model) {
    bool ret = false;
    larodError* larodError = NULL;

    int fd = open(modelPath, O_RDONLY);
    if (fd < 0) {
        syslog(LOG_ERR, "Could not open inference model file %s: %s\n",
                modelPath, strerror(errno));

        goto end;
    }

    *model = larodLoadModel(conn, fd, chip, LAROD_ACCESS_PRIVATE, NULL, NULL,
                            &larodError);
    if (!*model) {
        syslog(LOG_ERR, "Could not load inference model (%d): %s\n",
                larodError->code, larodError->msg);

        goto end;
    }

    ret = true;

end:
    if (fd >= 0) {
        close(fd);
    }

    larodClearError(&larodError);

    return ret;
}

bool setupPreprocModel(larodConnection* const conn, const larodChip infChip,
                       larodModel** const model) {
    bool ret = false;
    larodError* larodError = NULL;

    // The LAROD_CHIP_CVFLOW_NN takes planar RGB as input and so we need to use
    // a preprocessing backend that can convert to that format.
    larodChip preprocChip;
    char* preprocOutputFormat;
    if (infChip == LAROD_CHIP_CVFLOW_NN) {
        preprocChip = LAROD_CHIP_CVFLOW_PROC;
        preprocOutputFormat = "rgb-planar";
    } else {
        // We default to converting to interleaved RGB with libyuv unless on
        // Ambarella platforms.
        preprocChip = LAROD_CHIP_LIBYUV;
        preprocOutputFormat = "rgb-interleaved";
    }

    larodMap* modelParams = larodCreateMap(&larodError);
    if (!modelParams) {
        syslog(LOG_ERR, "Could not create preprocessing model map (%d): %s\n",
                larodError->code, larodError->msg);

        goto end;
    }

    if (!larodMapSetStr(modelParams, "image.input.format", "nv12",
                        &larodError)) {
        syslog(LOG_ERR, "Could not set preprocessing input format (%d): %s\n",
                larodError->code, larodError->msg);

        goto end;
    }

    if (!larodMapSetIntArr2(modelParams, "image.input.size", VDO_OUTPUT_WIDTH,
                            VDO_OUTPUT_HEIGHT, &larodError)) {
        syslog(LOG_ERR, "Could not set preprocessing input size (%d): %s\n",
                larodError->code, larodError->msg);

        goto end;
    }

    if (!larodMapSetStr(modelParams, "image.output.format", preprocOutputFormat,
                        &larodError)) {
        syslog(LOG_ERR, "Could not set preprocessing output format (%d): %s\n",
                larodError->code, larodError->msg);

        goto end;
    }

    if (!larodMapSetIntArr2(modelParams, "image.output.size",
                            INFERENCE_INPUT_WIDTH, INFERENCE_INPUT_HEIGHT,
                            &larodError)) {
        syslog(LOG_ERR, "Could not set preprocessing output size (%d): %s\n",
                larodError->code, larodError->msg);

        goto end;
    }

    // This preprocessing model is defined by the modelParams map so we
    // don't need to supply a model file in this case.
    *model = larodLoadModel(conn, -1, preprocChip, LAROD_ACCESS_PRIVATE, NULL,
                            modelParams, &larodError);
    if (!*model) {
        syslog(LOG_ERR, "Could not load preprocessing model (%d): %s\n",
                larodError->code, larodError->msg);

        goto end;
    }

    ret = true;

end:
    larodClearError(&larodError);

    larodDestroyMap(&modelParams);

    return ret;
}

bool setupInfJobRequest(larodConnection* const conn,
                        const larodModel* const infModel,
                        larodTensor*** const infInputs,
                        larodTensor*** const infOutputs,
                        larodJobRequest** const infJobReq) {
    bool ret = false;
    larodError* larodError = NULL;

    // Note that these tensors will serve as output tensors to the preprocessing
    // jobs as well.
    *infInputs =
        larodAllocModelInputs(conn, infModel, 0, NULL, NULL, &larodError);
    if (!*infInputs) {
        syslog(LOG_ERR, "Could not allocate inference input tensors (%d): %s\n",
                larodError->code, larodError->msg);

        goto end;
    }

    *infOutputs =
        larodAllocModelOutputs(conn, infModel, 0, NULL, NULL, &larodError);
    if (!*infOutputs) {
        syslog(LOG_ERR,
                "Could not allocate inference output tensors (%d): %s\n",
                larodError->code, larodError->msg);

        goto end;
    }

    *infJobReq = larodCreateJobRequest(infModel, *infInputs, 1, *infOutputs, 1,
                                       NULL, &larodError);
    if (!*infJobReq) {
        syslog(LOG_ERR, "Could not create inference job request (%d): %s\n",
                larodError->code, larodError->msg);

        goto end;
    }

    ret = true;

end:
    larodClearError(&larodError);

    return ret;
}

bool setupInfOutputMapping(larodTensor* const infOutputTensor,
                           int* const infOutputFd, size_t* const infOutputSize,
                           uint8_t** const infOutputBuf) {
    bool ret = false;
    larodError* larodError = NULL;

    *infOutputFd = larodGetTensorFd(infOutputTensor, &larodError);
    if (*infOutputFd == LAROD_INVALID_FD) {
        syslog(LOG_ERR, "Could not get inference output tensor fd (%d): %s\n",
                larodError->code, larodError->msg);

        goto end;
    }

    if (!larodGetTensorFdSize(infOutputTensor, infOutputSize, &larodError)) {
        syslog(LOG_ERR,
                "Could not get inference output tensor fd size (%d): %s\n",
                larodError->code, larodError->msg);

        goto end;
    }

    *infOutputBuf =
        mmap(NULL, *infOutputSize, PROT_READ, MAP_PRIVATE, *infOutputFd, 0);
    if (*infOutputBuf == MAP_FAILED) {
        syslog(LOG_ERR, "Could not map inference output tensor's fd : %s\n",
                strerror(errno));
        *infOutputBuf = NULL;

        goto end;
    }

    ret = true;

end:
    larodClearError(&larodError);

    return ret;
}

bool setupVdoStream(VdoStream** const vdoStream) {
    bool ret = false;
    GError* vdoError = NULL;

    VdoMap* map = vdo_map_new();
    if (!map) {
        syslog(LOG_ERR, "Could not create VDO map\n");

        return false;
    }

    vdo_map_set_uint32(map, "channel", VDO_CHANNEL);
    vdo_map_set_uint32(map, "format", VDO_COLORFORMAT);
    vdo_map_set_uint32(map, "height", VDO_OUTPUT_HEIGHT);
    vdo_map_set_uint32(map, "width", VDO_OUTPUT_WIDTH);
    vdo_map_set_uint32(map, "framerate", VDO_FRAMERATE);
    vdo_map_set_uint32(map, "buffer.strategy", VDO_BUFFER_STRAT);

    // Create stream. The larodVdoUtilsDestroyTensor() destroy function is
    // called for each VdoBuffer of the stream upon the stream's destruction.
    // This will clean up any larodTensor using the buffers of the stream.
    *vdoStream = vdo_stream_new(map, larodVdoUtilsDestroyTensor, &vdoError);
    if (!*vdoStream) {
        syslog(LOG_ERR, "Could not create VDO stream: %s\n",
                vdoError ? vdoError->message : "N/A");

        goto end;
    }

    if (!vdo_stream_start(*vdoStream, &vdoError)) {
        syslog(LOG_ERR, "Could not start VDO stream: %s\n", vdoError->message);

        goto end;
    }

    ret = true;

end:
    if (vdoError) {
        g_clear_error(&vdoError);
    }

    g_object_unref(map);

    return ret;
}

bool setupPreprocJobRequest(larodConnection* const conn,
                            const larodModel* const preprocModel,
                            larodTensor** const preprocOutputs,
                            VdoStream* const vdoStream,
                            larodJobRequest** const preprocJobReq,
                            VdoBuffer** const vdoBuffer) {
    bool ret = false;
    larodError* larodError = NULL;

    GError* vdoError;
    *vdoBuffer = vdo_stream_get_buffer(vdoStream, &vdoError);
    if (!*vdoBuffer) {
        syslog(LOG_ERR, "Could not get buffer from VDO stream: %s\n",
                vdoError->message);
        g_clear_error(&vdoError);

        goto end;
    }

    // We don't need to keep track of the preprocessing input tensors since the
    // larod-vdo-utils lib will do so for us.
    larodTensor* tmpPreprocInputs[1];
    tmpPreprocInputs[0] =
        larodVdoUtilsBufferToTensor(conn, vdoStream, *vdoBuffer, &larodError);
    if (!tmpPreprocInputs[0]) {
        syslog(LOG_ERR,
                "Could not create larod tensor from VDO buffer (%d): %s\n",
                larodError->code, larodError->msg);

        goto end;
    }

    if (*preprocJobReq) {
        if (!larodSetJobRequestInputs(*preprocJobReq, tmpPreprocInputs, 1,
                                      &larodError)) {
            syslog(LOG_ERR,
                    "Could not set input tensors on preprocessing job request "
                    "(%d): %s\n",
                    larodError->code, larodError->msg);

            goto end;
        }
    } else {
        *preprocJobReq =
            larodCreateJobRequest(preprocModel, tmpPreprocInputs, 1,
                                  preprocOutputs, 1, NULL, &larodError);
        if (!*preprocJobReq) {
            syslog(LOG_ERR,
                    "Could not create preprocessing job request (%d): %s\n",
                    larodError->code, larodError->msg);

            goto end;
        }
    }

    ret = true;

end:
    larodClearError(&larodError);

    return ret;
}

// Auxiliary type to let us sort scores keeping track of indices.
struct Score {
    size_t idx;
    float floatVal;
    uint8_t uint8Val;
    bool uint8;
};

// This is used in the qsort callback to sort elements based on classification
// score.
static int scoreCompare(const void* lhs, const void* rhs) {
    struct Score* lhsScore = (struct Score*) lhs;
    struct Score* rhsScore = (struct Score*) rhs;

    if (lhsScore->uint8) {
        if (lhsScore->uint8Val > rhsScore->uint8Val) {
            return -1;
        } else if (lhsScore->uint8Val < rhsScore->uint8Val) {
            return 1;
        }
    } else {
        if (lhsScore->floatVal > rhsScore->floatVal) {
            return -1;
        } else if (lhsScore->floatVal < rhsScore->floatVal) {
            return 1;
        }
    }

    return 0;
}

bool displayInfOutput(const uint8_t* const infOutputBuf,
                      const size_t infOutputSize,
                      const larodTensor* const infOutputTensor) {
    larodError* larodError = NULL;

    larodTensorDataType dataType =
        larodGetTensorDataType(infOutputTensor, &larodError);
    if (dataType == LAROD_TENSOR_DATA_TYPE_INVALID) {
        syslog(LOG_ERR,
                "Could not get inference output tensor data type (%d): %s\n",
                larodError->code, larodError->msg);
        larodClearError(&larodError);

        return false;
    }

    const larodTensorPitches* pitches =
        larodGetTensorPitches(infOutputTensor, &larodError);
    if (!pitches) {
        syslog(LOG_ERR,
                "Could not get inference output tensor pitches (%d): %s\n",
                larodError->code, larodError->msg);
        larodClearError(&larodError);

        return false;
    }

    size_t dataTypeSize = dataType == LAROD_TENSOR_DATA_TYPE_UINT8 ?
                              sizeof(uint8_t) :
                              sizeof(float);
    // We need to figure out how we should stride our output buffer to obtain
    // the actual scores. Two common output formats are supported.
    size_t spacePerElement;
    if (pitches->pitches[pitches->len - 1] >=
        dataTypeSize * INFERENCE_NUM_OUTPUT_ELEMS) {
        // TFLite MobileNetV2 models will typically fall into this category.
        spacePerElement = dataTypeSize;
    } else if (infOutputSize / pitches->pitches[pitches->len - 1] ==
               INFERENCE_NUM_OUTPUT_ELEMS) {
        // CVFlowNN MobileNetV2 models will typically fall into this category.
        spacePerElement = pitches->pitches[pitches->len - 1];
    } else {
        syslog(LOG_ERR,
                "This MobileNetV2 output tensor format is not supported\n");

        return false;
    }

    // Next we create a temporary array to store the classification scores along
    // with some metadata like index so that we know which element got which
    // score also after sort.
    struct Score scores[INFERENCE_NUM_OUTPUT_ELEMS];

   syslog(LOG_INFO, "\n===== Top %d inference scores =====\n\n", NUM_TOP_SCORES);

    if (dataType == LAROD_TENSOR_DATA_TYPE_UINT8) {
        for (size_t i = 0; i < INFERENCE_NUM_OUTPUT_ELEMS; i++) {
            scores[i].idx = i;
            scores[i].uint8Val = *(infOutputBuf + i * spacePerElement);
            scores[i].uint8 = true; // Indicate kind of data to comparator.
        }

        qsort(scores, INFERENCE_NUM_OUTPUT_ELEMS, sizeof(scores[0]),
              scoreCompare);

        for (size_t i = 0; i < NUM_TOP_SCORES; i++) {
           syslog(LOG_INFO, "%zu. idx %zu labeled \"%s\" with score %.2f\n", i + 1,
                   scores[i].idx, imagenetLabels[scores[i].idx],
                   ((float) scores[i].uint8Val) / 255.0f);
        }
    } else if (dataType == LAROD_TENSOR_DATA_TYPE_FLOAT32) {
        for (size_t i = 0; i < INFERENCE_NUM_OUTPUT_ELEMS; i++) {
            scores[i].idx = i;
            scores[i].floatVal = (float) *(infOutputBuf + i * spacePerElement);
            scores[i].uint8 = false; // Indicate kind of data to comparator.
        }

        qsort(scores, INFERENCE_NUM_OUTPUT_ELEMS, sizeof(scores[0]),
              scoreCompare);

        for (size_t i = 0; i < NUM_TOP_SCORES; i++) {
           syslog(LOG_INFO, "%zu. idx %zu labeled \"%s\" with score %.2f\n", i + 1,
                   scores[i].idx, imagenetLabels[scores[i].idx],
                   scores[i].floatVal);
        }
    } else {
        syslog(LOG_ERR, "Only models with either UINT8 or FLOAT32 output types "
                        "are supported\n");

        return false;
    }

    return true;
}
