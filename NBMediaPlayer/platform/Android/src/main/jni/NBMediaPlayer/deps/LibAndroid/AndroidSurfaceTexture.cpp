#include "AndroidSurfaceTexture.h"
#include "AndroidJni.h"

typedef struct ASDK_SurfaceTexture_fields_t {
    jclass clazz;

    jmethodID jmid_init;
    jmethodID jmid_setOnFrameAvailableListener;
    jmethodID jmid_updateTexImage;
    jmethodID jmid_releaseTexImage;
    jmethodID jmid_release;
} ASDK_SurfaceTexture_fields_t;

static ASDK_SurfaceTexture_fields_t g_clazz;

int ASDK_SurfaceTexture__loadClass(JNIEnv *env) {

    FIND_JAVA_CLASS( env, g_clazz.clazz, "android/graphics/SurfaceTexture");

    FIND_JAVA_METHOD(env, g_clazz.jmid_init,    g_clazz.clazz,
        "<init>",                   "(I)V");

    FIND_JAVA_METHOD(env, g_clazz.jmid_setOnFrameAvailableListener, g_clazz.clazz,
        "setOnFrameAvailableListener", "(Landroid/graphics/SurfaceTexture$OnFrameAvailableListener;)V");

    FIND_JAVA_METHOD(env, g_clazz.jmid_updateTexImage, g_clazz.clazz,
        "updateTexImage",      "()V");

    // FIND_JAVA_METHOD(env, g_clazz.jmid_releaseTexImage, g_clazz.clazz,
    //     "releaseTexImage", "()V");

    FIND_JAVA_METHOD(env, g_clazz.jmid_release, g_clazz.clazz, "release", "()V");

    SDLTRACE("android.graphics.SurfaceTexture class loaded");
    return 0;
}

jobject  ASDK_SurfaceTexture__init(JNIEnv *env, int texName) {
    jobject local_ref = env->NewObject(g_clazz.clazz, g_clazz.jmid_init, texName);

    if (JNI_CatchException(env) || !local_ref) {
        return NULL;
    }

    return local_ref;
}

jobject ASDK_SurfaceTexture__initWithGlobalRef(JNIEnv* env, int texName) {
    jobject local_ref = env->NewObject(g_clazz.clazz, g_clazz.jmid_init, texName);

    if (JNI_CatchException(env) || !local_ref) {
        return NULL;
    }

    jobject global_ref = env->NewGlobalRef(local_ref);
    JNI_DeleteLocalRefP(env, &local_ref);
    return global_ref;
}

void ASDK_SurfaceTexture__setOnFrameAvailableListener(JNIEnv* env, jobject surfaceTexture, jobject listener) {
    env->CallVoidMethod(surfaceTexture, g_clazz.jmid_setOnFrameAvailableListener, listener);

    if (JNI_CatchException(env)) {
        return ;
    }
}

bool ASDK_SurfaceTexture__updateTexImage(JNIEnv* env, jobject surfaceTexture) {
    env->CallVoidMethod(surfaceTexture, g_clazz.jmid_updateTexImage);

    if (JNI_CatchException(env)) {
        SDLTRACE("ASDK_SurfaceTexture__updateTexImage catch an exception");
        return false;
    }

    return true;
}

bool ASDK_SurfaceTexture__releaseTexImage(JNIEnv* env, jobject surfaceTexture) {
    // env->CallVoidMethod(surfaceTexture, g_clazz.jmid_releaseTexImage);

    // if (JNI_CatchException(env)) {
    //     SDLTRACE("ASDK_SurfaceTexture__updateTexImage catch an exception");
    //     return false;
    // }

    return true;
}

bool ASDK_SurfaceTexture__release(JNIEnv* env, jobject surfaceTexture) {
    env->CallVoidMethod(surfaceTexture, g_clazz.jmid_release);

    if (JNI_CatchException(env)) {
        SDLTRACE("ASDK_SurfaceTexture__updateTexImage catch an exception");
        return false;
    }

    return true;
}