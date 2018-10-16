#include "AndroidMediaformatJava.h"

#include "AndroidJni.h"
#include "AndroidMediaformatInternal.h"
#include "AndroidBytebuffer.h"

typedef struct AMediaFormat_Opaque {
    jobject android_media_format;

    jobject android_byte_buffer;
} AMediaFormat_Opaque;

typedef struct AMediaFormatJava_fields_t {
    jclass clazz;

    jmethodID jmid__ctor;

    jmethodID jmid_createAudioFormat;
    jmethodID jmid_createVideoFormat;

    jmethodID jmid_getInteger;
    jmethodID jmid_setInteger;

    jmethodID jmid_setByteBuffer;

} AMediaFormatJava_fields_t;
static AMediaFormatJava_fields_t g_clazz;

int AMediaFormatJava__loadClass(JNIEnv *env) {
    jint sdk_int = Android_GetApiLevel();
    if (sdk_int < API_16_JELLY_BEAN) {
        return 0;
    }

    FIND_JAVA_CLASS( env, g_clazz.clazz, "android/media/MediaFormat");

    FIND_JAVA_STATIC_METHOD(env, g_clazz.jmid_createAudioFormat,    g_clazz.clazz,
        "createAudioFormat",    "(Ljava/lang/String;II)Landroid/media/MediaFormat;");
    FIND_JAVA_STATIC_METHOD(env, g_clazz.jmid_createVideoFormat,    g_clazz.clazz,
        "createVideoFormat",    "(Ljava/lang/String;II)Landroid/media/MediaFormat;");

    FIND_JAVA_METHOD(env, g_clazz.jmid__ctor,           g_clazz.clazz,
        "<init>"    ,           "()V");
    FIND_JAVA_METHOD(env, g_clazz.jmid_getInteger,      g_clazz.clazz,
        "getInteger",           "(Ljava/lang/String;)I");
    FIND_JAVA_METHOD(env, g_clazz.jmid_setInteger,      g_clazz.clazz,
        "setInteger",           "(Ljava/lang/String;I)V");
    FIND_JAVA_METHOD(env, g_clazz.jmid_setByteBuffer,   g_clazz.clazz,
        "setByteBuffer",        "(Ljava/lang/String;Ljava/nio/ByteBuffer;)V");

    SDLTRACE("android.media.MediaFormat class loaded");

    return 0;
}

jobject AMediaFormatJava_getObject(JNIEnv *env, const AMediaFormat *thiz) {
    if (!thiz || !thiz->opaque)
        return NULL;

    AMediaFormat_Opaque *opaque = (AMediaFormat_Opaque *)thiz->opaque;
    return opaque->android_media_format;
}

static jobject getAndroidMediaFormat(JNIEnv *env, const AMediaFormat* thiz) {
    if (!thiz)
        return NULL;

    AMediaFormat_Opaque *opaque = (AMediaFormat_Opaque *)thiz->opaque;
    if (!opaque)
        return NULL;

    return opaque->android_media_format;
}

static amedia_status_t AMediaFormatJava_delete(AMediaFormat* aformat) {
    if (!aformat)
        return AMEDIA_OK;

    JNIEnv *env = NULL;
    if (JNI_OK != JNI_SetupThreadEnv(&env)) {
        SDLTRACE("%s: SetupThreadEnv failed", __func__);
        return AMEDIA_ERROR_UNKNOWN;
    }

    AMediaFormat_Opaque *opaque = (AMediaFormat_Opaque *)aformat->opaque;
    if (opaque) {
        JNI_DeleteGlobalRefP(env, &opaque->android_byte_buffer);
        JNI_DeleteGlobalRefP(env, &opaque->android_media_format);
    }

    AMediaFormat_FreeInternal(aformat);
    return AMEDIA_OK;
}

static bool AMediaFormatJava_getInt32(AMediaFormat* aformat, const char* name, int32_t *out) {
    JNIEnv *env = NULL;
    if (JNI_OK != JNI_SetupThreadEnv(&env)) {
        SDLTRACE("%s: JNI_SetupThreadEnv: failed", __func__);
        return false;
    }

    jobject android_media_format = getAndroidMediaFormat(env, aformat);
    if (!android_media_format) {
        SDLTRACE("%s: getAndroidMediaFormat: failed", __func__);
        return false;
    }

    jstring jname = env->NewStringUTF(name);
    if (JNI_CatchException(env) || !jname) {
        SDLTRACE("%s: NewStringUTF: failed", __func__);
        return false;
    }

    jint ret = env->CallIntMethod(android_media_format, g_clazz.jmid_getInteger, jname);
    JNI_DeleteLocalRefP(env, (jobject*)&jname);
    if (JNI_CatchException(env)) {
        SDLTRACE("%s: CallIntMethod: failed", __func__);
        return false;
    }

    if (out)
        *out = ret;
    return true;
}

