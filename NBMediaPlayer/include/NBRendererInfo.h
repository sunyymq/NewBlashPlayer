//
// Created by liuenbao on 18-9-15.
//

#ifndef NBRENDERERINFO_H
#define NBRENDERERINFO_H

#include <NBMacros.h>

#ifdef BUILD_TARGET_LINUX64
#include <X11/Xlib.h>
typedef struct NBRendererTarget {
   ::Display* display;
   int screen;
   ::Window window;
} NBRendererTarget;
#elif BUILD_TARGET_IOS
typedef struct NBRendererTarget {
   void* params;
} NBRendererTarget;
#elif BUILD_TARGET_ANDROID
#include <android/native_window.h> // requires ndk r5 or newer
#include <EGL/egl.h>
typedef struct NBRendererTarget {
    void* params;
    EGLDisplay _display;
    EGLSurface _surface;
    EGLContext _context;
    ANativeWindow *window;
} NBRendererTarget;
#endif

void* getRendererCtx(NBRendererTarget* target);
nb_status_t prepareRendererCtx(NBRendererTarget* target);
void destroyRendererCtx(NBRendererTarget* target);
nb_status_t preRender(NBRendererTarget* target, NBRenderInfo* info);
nb_status_t postRender(NBRendererTarget* target);

#endif //NBRENDERERINFO_H
