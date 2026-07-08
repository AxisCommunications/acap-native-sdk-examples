// Copyright (C) 2026 Axis Communications AB, Lund, Sweden
// Licensed under the MIT License. See LICENSE file for details.

/**
 * - axoverlay2_skia -
 *
 * This application demonstrates how the use the axoverlay API version 2.0
 * together with the Skia graphics toolkit to leverage GPU acceleration.
 */

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl3.h>
#include <axoverlay2.h>
#include <drm/drm_fourcc.h>
#include <glib-unix.h>
#include <glib.h>
#include <include/core/SkCanvas.h>
#include <include/core/SkColor.h>
#include <include/core/SkColorSpace.h>
#include <include/core/SkSurface.h>
#include <include/gpu/ganesh/GrBackendSurface.h>
#include <include/gpu/ganesh/GrDirectContext.h>
#include <include/gpu/ganesh/GrTypes.h>
#include <include/gpu/ganesh/SkSurfaceGanesh.h>
#include <include/gpu/ganesh/gl/GrGLBackendSurface.h>
#include <include/gpu/ganesh/gl/GrGLDirectContext.h>
#include <include/gpu/ganesh/gl/GrGLInterface.h>
#include <map>
#include <math.h>
#include <syslog.h>
#include <vdo-error.h>
#include <vdo-stream.h>

#include "axo2_wrappers.hh"
#include "gpu_error.hh"

/*
 * Global GPU rendering context. Normally it is enough to have a single such context per
 * application.
 */
struct RenderContext {
    EGLDisplay display = 0;
    EGLContext ctx     = 0;
    EGLSurface pbuffer = 0;
    sk_sp<GrDirectContext> gr_context;

    RenderContext() = default;

    ~RenderContext() {
        gr_context.reset();

        if (display)
            eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

        if (display && pbuffer)
            eglDestroySurface(display, pbuffer);

        if (display && ctx)
            eglDestroyContext(display, ctx);

        if (display)
            eglTerminate(display);
    }

    RenderContext(const RenderContext&)            = delete;
    RenderContext& operator=(const RenderContext&) = delete;
    RenderContext(RenderContext&&)                 = delete;
    RenderContext& operator=(RenderContext&&)      = delete;
};

/*
 * A "Surface" ready for rendering. This is an Axoverlay buffer wrapped by the GPU interfaces.
 */
struct RenderSurface {
    std::shared_ptr<RenderContext> context;

    EGLImageKHR image = 0;
    unsigned texture  = 0;
    sk_sp<SkSurface> surface;

    RenderSurface(std::shared_ptr<RenderContext> context) : context(context) {}

    ~RenderSurface() {
        static auto eglDestroyImageKHR =
            (PFNEGLDESTROYIMAGEKHRPROC)eglGetProcAddress("eglDestroyImageKHR");

        surface.reset();

        if (texture)
            glDeleteTextures(1, &texture);

        if (image)
            eglDestroyImageKHR(context->display, image);
    }

    RenderSurface(const RenderSurface&)            = delete;
    RenderSurface& operator=(const RenderSurface&) = delete;
    RenderSurface(RenderSurface&&)                 = delete;
    RenderSurface& operator=(RenderSurface&&)      = delete;
};

/* Record of an overlay that we own */
struct Overlay {
    int overlay_id;     /* Overlay ID number (from Axoverlay) */
    unsigned stream_id; /* Stream ID number (from VDO) */

    AxoDetailedFormat format;

    /* The used portion of the overlay in pixels */
    unsigned used_width;
    unsigned used_height;

    /* The total size of the overlay in pixels, including padding */
    unsigned full_width;
    unsigned full_height;

    /* Number of frames successfully rendered and submitted for this overlay */
    unsigned frame_count;

    /*
     * Cached surfaces for this overlay. There are only a small amount of distinct buffers for each
     * overlay which are cycled to produce animation. The operation to "import" each buffer into
     * the GPU APIs can be somewhat expensive, so it is prudent to cache the results.
     *
     * The cache key is the buffer ID returned by axo_buffer_get_id().
     */
    std::map<unsigned long, std::unique_ptr<RenderSurface>> surface_cache;
};