static void AMediaFormatJava_setInt32(AMediaFormat* aformat, const char* name, int32_t value) {
    JNIEnv *env = NULL;
    if (JNI_OK != JNI_SetupThreadEnv(&env)) {
        SDLTRACE("%s: JNI_SetupThreadEnv: failed", __func__);
        return;
    }

    jobject android_media_format = getAndroidMediaFormat(env, aformat);
    if (!android_media_format) {
        SDLTRACE("%s: getAndroidMediaFormat: failed", __func__);
        return;
    }

    jstring jname = env->NewStringUTF(name);
    if (JNI_CatchException(env) || !jname) {
        SDLTRACE("%s: NewStringUTF: failed", __func__);
        return;
    }

    env->CallVoidMethod(android_media_format, g_clazz.jmid_setInteger, jname, value);
    JNI_DeleteLocalRefP(env, (jobject*)&jname);
    if (JNI_CatchException(env)) {
        SDLTRACE("%s: CallVoidMethod: failed", __func__);
        return;
    }
}

static void AMediaFormatJava_setBuffer(AMediaFormat* aformat, const char* name, void* data, size_t size) {
    JNIEnv *env = NULL;
    if (JNI_OK != JNI_SetupThreadEnv(&env)) {
        SDLTRACE("%s: JNI_SetupThreadEnv: failed", __func__);
        return;
    }

    AMediaFormat_Opaque *opaque = (AMediaFormat_Opaque *)aformat->opaque;
    jobject android_media_format = opaque->android_media_format;
    if (!opaque->android_byte_buffer) {
        jobject local_android_byte_buffer = ASDK_ByteBuffer_allocateDirectAsGlobalRef(env, size);
        if (JNI_CatchException(env) || !local_android_byte_buffer) {
            SDLTRACE("%s: ASDK_ByteBuffer_allocateDirectAsGlobalRef: failed", __func__);
            return;
        }
        opaque->android_byte_buffer = local_android_byte_buffer;;
    }

    ASDK_ByteBuffer__setDataLimited(env, opaque->android_byte_buffer, data, size);
    if (JNI_CatchException(env)) {
        SDLTRACE("%s: ASDK_ByteBuffer__setDataLimited: failed", __func__);
        return;
    }

    jstring jname = env->NewStringUTF(name);
    if (JNI_CatchException(env) || !jname) {
        SDLTRACE("%s: NewStringUTF: failed", __func__);
        return;
    }

    env->CallVoidMethod(android_media_format, g_clazz.jmid_setByteBuffer, jname, opaque->android_byte_buffer);
    JNI_DeleteLocalRefP(env, (jobject*)&jname);
    if (JNI_CatchException(env)) {
        SDLTRACE("%s: call jmid_setByteBuffer: failed", __func__);
        return;
    }
}

static void setup_aformat(AMediaFormat *aformat, jobject android_media_format) {
    AMediaFormat_Opaque *opaque = aformat->opaque;
    opaque->android_media_format = android_media_format;

    aformat->func_delete    = AMediaFormatJava_delete;
    aformat->func_getInt32  = AMediaFormatJava_getInt32;
    aformat->func_setInt32  = AMediaFormatJava_setInt32;
    aformat->func_setBuffer = AMediaFormatJava_setBuffer;
}

AMediaFormat *AMediaFormatJava_new(JNIEnv *env) {
    SDLTRACE("%s", __func__);
    jobject android_media_format = JNI_NewObjectAsGlobalRef(env, g_clazz.clazz, g_clazz.jmid__ctor);
    if (JNI_CatchException(env) || !android_media_format) {
        return NULL;
    }

    AMediaFormat *aformat = AMediaFormat_CreateInternal(sizeof(AMediaFormat_Opaque));
    if (!aformat) {
        JNI_DeleteGlobalRefP(env, &android_media_format);
        return NULL;
    }

    setup_aformat(aformat, android_media_format);
    return aformat;
}

AMediaFormat *AMediaFormatJava_init(JNIEnv *env, jobject android_format) {
    SDLTRACE("%s", __func__);
    jobject global_android_media_format = env->NewGlobalRef(android_format);
    if (JNI_CatchException(env) || !global_android_media_format) {
        return NULL;
    }

    AMediaFormat *aformat = AMediaFormat_CreateInternal(sizeof(AMediaFormat_Opaque));
    if (!aformat) {
        JNI_DeleteGlobalRefP(env, &global_android_media_format);
        return NULL;
    }

    setup_aformat(aformat, global_android_media_format);
    return aformat;
}

AMediaFormat *AMediaFormatJava_createVideoFormat(JNIEnv *env, const char *mime, int width, int height) {
    SDLTRACE("%s", __func__);
    jstring jmime = env->NewStringUTF(mime);
    if (JNI_CatchException(env) || !jmime) {
        return NULL;
    }

    jobject local_android_media_format = env->CallStaticObjectMethod(g_clazz.clazz, g_clazz.jmid_createVideoFormat, jmime, width, height);
    JNI_DeleteLocalRefP(env, (jobject*)&jmime);
    if (JNI_CatchException(env) || !local_android_media_format) {
        return NULL;
    }

    jobject global_android_media_format = env->NewGlobalRef(local_android_media_format);
    JNI_DeleteLocalRefP(env, &local_android_media_format);
    if (JNI_CatchException(env) || !global_android_media_format) {
        return NULL;
    }

    AMediaFormat *aformat = AMediaFormat_CreateInternal(sizeof(AMediaFormat_Opaque));
    if (!aformat) {
        JNI_DeleteGlobalRefP(env, &global_android_media_format);
        return NULL;
    }

    setup_aformat(aformat, global_android_media_format);
    AMediaFormat_setInt32(aformat, AMEDIAFORMAT_KEY_MAX_INPUT_SIZE, 0);
    return aformat;
}
