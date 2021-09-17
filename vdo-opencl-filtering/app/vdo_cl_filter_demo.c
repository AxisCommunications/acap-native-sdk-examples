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

/**
 * - vdo_cl_filter_demo -
 *
 * The application starts a VDO stream in YUV NV12 format and continuously
 * captures n frames from the VDO service (5 by default).
 *
 * OpenCL uses the received frame buffer as input to the filtering operations.
 * The output buffer is different from the input, and is mapped by this example.
 * All image memory is allocated such that it may be zero-copied to the GPU,
 * ensuring good performance.
 *
 * Sobel filtering is performed according to the sobel_nv12 OpenCL program.
 * You may choose to filter a half or a full image, with two different filter
 * kernels. The result is written to an output file with default name
 * /usr/local/packages/vdo_cl_filter_demo/cl_vdo_demo.yuv.
 *
 * Suppose you have completed the steps of installation. You may then go to
 * /usr/local/packages/vdo_cl_filter_demo on your device and run the example as:
 *  ./vdo_cl_filter_demo
 *
 * It can also be ran from the Apps menu.
 */
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <syslog.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <errno.h>

#include "vdo-map.h"
#include "vdo-error.h"
#include "vdo-types.h"
#include "vdo-stream.h"

#define CL_TARGET_OPENCL_VERSION 120
#include <CL/cl.h>

#define MAX_SOURCE_SIZE (0x100000)

#define VDO_CLIENT_ERROR g_quark_from_static_string("vdo-client-error")
#define VDO_SUBFORMAT_NV12 "NV12"

/* Supported filter kernels */
#define FILTER_SOBEL_3X3 "sobel_3x3"
#define FILTER_SOBEL_3X1 "sobel_3x1"

/* Which part of the captured images to filter with OpenCL */
enum render_area {
  FULL_AREA = 0,
  HALF_AREA,
};

/* VDO Data */
static VdoStream* stream;

/* OpenCL Data */
cl_platform_id platform_id = NULL;
cl_device_id device_id = NULL;
cl_uint ret_num_devices;
cl_uint ret_num_platforms;
cl_context context;
cl_program program;
cl_kernel kernel;
cl_command_queue command_queue;

/* OpenCL memory objects */
cl_mem out_image_y;
cl_mem out_image_cbcr;

size_t global_work_size[2];

GHashTable *table = NULL;

static void
print_cl_platform_info(cl_platform_id id)
{
    cl_platform_info param_names[] = {
        CL_PLATFORM_PROFILE,
        CL_PLATFORM_VERSION,
        CL_PLATFORM_NAME,
        CL_PLATFORM_VENDOR,
        CL_PLATFORM_EXTENSIONS
    };

    syslog(LOG_INFO, "Platform info:");
    for (int i = 0; i < 5; i++) {
        size_t size = 0;
        char *info;
        clGetPlatformInfo(id, param_names[i], 0, NULL, &size);

        info = (char*)malloc(size);
        clGetPlatformInfo(id, param_names[i], size, info, NULL);
        syslog(LOG_INFO, "- %s\n", info);
        free(info);
    }
    syslog(LOG_INFO, "End of info");
}

/* Create cl kernel program */
static cl_program
create_cl_program(cl_context cont, const char *file_name, cl_int *ret)
{
    FILE *fp;
    char *source_str;
    size_t source_size;
    fp = fopen(file_name, "r");
    if (!fp)
    {
        syslog(LOG_ERR, "Failed to load kernel.");
        exit(1);
    }
    source_str = (char *)malloc(MAX_SOURCE_SIZE);

    source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);

    syslog(LOG_INFO, "Read cl file \"%s\", size of %zu bytes", file_name,
          source_size);

    fclose(fp);

    cl_program prog = clCreateProgramWithSource(cont, 1,
                                               (const char **)&source_str,
                                               (const size_t *)&source_size,
                                               ret);
    free(source_str);
    return prog;
}