static std::unique_ptr<RenderContext> create_render_context_and_make_current();
static int signal_callback(void* userdata);
static int animation_tick_callback(void* userdata);
static int stream_event_callback(GIOChannel* channel, GIOCondition condition, void* userdata);
static void create_overlay(unsigned stream_id, unsigned stream_width, unsigned stream_height);
static void remove_overlay(unsigned stream_id);
static void process_next_frame(Overlay* overlay);
static std::unique_ptr<RenderSurface> create_render_surface(const Overlay& overlay, int dma_buf_fd);
static void render_frame(const Overlay& overlay, RenderSurface* surface);
static void draw_graphics(const Overlay& overlay, SkCanvas* canvas);

static VdoStream* vdo_event_stream;

/* A table of all overlays we currently own. Key: int stream_id; Value: stream
 * overlay */
static std::map<int, Overlay> overlay_table;

/*
 * Parameters for overlay animation. In a real application we would probably use
 * an external data source. Here we use a fixed 30 fps tick to advance our
 * placeholder animation
 */
static unsigned animation_state;
static unsigned tick_period_us = 1000000 / 30;

/* GLib main loop */
static GMainLoop* main_loop;

static std::shared_ptr<RenderContext> render_context;

/* Enable debug logging? */
static const bool debug = false;

int main(void) {
    g_autoptr(GError) error           = nullptr;
    AxoErr axo_error                  = {};
    g_autoptr(VdoMap) stream_filter   = nullptr;
    bool axo_running                  = false;
    int stream_event_fd               = -1;
    g_autoptr(GIOChannel) vdo_channel = nullptr;
    unsigned vdo_watch_id             = 0;
    int ret                           = 1;

    /* Setup and enable the global GPU rendering context */
    render_context = create_render_context_and_make_current();
    if (!render_context) {
        syslog(LOG_ERR, "Failed to create render context");
        goto out;
    }

    /* Start Axoverlay */
    if (!axo_start(nullptr, &axo_error.err)) {
        syslog(LOG_ERR, "Failed to start Axoverlay: %s", axo_err_get_message(axo_error.err));
        goto out;
    }

    axo_running = true;

    /*
     * Set up a GLib main loop for our application. More external event sources
     * may of course also be hooked into the same loop depending on the
     * application's needs.
     */
    main_loop = g_main_loop_new(nullptr, FALSE);

    /* Set up a simple timer to advance the overlay animation (period in
     * milliseconds) */
    g_timeout_add(tick_period_us / 1000, animation_tick_callback, nullptr);

    /*
     * Stream 0 in VDO is a magic pseudo-stream which is always present in the
     * system. We can use stream 0 to get events about all other streams.
     */
    vdo_event_stream = vdo_stream_get(0, &error);
    if (!vdo_event_stream) {
        syslog(LOG_ERR, "Failed to open vdo stream 0: %s", error->message);
        goto out;
    }

    /*
     * Set up VDO filter to disregard streams that do not want overlays. Drawing
     * overlays for a stream that will not use them is wasteful.
     */
    stream_filter = vdo_map_new();
    vdo_map_set_string(stream_filter, "filter", "overlay");

    if (!vdo_stream_attach(vdo_event_stream, stream_filter, &error)) {
        syslog(LOG_ERR, "Failed to attach filter to vdo stream 0: %s", error->message);
        goto out;
    }

    stream_event_fd = vdo_stream_get_event_fd(vdo_event_stream, &error);
    if (stream_event_fd < 0) {
        syslog(LOG_ERR, "Failed to get stream 0 event fd: %s", error->message);
        goto out;
    }

    /* Hook VDO stream event fd into our GLib event loop via a GIOChannel */
    vdo_channel  = g_io_channel_unix_new(stream_event_fd);
    vdo_watch_id = g_io_add_watch(vdo_channel,
                                  (GIOCondition)(G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP),
                                  stream_event_callback,
                                  nullptr);
    if (!vdo_watch_id) {
        syslog(LOG_ERR, "Failed to add stream event fd to event loop");
        goto out;
    }

    /* Set up signal handling to gracefully stop the main loop */
    g_unix_signal_add(SIGINT, signal_callback, main_loop);
    g_unix_signal_add(SIGTERM, signal_callback, main_loop);

    /* Enter main loop */
    g_main_loop_run(main_loop);

    ret = 0;

out:
    if (axo_running)
        axo_stop(nullptr);
    if (main_loop)
        g_main_loop_unref(main_loop);

    return ret;
}

