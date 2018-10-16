#include "AndroidSurface.h"
#include "AndroidJni.h"

typedef struct ASDK_Surface_fields_t {
    jclass clazz;

    jmethodID jmid_init;
} ASDK_Surface_fields_t;

static ASDK_Surface_fields_t g_clazz;

int ASDK_Surface__loadClass(JNIEnv *env) {

    FIND_JAVA_CLASS( env, g_clazz.clazz, "android/view/Surface");

    FIND_JAVA_METHOD(env, g_clazz.jmid_init,    g_clazz.clazz,
        "<init>",                   "(Landroid/graphics/SurfaceTexture;)V");

    SDLTRACE("android.view.Surface class loaded");
    return 0;
}

jobject  ASDK_Surface__init(JNIEnv *env, jobject surfaceTexture) {
    jobject local_ref = env->NewObject(g_clazz.clazz, g_clazz.jmid_init, surfaceTexture);

    if (JNI_CatchException(env) || !local_ref) {
        return NULL;
    }

    return local_ref;
}

jobject ASDK_Surface__initWithGlobalRef(JNIEnv* env, jobject surfaceTexture) {
    jobject local_ref = env->NewObject(g_clazz.clazz, g_clazz.jmid_init, surfaceTexture);

    if (JNI_CatchException(env) || !local_ref) {
        return NULL;
    }

    jobject global_ref = env->NewGlobalRef(local_ref);
    JNI_DeleteLocalRefP(env, &local_ref);
    return global_ref;
}