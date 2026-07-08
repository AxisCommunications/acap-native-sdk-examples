// Copyright (C) 2026 Axis Communications AB, Lund, Sweden
// Licensed under the MIT License. See LICENSE file for details.

/*
 * Simple GPU error handling. The numeric codes used by the GPU interface are translated into
 * descriptive strings.
 */
#pragma once

#include <EGL/egl.h>
#include <syslog.h>
#include <unordered_map>

inline bool egl_check_error(const char* what) {
    int e = eglGetError();
    if (e == EGL_SUCCESS)
        return false;

    static const std::unordered_map<int, std::pair<const char*, const char*>> error_table = {
        {EGL_NOT_INITIALIZED,
         {"EGL_NOT_INITIALIZED",
          "EGL is not initialized, or could not be initialized, for the "
          "specified EGL display connection"}},
        {EGL_BAD_ACCESS,
         {"EGL_BAD_ACCESS",
          "EGL cannot access a requested resource (for "
          "example a context is bound in another thread)"}},
        {EGL_BAD_ALLOC,
         {"EGL_BAD_ALLOC", "EGL failed to allocate resources for the requested operation"}},
        {EGL_BAD_ATTRIBUTE,
         {"EGL_BAD_ATTRIBUTE",
          "An unrecognized attribute or attribute value "
          "was passed in the attribute list"}},
        {EGL_BAD_CONTEXT,
         {"EGL_BAD_CONTEXT",
          "An EGLContext argument does not name a valid "
          "EGL rendering context"}},
        {EGL_BAD_CONFIG,
         {"EGL_BAD_CONFIG",
          "An EGLConfig argument does not name a valid EGL "
          "frame buffer configuration"}},
        {EGL_BAD_CURRENT_SURFACE,
         {"EGL_BAD_CURRENT_SURFACE",
          "The current surface of the calling thread is a window, pixel "
          "buffer or pixmap that is no longer valid"}},
        {EGL_BAD_DISPLAY,
         {"EGL_BAD_DISPLAY",
          "An EGLDisplay argument does not name a valid "
          "EGL display connection"}},
        {EGL_BAD_SURFACE,
         {"EGL_BAD_SURFACE",
          "An EGLSurface argument does not name a valid surface (window, "
          "pixel buffer or pixmap) configured for GL rendering"}},
        {EGL_BAD_MATCH,
         {"EGL_BAD_MATCH",
          "Arguments are inconsistent (for example, a valid context requires "
          "buffers not supplied by a valid surface)"}},
        {EGL_BAD_PARAMETER, {"EGL_BAD_PARAMETER", "One or more argument values are invalid"}},
        {EGL_BAD_NATIVE_PIXMAP,
         {"EGL_BAD_NATIVE_PIXMAP",
          "A NativePixmapType argument does not "
          "refer to a valid native pixmap"}},
        {EGL_BAD_NATIVE_WINDOW,
         {"EGL_BAD_NATIVE_WINDOW",
          "A NativeWindowType argument does not "
          "refer to a valid native window"}},
        {EGL_CONTEXT_LOST,
         {"EGL_CONTEXT_LOST",
          "A power management event has occurred. The application must "
          "destroy all contexts and reinitialise OpenGL ES state and objects "
          "to continue rendering"}},
    };

    auto it = error_table.find(e);
    if (it == error_table.end())
        syslog(LOG_ERR, "Failed to %s: EGL error %d", what, e);
    else
        syslog(LOG_ERR, "Failed to %s: %s: %s", what, it->second.first, it->second.second);

    return true;
}

inline bool gl_check_error(const char* what) {
    int e = glGetError();
    if (e == GL_NO_ERROR)
        return false;

    static const std::unordered_map<int, std::pair<const char*, const char*>> error_table = {
        {GL_INVALID_ENUM,
         {"GL_INVALID_ENUM",
          "An unacceptable value is specified for an enumerated argument. "
          "The offending command is ignored and has no other side effect "
          "than to set the error flag"}},
        {GL_INVALID_VALUE,
         {"GL_INVALID_VALUE",
          "A numeric argument is out of range. The offending command is "
          "ignored and has no other side effect than to set the error flag"}},
        {GL_INVALID_OPERATION,
         {"GL_INVALID_OPERATION",
          "The specified operation is not allowed in the current state. The "
          "offending command is ignored and has no other side effect than to "
          "set the error flag"}},
        {GL_INVALID_FRAMEBUFFER_OPERATION,
         {"GL_INVALID_FRAMEBUFFER_OPERATION",
          "The framebuffer object is not complete. The offending command is "
          "ignored and has no other side effect than to set the error flag"}},
        {GL_OUT_OF_MEMORY,
         {"GL_OUT_OF_MEMORY",
          "There is not enough memory left to execute the command. The state "
          "of the GL is undefined, except for the state of the error flags, "
          "after this error is recorded"}},
    };

    auto it = error_table.find(e);
    if (it == error_table.end())
        syslog(LOG_ERR, "Failed to %s: GL error %d", what, e);
    else
        syslog(LOG_ERR, "Failed to %s: %s: %s", what, it->second.first, it->second.second);

    return true;
}