/*
 * Set up global GPU rendering context. Skia uses the industry-standard EGL interface to
 * interoperate with the GPU.
 */
static std::unique_ptr<RenderContext> create_render_context_and_make_current() {
    std::unique_ptr<RenderContext> rc = std::make_unique<RenderContext>();

    rc->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (egl_check_error("open EGL display"))
        return nullptr;

    eglInitialize(rc->display, nullptr, nullptr);
    if (egl_check_error("initialize EGL display"))
        return nullptr;

    eglBindAPI(EGL_OPENGL_ES_API);
    if (egl_check_error("bind OpenGL ES API"))
        return nullptr;

    static const int config_attribs[] = {
        EGL_SURFACE_TYPE,
        EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE,
        EGL_OPENGL_ES3_BIT,
        EGL_NONE,
    };
    EGLConfig config;
    int num_configs;
    eglChooseConfig(rc->display, config_attribs, &config, 1, &num_configs);
    if (egl_check_error("choose EGL config"))
        return nullptr;

    static const int ctx_attribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    rc->ctx = eglCreateContext(rc->display, config, EGL_NO_CONTEXT, ctx_attribs);
    if (egl_check_error("create EGL context"))
        return nullptr;

    static const int pb_attribs[] = {EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE};
    rc->pbuffer                   = eglCreatePbufferSurface(rc->display, config, pb_attribs);
    if (egl_check_error("create EGL pbuffer surface"))
        return nullptr;

    eglMakeCurrent(rc->display, rc->pbuffer, rc->pbuffer, rc->ctx);
    if (egl_check_error("make EGL context current"))
        return nullptr;

    auto gl_interface = GrGLMakeNativeInterface();
    if (!gl_interface) {
        syslog(LOG_ERR, "Failed to create Skia GL interface");
        return nullptr;
    }

    rc->gr_context = GrDirectContexts::MakeGL(std::move(gl_interface));
    if (!rc->gr_context) {
        syslog(LOG_ERR, "Failed to create Skia GrDirectContext");
        return nullptr;
    }

    return rc;
}

static int signal_callback(void* userdata) {
    (void)userdata;

    g_main_loop_quit(main_loop);
    return G_SOURCE_REMOVE;
}

/*
 * Callback issued at a regular interval. This is a placeholder, representing
 * your application's external data sources that would trigger the overlay to be
 * updated.
 *
 * Returning G_SOURCE_CONTINUE keeps the timer running.
 */
static int animation_tick_callback(void* userdata) {
    (void)userdata;

    /* Move the animation forward to the next frame */
    animation_state++;

    /* Process next frame for each existing overlay */
    for (auto& [stream_id, overlay] : overlay_table) process_next_frame(&overlay);

    return G_SOURCE_CONTINUE;
}

/*
 * Callback issued when VDO tells us there is a new stream event. We are
 * interested in stream connection and disconnection events.
 *
 * Returning G_SOURCE_CONTINUE keeps the watch active; G_SOURCE_REMOVE tears it
 * down.
 */