static int
free_opencl()
{
    int cl_ret;
    cl_ret = clReleaseMemObject(out_image_y);
    cl_ret |= clReleaseMemObject(out_image_cbcr);
    if (cl_ret != CL_SUCCESS) {
        syslog(LOG_ERR, "Failed to release memory objects: %d", cl_ret);
        return -1;
    }
    cl_ret = clReleaseKernel(kernel);
    if (cl_ret != CL_SUCCESS) {
        syslog(LOG_ERR, "Failed to release the kernel: %d", cl_ret);
        return -1;
    }
    cl_ret = clReleaseProgram(program);
    if (cl_ret != CL_SUCCESS) {
        syslog(LOG_ERR, "Failed to release the program: %d", cl_ret);
        return -1;
    }
    cl_ret = clReleaseCommandQueue(command_queue);
    if (cl_ret != CL_SUCCESS) {
        syslog(LOG_ERR, "Failed to release the command queue: %d", cl_ret);
        return -1;
    }
    cl_ret = clReleaseContext(context);
    if (cl_ret != CL_SUCCESS) {
        syslog(LOG_ERR, "Failed to release the context: %d", cl_ret);
        return -1;
    }
    return 0;
}

static int
setup_opencl(const char* kernel_name, enum render_area area, unsigned width, unsigned height)
{
    cl_int ret = clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
    if (ret != CL_SUCCESS) {
        syslog(LOG_ERR, "Could not get device id's");
        return -1;
    }

    print_cl_platform_info(platform_id);

    ret = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_DEFAULT, 1,
                        &device_id, &ret_num_devices);
    if (ret != CL_SUCCESS) {
        syslog(LOG_ERR, "Could not get device id's");
        return -1;
    }

    context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &ret);
    if (ret != CL_SUCCESS) {
        syslog(LOG_ERR, "Could not create opencl context");
        return -1;
    }

    program = create_cl_program(context,
                        "/usr/local/packages/vdo_cl_filter_demo/sobel_nv12.cl",
                        &ret);

    if (ret != CL_SUCCESS) {
        syslog(LOG_ERR, "Could not create cl program");
        return -1;
    }

    /* This string can be used to pass paramaters to the OpenCl compiler */
    char options[] = "";
    ret = clBuildProgram(program, 1, &device_id, options, NULL, NULL);
    if (ret != CL_SUCCESS) {
        syslog(LOG_ERR, "Could not build cl_program");

        if (ret == CL_BUILD_PROGRAM_FAILURE) {
            /* Determine the size of the program log */
            size_t log_size;
            clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 0,
                                 NULL, &log_size);
            /* Allocate memory for the program log */
            char *log = (char *)malloc(log_size);

            /* Get the program log */
            clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG,
                                 log_size, log, NULL);

            /* Print the program log */
            syslog(LOG_INFO, "%s", log);
            free(log);
        }
        return -1;
    }

    kernel = clCreateKernel(program, kernel_name, &ret);
    if (ret != CL_SUCCESS) {
        syslog(LOG_ERR, "Could not create cl_program");
        return -1;
    }

    command_queue = clCreateCommandQueue(context, device_id, 0, &ret);
    if (ret != CL_SUCCESS) {
        syslog(LOG_ERR, "Could not create command queue");
        return -1;
    }

    switch (area) {
    case HALF_AREA:
        global_work_size[0] = height;
        global_work_size[1] = width / 16;
    break;
    case FULL_AREA:
    default:
        global_work_size[0] = height;
        global_work_size[1] = width / 8;
    break;
    }

    return 0;
}

/*
 * For our sobel operations we ignore cbcr values and simply output 128 for all
 * pixels directly in the kernel.
 */
static int
do_opencl_filtering(cl_mem *in_image_y, void *out_data, unsigned width,
                   unsigned height, size_t image_y_size, size_t image_cbcr_size)
{
    int ret;

    /*
     * This is the setting for local_work_size that works the best in terms
     * of not only speed, but also achieving correct functionality when stream
     * is rotated. This is due to the fact that global_work_size needs to be
     * evenly divisible by local_work_size in all dimensions.
     */
    size_t local_work_size[2] = {8, 4};
    size_t offset[2] = {1,0};

    /*
     * Since we use NV12 data we could also create a single memory object
     * direcly with luma and chroma included. For simplicity we split them up.
     *
     * The idea is to use CL_MEM_USE_HOST_PTR which means GPU access system
     * memory, such that no unnecessary data has to be copied to GPU memory.
     * This data may still however be cached in the GPU.
     */
    out_image_y = clCreateBuffer(context,
                                CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR,
                                image_y_size, out_data, &ret);
    out_image_cbcr = clCreateBuffer(context,
                                   CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR,
                                   image_cbcr_size, (out_data + image_y_size),
                                   &ret);
    if (ret != CL_SUCCESS) {
        syslog(LOG_ERR, "Unable to create cl memory objects");
        return -1;
    }

    ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)in_image_y);
    ret |= clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&out_image_y);
    ret |= clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&out_image_cbcr);
    ret |= clSetKernelArg(kernel, 3, sizeof(width), &width);
    ret |= clSetKernelArg(kernel, 4, sizeof(height), &height);
    ret |= clEnqueueNDRangeKernel(command_queue, kernel, 2, offset,
                                 global_work_size, local_work_size, 0, NULL,
                                 NULL);

    ret |= clFinish(command_queue);
    if (ret != CL_SUCCESS) {
        syslog(LOG_ERR, "Unable to complete OpenCL operations: %d", ret);
        return -1;
    }
    return 0;
}

