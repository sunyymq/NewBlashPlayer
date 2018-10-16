#include "AndroidJni.h"

#include "AndroidArraylist.h"
#include "AndroidBuild.h"
#include "AndroidBundle.h"
#include "AndroidBytebuffer.h"
#include "AndroidMediaformatJava.h"
#include "AndroidMediacodecJava.h"
#include "AndroidHashMap.h"
#include "AndroidLong.h"
#include "AndroidInteger.h"
#include "AndroidString.h"
// #include "AndroidBitmapConfig.h"
// #include "AndroidBitmap.h"
#include "AndroidSurfaceTexture.h"
#include "AndroidSurface.h"

#include "JNIUtils.h"

#include <sys/system_properties.h>

static pthread_key_t g_thread_key;
static pthread_once_t g_key_once = PTHREAD_ONCE_INIT;

int LibAndroid_OnLoad(JNIEnv* env) {
    
    int retval;
    // JNIEnv* env = NULL;
    
    // if (getJavaVM()->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
    //     return -1;
    // }
    
    retval = ASDK_ArrayList__loadClass(env);
    JNI_CHECK_RET(retval == 0, env, NULL, NULL, -1);
    retval = ASDK_Build__loadClass(env);
    JNI_CHECK_RET(retval == 0, env, NULL, NULL, -1);
    retval = ASDK_Bundle__loadClass(env);
    JNI_CHECK_RET(retval == 0, env, NULL, NULL, -1);
    
    // retval = Android_AudioTrack_global_init(env);
    // JNI_CHECK_RET(retval == 0, env, NULL, NULL, -1);
    
    retval = ASDK_ByteBuffer__loadClass(env);
    JNI_CHECK_RET(retval == 0, env, NULL, NULL, -1);
    retval = AMediaFormatJava__loadClass(env);
    JNI_CHECK_RET(retval == 0, env, NULL, NULL, -1);
    retval = AMediaCodecJava__loadClass(env);
    JNI_CHECK_RET(retval == 0, env, NULL, NULL, -1);

    retval = ASDK_HashMap__loadClass(env);
    JNI_CHECK_RET(retval == 0, env, NULL, NULL, -1);

    retval = ASDK_Long__loadClass(env);
    JNI_CHECK_RET(retval == 0, env, NULL, NULL, -1);

    retval = ASDK_Integer__loadClass(env);
    JNI_CHECK_RET(retval == 0, env, NULL, NULL, -1);

    retval = ASDK_String__loadClass(env);
    JNI_CHECK_RET(retval == 0, env, NULL, NULL, -1);

    // retval = ASDK_BitmapConfig__loadClass(env);
    // JNI_CHECK_RET(retval == 0, env, NULL, NULL, -1);

    // retval = ASDK_Bitmap__loadClass(env);
    // JNI_CHECK_RET(retval == 0, env, NULL, NULL, -1);

    retval = ASDK_SurfaceTexture__loadClass(env);
    JNI_CHECK_RET(retval == 0, env, NULL, NULL, -1);

    retval = ASDK_Surface__loadClass(env);
    JNI_CHECK_RET(retval == 0, env, NULL, NULL, -1);

    // retval = ASDK_TDOnFrameAvailableListener__loadClass(env);
    // JNI_CHECK_RET(retval == 0, env, NULL, NULL, -1);

    return 0;
}

void LibAndroid_OnUnLoad(JNIEnv* env) {
    
}

static void JNI_ThreadDestroyed(void* value) {
    JNIEnv *env = (JNIEnv*) value;
    if (env != NULL) {
        getJavaVM()->DetachCurrentThread();
        pthread_setspecific(g_thread_key, NULL);
    }
}

static void make_thread_key() {
    pthread_key_create(&g_thread_key, JNI_ThreadDestroyed);
}

jint JNI_SetupThreadEnv(JNIEnv **p_env) {
    JavaVM *jvm = getJavaVM();
    if (!jvm) {
        SDLTRACE("JNI_GetJvm: AttachCurrentThread: NULL jvm");
        return -1;
    }

    pthread_once(&g_key_once, make_thread_key);

    JNIEnv *env = (JNIEnv*) pthread_getspecific(g_thread_key);
    if (env) {
        *p_env = env;
        return 0;
    }

    if (jvm->AttachCurrentThread(&env, NULL) == JNI_OK) {
        pthread_setspecific(g_thread_key, env);
        *p_env = env;
        return 0;
    }

    return -1;
}

void JNI_ThrowException(JNIEnv *env, const char* msg) {
    jniThrowException(env, "java/lang/IllegalStateException", msg);
}

jboolean JNI_RethrowException(JNIEnv *env) {
    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        return JNI_TRUE;
    }

    return JNI_FALSE;
}

jboolean JNI_CatchException(JNIEnv *env) {
    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        return JNI_TRUE;
    }

    return JNI_FALSE;
}

jobject JNI_NewObjectAsGlobalRef(JNIEnv *env, jclass clazz, jmethodID methodID, ...) {
    va_list args;
    va_start(args, methodID);

    jobject global_object = NULL;
    jobject local_object = env->NewObjectV(clazz, methodID, args);
    if (!JNI_RethrowException(env) && local_object) {
        global_object = env->NewGlobalRef(local_object);
        JNI_DeleteLocalRefP(env, &local_object);
    }

    va_end(args);
    return global_object;
}

void JNI_DeleteGlobalRefP(JNIEnv *env, jobject *obj_ptr) {
    if (!obj_ptr || !*obj_ptr)
        return;

    env->DeleteGlobalRef(*obj_ptr);
    *obj_ptr = NULL;
}

void JNI_DeleteLocalRefP(JNIEnv *env, jobject *obj_ptr) {
    if (!obj_ptr || !*obj_ptr)
        return;

    env->DeleteLocalRef(*obj_ptr);
    *obj_ptr = NULL;
}


int Android_GetApiLevel() {
    static int SDK_INT = 0;
    if (SDK_INT > 0)
        return SDK_INT;

    JNIEnv *env = NULL;
    if (JNI_OK != JNI_SetupThreadEnv(&env)) {
        SDLTRACE("Android_GetApiLevel: SetupThreadEnv failed");
        return 0;
    }

    SDK_INT = ASDK_Build_VERSION__SDK_INT(env);
    return SDK_INT;
#if 0
    char value[PROP_VALUE_MAX];
    memset(value, 0, sizeof(value));
    __system_property_get("ro.build.version.sdk", value);
    SDK_INT = atoi(value);
    return SDK_INT;
#endif
}