static int stream_event_callback(GIOChannel* channel, GIOCondition condition, void* userdata) {
    (void)channel;
    (void)userdata;

    g_autoptr(GError) error         = nullptr;
    g_autoptr(VdoMap) vdo_event     = nullptr;
    g_autoptr(VdoStream) vdo_stream = nullptr;
    g_autoptr(VdoMap) stream_info   = nullptr;

    if (condition & (G_IO_ERR | G_IO_HUP)) {
        syslog(LOG_ERR, "Connection to vdo was broken, condition=0x%04x", condition);
        g_main_loop_quit(main_loop);
        return G_SOURCE_REMOVE;
    }

    vdo_event = vdo_stream_get_event(vdo_event_stream, &error);
    if (!vdo_event) {
        if (g_error_matches(error, VDO_ERROR, VDO_ERROR_NO_EVENT))
            return G_SOURCE_CONTINUE;

        syslog(LOG_ERR, "Failed to get vdo stream event: %s", error->message);
        g_main_loop_quit(main_loop);
        return G_SOURCE_REMOVE;
    }

    unsigned event_type = vdo_map_get_uint32(vdo_event, "event", 0);
    unsigned stream_id  = vdo_map_get_uint32(vdo_event, "id", 0);

    if (event_type == VDO_STREAM_EVENT_EXISTING || event_type == VDO_STREAM_EVENT_CREATED) {
        /*
         * A new stream or a stream which already existed at the time our
         * application started. In this example we want to add one overlay to every
         * such stream.
         */
        vdo_stream = vdo_stream_get(stream_id, &error);
        if (!vdo_stream) {
            syslog(LOG_ERR, "Failed to get stream information from vdo: %s", error->message);
            g_main_loop_quit(main_loop);
            return G_SOURCE_REMOVE;
        }

        stream_info = vdo_stream_get_info(vdo_stream, nullptr);
        if (!stream_info) {
            syslog(LOG_ERR, "Vdo stream is missing info");
            g_main_loop_quit(main_loop);
            return G_SOURCE_REMOVE;
        }

        if (debug)
            vdo_map_dump(stream_info);

        unsigned width  = vdo_map_get_uint32(stream_info, "width", 0);
        unsigned height = vdo_map_get_uint32(stream_info, "height", 0);
        if (!width || !height) {
            syslog(LOG_ERR, "Vdo reported invalid stream size %ux%u", width, height);
            g_main_loop_quit(main_loop);
            return G_SOURCE_REMOVE;
        }

        create_overlay(stream_id, width, height);
    } else if (event_type == VDO_STREAM_EVENT_CLOSED) {
        /* A stream closed down, so we need to clean up the overlay on that stream
         */
        remove_overlay(stream_id);
    }

    return G_SOURCE_CONTINUE;
}

/*
 * Create an overlay on the specified stream.
 *
 * Note that it is possible to create multiple overlays per stream, or we could
 * filter out streams further based on the properties we get from VDO. In this
 * example we just create one overlay on every stream.
 */
