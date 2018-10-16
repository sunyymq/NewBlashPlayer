//
// Created by liuenbao on 18-9-15.
//

#include <NBRendererInfo.h>
#include <android/native_window_jni.h> // requires ndk r5 or newer
#include <JNIUtils.h>
#include <GLES2/gl2.h>
#include <NBLog.h>

#define LOG_TAG "NBRenderContext"

nb_status_t prepareRendererCtx(NBRendererTarget* target) {
    target->window = ANativeWindow_fromSurface(getJNIEnv(), (jobject)target->params);
    if (target->window == NULL) {
        return UNINITIALIZED;
    }

    const EGLint attribs[] =
    {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL_NONE
    };

    int contextAttrs[] =
    {
            EGL_CONTEXT_CLIENT_VERSION, 2,
            EGL_NONE,
    };

    EGLDisplay display;
    EGLConfig config;
    EGLint numConfigs;
    EGLint format;
    EGLSurface surface;
    EGLContext context;
    EGLint width;
    EGLint height;
    GLfloat ratio;

    if ((display = eglGetDisplay(EGL_DEFAULT_DISPLAY)) == EGL_NO_DISPLAY) {
        NBLOG_ERROR(LOG_TAG, "eglGetDisplay() returned error %d", eglGetError());
        return false;
    }
    if (!eglInitialize(display, 0, 0)) {
        NBLOG_ERROR(LOG_TAG, "eglInitialize() returned error %d", eglGetError());
        return false;
    }

    if (!eglChooseConfig(display, attribs, &config, 1, &numConfigs)) {
        NBLOG_ERROR(LOG_TAG, "eglChooseConfig() returned error %d", eglGetError());
        return false;
    }

    if (!eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format)) {
        NBLOG_ERROR(LOG_TAG, "eglGetConfigAttrib() returned error %d", eglGetError());
        return false;
    }

    ANativeWindow_setBuffersGeometry(target->window, 0, 0, format);

    if (!(surface = eglCreateWindowSurface(display, config, target->window, 0))) {
        NBLOG_ERROR(LOG_TAG, "eglCreateWindowSurface() returned error %d", eglGetError());
        return false;
    }

    if (!(context = eglCreateContext(display, config, 0, contextAttrs))) {
        NBLOG_ERROR(LOG_TAG, "eglCreateContext() returned error %d", eglGetError());
        return false;
    }

    if (!eglMakeCurrent(display, surface, surface, context)) {
        NBLOG_ERROR(LOG_TAG, "eglMakeCurrent() returned error %d", eglGetError());
        return false;
    }

    if (!eglQuerySurface(display, surface, EGL_WIDTH, &width) ||
        !eglQuerySurface(display, surface, EGL_HEIGHT, &height)) {
        NBLOG_ERROR(LOG_TAG, "eglQuerySurface() returned error %d", eglGetError());
        return false;
    }

    target->_display = display;
    target->_surface = surface;
    target->_context = context;

    NBLOG_INFO(LOG_TAG, "The display is create complete");

    return OK;
}

void destroyRendererCtx(NBRendererTarget* target) {
    eglMakeCurrent(target->_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(target->_display, target->_context);
    eglDestroySurface(target->_display, target->_surface);
    eglTerminate(target->_display);

    target->_display = EGL_NO_DISPLAY;
    target->_surface = EGL_NO_SURFACE;
    target->_context = EGL_NO_CONTEXT;

    ANativeWindow_release(target->window);
}

nb_status_t preRender(NBRendererTarget* target, NBRenderInfo* info) {
    info->x = 0;
    info->y = 0;
    info->width = ANativeWindow_getWidth(target->window);
    info->height = ANativeWindow_getHeight(target->window);

//    NBLOG_INFO(LOG_TAG, "preRender width : %d height : %d", info->width, info->height);

    return OK;
}

nb_status_t postRender(NBRendererTarget* target) {
    // Display it
    if (!eglSwapBuffers(target->_display, target->_surface)) {
        NBLOG_ERROR(LOG_TAG, "eglSwapBuffers() returned error %d", eglGetError());
    }
    return OK;
}