static void
free_table_entry(gpointer key, gpointer value, gpointer user_data)
{
    clReleaseMemObject((cl_mem)value);
}

/* Release cl memory objects on hash table cleanup */
static void
hash_table_destroy(void)
{
    g_hash_table_foreach(table, free_table_entry, NULL);
    g_hash_table_destroy(table);
}

/*
 * Map a VDO buffer address to an OpenCL memory object. If the address has
 * already been mapped, look up the OpenCL memory object in the hash table
 */
static int
map_input_buffer(void *buffer, cl_mem *in_images, cl_mem *in_image_y,
                size_t image_size, enum render_area area, unsigned buffer_count)
{
    int ret;
    static unsigned count = 0;

    if (g_hash_table_contains(table, buffer)) {
        *in_image_y = *((cl_mem*) g_hash_table_lookup(table, buffer));
        return 0;
    }

    /* Make sure we're not getting any more unique addresses than asked for */
    if (count == buffer_count) {
        return -1;
    }

    /*
     * Re-use already allocated VDO frame buffer as input to OpenCL program.
     * In this specific example we don't need the bottom 1/3rd of the frame
     * containing cbcr data, so we simply ignore it.
     */
    in_images[count] = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                                     image_size, buffer, &ret);

    if (ret != CL_SUCCESS) {
        syslog(LOG_ERR, "Unable to create new cl memory object: %d", ret);
        return -1;
    }

    g_hash_table_insert(table, (gpointer) buffer, (gpointer) (&in_images[count]));
    *in_image_y = in_images[count];
    count++;
    return 0;
}