static void create_overlay(unsigned stream_id, unsigned stream_width, unsigned stream_height) {
    AxoErr axo_error         = {};
    AxoProps props           = nullptr;
    AxoMatch match           = nullptr;
    AxoDetailedFormat format = nullptr;

    /*
     * Find a suitable format to use for the overlay. We want to draw a full-colour overlay with
     * transparency (ARGB32). We also want to enable compression if possible.
     *
     * It is recommended to enable compression whenever the GPU is used for rendering, as this
     * improves system performance.
     *
     * Note that compression is not always available. If not, Axoverlay will simply return a format
     * corresponding to uncompressed ARGB32 which can be used in the same way.
     */
    format.reset(axo_suggest_detailed_format(
        AXO_FORMAT_ARGB32,
        (axo_format_flags)(AXO_FORMAT_FLAGS_COMPRESSED | AXO_FORMAT_FLAGS_GPU),
        &axo_error.err));
    if (!format) {
        syslog(LOG_ERR, "Failed to find a suitable format %s", axo_err_get_message(axo_error.err));
        return;
    }

    /* In this example we scale the overlay to a given fraction of the stream size
     */
    unsigned overlay_size = MIN(stream_width, stream_height) / 8;

    /*
     * For streams of high resolution, the overlay will become very big. This can
     * require excessive memory and CPU/GPU time.
     *
     * In such cases it is useful to enable the built-in upscaling function. The
     * upscaling function lets us draw in half resolution compared to what will be
     * visible in the stream.
     *
     * Here we use a threshold of 4 megapixel for when to enable upscaling.
     */
    bool use_upscale = stream_width * stream_height > 4000000;

    if (use_upscale)
        overlay_size /= 2;

    unsigned overlay_used_width = overlay_size, overlay_used_height = overlay_size;

    /*
     * It is important to ensure that the size is properly aligned. The
     * calculations above can result in odd numbers and other dimensions which are
     * not supported by the overlay system. This utility function shall always be
     * used to calculate the required padding.
     */
    unsigned overlay_full_width, overlay_full_height;
    axo_detailed_format_get_aligned_size(format.get(),
                                         overlay_used_width,
                                         overlay_used_height,
                                         &overlay_full_width,
                                         &overlay_full_height);

    /*
     * The GPU driver may require a more strict alignment than Axoverlay does. It is assumed that
     * 16x16 alignment will be sufficient in any case.
     */
    overlay_full_width  = (overlay_full_width + 15) & ~15u;
    overlay_full_height = (overlay_full_height + 15) & ~15u;

    /* Create the overlay */
    props.reset(axo_props_new());
    axo_props_set_detailed_format(props.get(), format.get());
    axo_props_set_size(props.get(), overlay_full_width, overlay_full_height);
    axo_props_set_upscale_x2(props.get(), use_upscale);

    /*
     * When GPU rendering is used, there is no need to do CPU cache maintenance of the overlay
     * buffers because the CPU will not access them. This saves some work on each frame.
     */
    axo_props_set_manual_dma_sync(props.get(), true);

    match.reset(axo_match_new());
    axo_match_stream_id(match.get(), stream_id);

    int overlay_id = axo_create_overlay(props.get(), match.get(), &axo_error.err);
    if (overlay_id < 0) {
        /*
         * It can happen that the stream closes down before we have time to create
         * an overlay on it. This is not an error. The condition is indicated by a
         * special code. In this case we will soon receive a disconnect event from
         * VDO, so just ignore it and keep going.
         */
        if (axo_err_get_code(axo_error.err) != AXO_ERR_NO_STREAM)
            syslog(LOG_ERR,
                   "Failed to create overlay on stream %d: %s",
                   stream_id,
                   axo_err_get_message(axo_error.err));
        return;
    }

    syslog(LOG_INFO,
           "Created overlay %u on stream %u, stream_size=%ux%u "
           "overlay_used_size=%ux%u "
           "overlay_full_size=%ux%u",
           overlay_id,
           stream_id,
           stream_width,
           stream_height,
           overlay_used_width,
           overlay_used_height,
           overlay_full_width,
           overlay_full_height);

    /* Create and store a record of this overlay in the overlay table */
    overlay_table[stream_id] = {
        .overlay_id    = overlay_id,
        .stream_id     = stream_id,
        .format        = std::move(format),
        .used_width    = overlay_used_width,
        .used_height   = overlay_used_height,
        .full_width    = overlay_full_width,
        .full_height   = overlay_full_height,
        .frame_count   = 0,
        .surface_cache = {},
    };

    /* Process the first frame for this new overlay */
    process_next_frame(&overlay_table[stream_id]);
}

/*
 * Remove the overlay on the specified stream, if one existed.
 */
static void remove_overlay(unsigned stream_id) {
    AxoErr axo_error = {};

    auto it = overlay_table.find(stream_id);
    if (it == overlay_table.end())
        return;

    const Overlay* overlay = &it->second;

    if (!axo_remove_overlay(overlay->overlay_id, &axo_error.err)) {
        syslog(LOG_ERR,
               "Failed to remove overlay %u on stream %u: %s",
               overlay->overlay_id,
               stream_id,
               axo_err_get_message(axo_error.err));
        return;
    }

    syslog(LOG_INFO, "Removed overlay %u from stream %u", overlay->overlay_id, stream_id);

    overlay_table.erase(it);
}

/*
 * Draw and submit a new frame of animation for this overlay.
 */
static void process_next_frame(Overlay* overlay) {
    AxoErr axo_error = {};

    /* Get a free buffer to draw into from the overlay system */
    axo_buffer* buffer = axo_get_buffer(overlay->overlay_id, nullptr, &axo_error.err);
    if (!buffer) {
        axo_err_code code = axo_err_get_code(axo_error.err);

        /*
         * There are some situations where a buffer is not available within a
         * reasonable time. This happens during normal camera usage. All clients
         * must handle these conditions by checking for specific error codes. Please
         * see the API documentation for axo_get_buffer.
         */
        if (code == AXO_ERR_NO_STREAM || code == AXO_ERR_WAIT)
            return;

        syslog(LOG_ERR,
               "Failed to get buffer for overlay %u: %s",
               overlay->overlay_id,
               axo_err_get_message(axo_error.err));
        return;
    }

    unsigned long buffer_id = axo_buffer_get_id(buffer);
    int dma_buf_fd          = axo_buffer_get_dma_buf_fd(buffer);
    if (dma_buf_fd < 0) {
        syslog(LOG_ERR, "No dma buf for buffer");
        return;
    }

    /* See if there is a cached surface that can be re-used */
    auto it = overlay->surface_cache.find(buffer_id);
    if (it == overlay->surface_cache.end()) {
        /* First time rendering for this frame so create a new surface */
        std::unique_ptr<RenderSurface> surface = create_render_surface(*overlay, dma_buf_fd);
        if (!surface) {
            syslog(LOG_ERR, "Failed to create render surface for buffer");
            return;
        }

        overlay->surface_cache[buffer_id] = std::move(surface);
    }

    render_frame(*overlay, &*overlay->surface_cache[buffer_id]);

    /* Submit the buffer to be shown on the stream */
    if (!axo_submit_buffer(buffer, nullptr, &axo_error.err)) {
        syslog(LOG_ERR,
               "Failed to submit buffer for overlay %u: %s",
               overlay->overlay_id,
               axo_err_get_message(axo_error.err));
        return;
    }

    /*
     * Log the first frame and then once per second (the tick runs at 30 fps) so the system log
     * confirms that rendering is ongoing without flooding it.
     */
    if (overlay->frame_count++ % 30 == 0)
        syslog(LOG_INFO,
               "Rendered %u frames for overlay %u on stream %u",
               overlay->frame_count,
               overlay->overlay_id,
               overlay->stream_id);
}

/*
 * "Import" an Axoverlay buffer into the GPU interface, setting up necessary context for the GPU to
 * render to the buffer. This only needs to be done once per buffer.
 */