int
main(int argc, char* argv[])
{
    GError *error = NULL;
    FILE* output_file = NULL;
    VdoMap *settings = NULL;
    VdoMap *vdo_stream_info = NULL;
    void *out_data = NULL;
    cl_mem *in_images = NULL;

    const gchar *output_file_format = "yuv";  /* Also the VDO stream format */

    /* VDO stream dimensions */
    const unsigned image_width = 1280;
    const unsigned image_height = 720;
    const guint frames = 5; /* Number of frames to process */
    const guint buffer_count = 3; /* Number of unique VDO buffers */

    /* Render settings specific for this example */
    const char* kernel_name = FILTER_SOBEL_3X3;
    enum render_area cur_render_area = HALF_AREA;

    /* Open connection to syslog */
    openlog(NULL, LOG_PID, LOG_USER);

    /* Set up VDO */
    settings = vdo_map_new();
    vdo_map_set_uint32(settings, "format", VDO_FORMAT_YUV);

    /*
     * Set subformat to NV12. In this specific example we don't need the chroma
     * data, so we could have set the subformat to Y800 in order to only receive
     * the luma.
     */
    vdo_map_set_string(settings, "subformat", VDO_SUBFORMAT_NV12);
    vdo_map_set_uint32(settings, "width",  image_width);
    vdo_map_set_uint32(settings, "height", image_height);
    vdo_map_set_uint32(settings, "buffer.count", buffer_count);

    /* Create a new stream */
    stream = vdo_stream_new(settings, NULL, &error);
    g_clear_object(&settings);
    if (!stream)
        goto exit;

    if (!vdo_stream_attach(stream, NULL, &error))
        goto exit;

    /* Collect stream information */
    vdo_stream_info = vdo_stream_get_info(stream, &error);
    if (!vdo_stream_info)
        goto exit;

    syslog(LOG_INFO, "Starting stream: %s in %s, %ux%u, %u fps.",
          output_file_format,
          vdo_map_get_string(vdo_stream_info, "subformat", NULL, "N/A"),
          vdo_map_get_uint32(vdo_stream_info, "width", 0),
          vdo_map_get_uint32(vdo_stream_info, "height", 0),
          vdo_map_get_uint32(vdo_stream_info, "framerate", 0));

    g_clear_object(&vdo_stream_info);

    /* Start the stream */
    if (!vdo_stream_start(stream, &error))
        goto exit;

    /* Open output file */
    char file_path[128];
    snprintf(file_path, sizeof(file_path), "/usr/local/packages/"
            "vdo_cl_filter_demo/cl_vdo_demo.%s", output_file_format);

    output_file = fopen(file_path, "wb");
    if (!output_file) {
        g_set_error(&error, VDO_CLIENT_ERROR, VDO_ERROR_IO, "open failed: %m");
        goto exit;
    }

    /* Size of luma data in the image buffer */
    size_t image_y_size = image_width * image_height;
    /* Size of chroma data in the image buffer */
    size_t image_cbcr_size = image_y_size / 2;

    /*
     * Allocate memory for output buffer. In this case it's more practical with
     * a separate output buffer since we're performing a filtering operation.
     */
    out_data = mmap(NULL, (image_y_size + image_cbcr_size), PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (out_data == MAP_FAILED) {
        syslog(LOG_ERR, "mmap failed: %d", errno);
        goto exit;
    }

    /* Set up OpenCL */
    if (setup_opencl(kernel_name, cur_render_area, image_width, image_height)) {
        syslog(LOG_ERR, "Unable to setup OpenCL");
        goto exit;
    }

    /* Initialize hash table for mapping VDO buffers to OpenCL memory objects */
    table = g_hash_table_new(g_direct_hash, g_direct_equal);

    /*
     * Allocate space for cl memory objects. The addresses to the memory objects
     * will be stored in the hash table.
     */
    in_images = (cl_mem *) malloc(sizeof(cl_mem) * buffer_count);

    /* Loop for the pre-determined number of frames */
    for (guint n = 0; n < frames; n++) {

        /* Lifetimes of buffer and frame are linked, no need to free frame */
        VdoBuffer* buffer = vdo_stream_get_buffer(stream, &error);
        VdoFrame* frame  = vdo_buffer_get_frame(buffer);

        /* Error occurred */
        if (!frame)
            goto exit;

        /* Get VDO frame buffer data */
        gpointer in_data = vdo_buffer_get_data(buffer);
        if (!in_data) {
            g_set_error(&error, VDO_CLIENT_ERROR, 0, "Failed to get data: %m");
            vdo_stream_buffer_unref(stream, &buffer, NULL);
            goto exit;
        }

        /*
         * Copying the image data to the output buffer is not always necessary,
         * if the filtering is done over the full image area.
         * For HALF_AREA however, we need to write the background image data
         * as the cl program will not render a full image.
         */
        if (cur_render_area != FULL_AREA)
            memcpy(out_data, in_data, image_y_size + image_cbcr_size);

        /*
         * Map a received VDO frame buffer with a cl memory object. A cl buffer
         * will be created for every unique VDO buffer determined by buffer_count.
         * If the frame buffer has already been mapped, re-use its assigned
         * cl buffer.
         */
        cl_mem in_image_y;
        if (map_input_buffer(in_data, in_images, &in_image_y, image_y_size, cur_render_area, buffer_count))
            goto exit;

        if (do_opencl_filtering(&in_image_y, out_data, image_width, image_height,
                               image_y_size, image_cbcr_size))
            goto exit;

        if (!fwrite(out_data, vdo_frame_get_size(frame), 1, output_file)) {
            g_set_error(&error, VDO_CLIENT_ERROR, 0,
                       "Unable to write frame: %m");
            vdo_stream_buffer_unref(stream, &buffer, NULL);
            goto exit;
        }

        /* Release the buffer and allow the server to reuse it */
        if (!vdo_stream_buffer_unref(stream, &buffer, &error))
            goto exit;
    }

exit:
    /* Ignore expected error */
    if (vdo_error_is_expected(&error))
        g_clear_error(&error);

    if (munmap(out_data, image_y_size + image_cbcr_size)) {
        syslog(LOG_ERR, "Unable to unmap image output buffer");
    }

    hash_table_destroy();

    if (in_images)
        free(in_images);

    gint ret = EXIT_SUCCESS;
    if (error) {
        syslog(LOG_INFO, "vdo-encode-client: %s", error->message);
        ret = EXIT_FAILURE;
    }

    if (output_file)
        fclose(output_file);

    if (free_opencl() != 0){
        syslog(LOG_ERR, "Unable to clean up opencl");
        ret = EXIT_FAILURE;
    }

    g_clear_error(&error);
    g_clear_object(&stream);
    return ret;
}