static std::unique_ptr<RenderSurface> create_render_surface(const Overlay& overlay,
                                                            int dma_buf_fd) {
    std::unique_ptr<RenderSurface> surface = std::make_unique<RenderSurface>(render_context);

    uint32_t drm_fourcc   = axo_detailed_format_get_drm_fourcc(overlay.format.get());
    uint64_t drm_modifier = axo_detailed_format_get_drm_modifier(overlay.format.get(), 0);

    /* clang-format off */
    int image_attribs[] = {
        EGL_WIDTH, (int)overlay.full_width,
        EGL_HEIGHT, (int)overlay.full_height,
        EGL_LINUX_DRM_FOURCC_EXT, (int)drm_fourcc,
        EGL_DMA_BUF_PLANE0_FD_EXT, dma_buf_fd,
        EGL_DMA_BUF_PLANE0_OFFSET_EXT, 0,
        EGL_DMA_BUF_PLANE0_PITCH_EXT, (int)overlay.full_width * 4,
        EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT, (int)(drm_modifier & 0xffffffff),
        EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT, (int)(drm_modifier >> 32),
        EGL_NONE,
    };
    /* clang-format on */

    static auto eglCreateImageKHR =
        (PFNEGLCREATEIMAGEKHRPROC)eglGetProcAddress("eglCreateImageKHR");
    static auto glEGLImageTargetTexture2DOES =
        (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)eglGetProcAddress("glEGLImageTargetTexture2DOES");

    surface->image = eglCreateImageKHR(render_context->display,
                                       EGL_NO_CONTEXT,
                                       EGL_LINUX_DMA_BUF_EXT,
                                       nullptr,
                                       image_attribs);
    if (egl_check_error("create EGL image"))
        return nullptr;

    if (surface->image == EGL_NO_IMAGE_KHR) {
        syslog(LOG_ERR, "Egl no image");
        return nullptr;
    }

    glGenTextures(1, &surface->texture);
    glBindTexture(GL_TEXTURE_2D, surface->texture);
    if (gl_check_error("bind GL texture"))
        return nullptr;

    glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, surface->image);
    if (gl_check_error("set GL texture target"))
        return nullptr;

    glBindTexture(GL_TEXTURE_2D, 0);

    GrGLTextureInfo texture_info = {
        .fTarget = GL_TEXTURE_2D,
        .fID     = surface->texture,
        .fFormat = GL_RGBA8,
    };
    GrBackendTexture backend_texture = GrBackendTextures::MakeGL((int)overlay.full_width,
                                                                 (int)overlay.full_height,
                                                                 skgpu::Mipmapped::kNo,
                                                                 texture_info);
    if (!backend_texture.isValid()) {
        syslog(LOG_ERR, "Failed to create backend texture for Skia");
        return nullptr;
    }

    surface->surface = SkSurfaces::WrapBackendTexture(surface->context->gr_context.get(),
                                                      backend_texture,
                                                      kTopLeft_GrSurfaceOrigin,
                                                      0,
                                                      kRGBA_8888_SkColorType,
                                                      nullptr,
                                                      nullptr);
    if (!surface->surface) {
        syslog(LOG_ERR, "Failed to wrap backend texture as Skia surface");
        return nullptr;
    }

    return surface;
}

static void render_frame(const Overlay& overlay, RenderSurface* surface) {
    SkCanvas* canvas = surface->surface->getCanvas();

    /* Skia save/restore operations ensure we start from a clean context each frame */
    canvas->save();

    draw_graphics(overlay, canvas);

    canvas->restore();

    /*
     * CPU sync implies that this operation will block until the results of the draw operation are
     * fully committed to memory.
     */
    render_context->gr_context->flushAndSubmit(GrSyncCpu::kYes);
}

static void draw_graphics(const Overlay& overlay, SkCanvas* canvas) {
    canvas->clear(SkColors::kTransparent);

    /*
     * Rescale coordinates so that (1.0, 1.0) is the bottom-right edge of the used
     * overlay area. The padding may extend slightly past this.
     */
    canvas->scale((float)overlay.used_width, (float)overlay.used_height);

    /* Animate the overlay with a simple rotation around the centre of the used
     * area */
    double angle_rad = M_PI * animation_state / 180.0;

    canvas->translate(0.5f, 0.5f);
    canvas->rotate((float)animation_state);
    canvas->translate(-0.5f, -0.5f);

    /* Animate colour selection */
    float r = (float)(sin(angle_rad) * sin(angle_rad));
    float g = (float)(cos(angle_rad) * cos(angle_rad));

    /* Draw an example icon */
    SkPaint paint;
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(0.04f);
    paint.setColor4f({r, g, 0.0f, 1.0f});
    paint.setAntiAlias(true);

    canvas->drawCircle(0.5f, 0.5f, 0.45f, paint);
    canvas->drawCircle(0.4f, 0.4f, 0.05f, paint);
    canvas->drawCircle(0.6f, 0.4f, 0.05f, paint);
    canvas->drawArc(SkRect::MakeLTRB(0.2f, 0.2f, 0.8f, 0.8f), 0.0f, 180.0f, false, paint);
}
