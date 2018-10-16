#include "AndroidMediacodecJava.h"
#include <assert.h>
#include "AndroidJni.h"
#include "AndroidMediacodecInternal.h"
#include "AndroidMediaformatJava.h"

static Class g_amediacodec_class = {
    "AMediaCodecJava"
};

typedef struct AMediaCodec_Opaque {
    jobject android_media_codec;

    jobjectArray    input_buffer_array;
    jobject         input_buffer;
    jobjectArray    output_buffer_array;
    jobject         output_buffer;
    jobject         output_buffer_info;

    bool            is_input_buffer_valid;
} AMediaCodec_Opaque;

typedef struct AMediaCodec_fields_t {
    jclass clazz;

    jmethodID jmid_createByCodecName;
    jmethodID jmid_createDecoderByType;
    jmethodID jmid_configure;
    jmethodID jmid_dequeueInputBuffer;
    jmethodID jmid_dequeueOutputBuffer;
    jmethodID jmid_flush;
    jmethodID jmid_getInputBuffers;
    jmethodID jmid_getOutputBuffers;
    jmethodID jmid_getOutputFormat;
    jmethodID jmid_queueInputBuffer;
    jmethodID jmid_release;
    jmethodID jmid_releaseOutputBuffer;
    jmethodID jmid_reset;
    jmethodID jmid_start;
    jmethodID jmid_stop;

    // >= API-18
    jmethodID jmid_getCodecInfo;
    jmethodID jmid_getName;
} AMediaCodec_fields_t;
static AMediaCodec_fields_t g_clazz;

typedef struct AMediaCodec_BufferInfo_fields_t {
    jclass clazz;

    jmethodID jmid__ctor;

    jfieldID jfid_flags;
    jfieldID jfid_offset;
    jfieldID jfid_presentationTimeUs;
    jfieldID jfid_size;
} AMediaCodec_BufferInfo_fields_t;
static AMediaCodec_BufferInfo_fields_t g_clazz_BufferInfo;

int AMediaCodecJava__loadClass(JNIEnv *env) {
    jint sdk_int = Android_GetApiLevel();
    SDLTRACE("MediaCodec: API-%d\n", sdk_int);
    if (sdk_int < API_16_JELLY_BEAN) {
        return 0;
    }

    //--------------------
    FIND_JAVA_CLASS( env, g_clazz.clazz, "android/media/MediaCodec");

    FIND_JAVA_STATIC_METHOD(env, g_clazz.jmid_createByCodecName,  g_clazz.clazz,
        "createByCodecName",  "(Ljava/lang/String;)Landroid/media/MediaCodec;");
    FIND_JAVA_STATIC_METHOD(env, g_clazz.jmid_createDecoderByType,  g_clazz.clazz,
        "createDecoderByType",  "(Ljava/lang/String;)Landroid/media/MediaCodec;");

    FIND_JAVA_METHOD(env, g_clazz.jmid_configure,            g_clazz.clazz,
        "configure",            "(Landroid/media/MediaFormat;Landroid/view/Surface;Landroid/media/MediaCrypto;I)V");
    FIND_JAVA_METHOD(env, g_clazz.jmid_dequeueInputBuffer,   g_clazz.clazz,
        "dequeueInputBuffer",   "(J)I");
    FIND_JAVA_METHOD(env, g_clazz.jmid_dequeueOutputBuffer,  g_clazz.clazz,
        "dequeueOutputBuffer",  "(Landroid/media/MediaCodec$BufferInfo;J)I");
    FIND_JAVA_METHOD(env, g_clazz.jmid_flush,                g_clazz.clazz,
        "flush",                "()V");
    FIND_JAVA_METHOD(env, g_clazz.jmid_getInputBuffers,      g_clazz.clazz,
        "getInputBuffers",      "()[Ljava/nio/ByteBuffer;");
    FIND_JAVA_METHOD(env, g_clazz.jmid_getOutputBuffers,     g_clazz.clazz,
        "getOutputBuffers",     "()[Ljava/nio/ByteBuffer;");
    FIND_JAVA_METHOD(env, g_clazz.jmid_getOutputFormat,     g_clazz.clazz,
        "getOutputFormat",      "()Landroid/media/MediaFormat;");
    FIND_JAVA_METHOD(env, g_clazz.jmid_queueInputBuffer,     g_clazz.clazz,
        "queueInputBuffer",     "(IIIJI)V");
    FIND_JAVA_METHOD(env, g_clazz.jmid_release,              g_clazz.clazz,
        "release",              "()V");
    FIND_JAVA_METHOD(env, g_clazz.jmid_releaseOutputBuffer,  g_clazz.clazz,
        "releaseOutputBuffer",  "(IZ)V");
    FIND_JAVA_METHOD(env, g_clazz.jmid_start,                g_clazz.clazz,
        "start",                "()V");
    FIND_JAVA_METHOD(env, g_clazz.jmid_stop,                 g_clazz.clazz,
        "stop",                 "()V");

    /*-
    if (sdk_int >= API_18_JELLY_BEAN_MR2) {
        FIND_JAVA_METHOD(env, g_clazz.jmid_getCodecInfo,         g_clazz.clazz,
            "getCodecInfo",         "(I)Landroid/media/MediaCodecInfo;");
        FIND_JAVA_METHOD(env, g_clazz.jmid_getName,              g_clazz.clazz,
            "getName",              "()Ljava/lang/String;");
    }
    */


    //--------------------
    FIND_JAVA_CLASS( env, g_clazz_BufferInfo.clazz, "android/media/MediaCodec$BufferInfo");

    FIND_JAVA_METHOD(env, g_clazz_BufferInfo.jmid__ctor,                g_clazz_BufferInfo.clazz,
        "<init>"    ,           "()V");

    FIND_JAVA_FIELD(env, g_clazz_BufferInfo.jfid_flags,                 g_clazz_BufferInfo.clazz,
        "flags",                "I");
    FIND_JAVA_FIELD(env, g_clazz_BufferInfo.jfid_offset,                g_clazz_BufferInfo.clazz,
        "offset",               "I");
    FIND_JAVA_FIELD(env, g_clazz_BufferInfo.jfid_presentationTimeUs,    g_clazz_BufferInfo.clazz,
        "presentationTimeUs",   "J");
    FIND_JAVA_FIELD(env, g_clazz_BufferInfo.jfid_size,                  g_clazz_BufferInfo.clazz,
        "size",                 "I");
    SDLTRACE("android.media.MediaCodec$BufferInfo class loaded");


    SDLTRACE("android.media.MediaCodec class loaded");
    return 0;
}

jobject AMediaCodecJava_getObject(JNIEnv *env, const AMediaCodec *thiz) {
    if (!thiz || !thiz->opaque)
        return NULL;

    AMediaCodec_Opaque *opaque = (AMediaCodec_Opaque *)thiz->opaque;
    return opaque->android_media_codec;
}

AMediaFormat *AMediaCodecJava_getOutputFormat(AMediaCodec *thiz) {
    if (!thiz || !thiz->opaque)
        return NULL;

    JNIEnv *env = NULL;
    if (JNI_OK != JNI_SetupThreadEnv(&env)) {
        SDLTRACE("%s: SetupThreadEnv failed", __func__);
        return NULL;
    }

    AMediaCodec_Opaque *opaque = (AMediaCodec_Opaque *)thiz->opaque;
    jobject android_format = env->CallObjectMethod(opaque->android_media_codec, g_clazz.jmid_getOutputFormat);
    if (JNI_CatchException(env) || !android_format) {
        return NULL;
    }

    AMediaFormat *aformat = AMediaFormatJava_init(env, android_format);
    JNI_DeleteLocalRefP(env, &android_format);
    return aformat;
}

static amedia_status_t AMediaCodecJava_delete(AMediaCodec* acodec) {
    SDLTRACE("%s\n", __func__);
    if (!acodec)
        return AMEDIA_OK;

    JNIEnv *env = NULL;
    if (JNI_OK != JNI_SetupThreadEnv(&env)) {
        SDLTRACE("AMediaCodecJava_delete: SetupThreadEnv failed");
        return AMEDIA_ERROR_UNKNOWN;
    }

    AMediaCodec_Opaque *opaque = (AMediaCodec_Opaque *)acodec->opaque;
    if (opaque) {
        if (opaque->android_media_codec) {
            env->CallVoidMethod(opaque->android_media_codec, g_clazz.jmid_release);
            JNI_CatchException(env);
        }

        JNI_DeleteGlobalRefP(env, &opaque->output_buffer_info);
        JNI_DeleteGlobalRefP(env, &opaque->output_buffer);
        JNI_DeleteGlobalRefP(env, (jobject*)&opaque->output_buffer_array);
        JNI_DeleteGlobalRefP(env, &opaque->input_buffer);
        JNI_DeleteGlobalRefP(env, (jobject*)&opaque->input_buffer_array);
        JNI_DeleteGlobalRefP(env, &opaque->android_media_codec);
    }

    AMediaCodec_FreeInternal(acodec);
    return AMEDIA_OK;
}

inline static int getInputBuffers(JNIEnv *env, AMediaCodec* acodec) {
    AMediaCodec_Opaque *opaque = (AMediaCodec_Opaque *)acodec->opaque;
    jobject android_media_codec = opaque->android_media_codec;
    JNI_DeleteGlobalRefP(env, (jobject*)&opaque->input_buffer_array);
    if (opaque->input_buffer_array)
        return 0;

    jobjectArray local_input_buffer_array = (jobjectArray)env->CallObjectMethod(android_media_codec, g_clazz.jmid_getInputBuffers);
    if (JNI_CatchException(env) || !local_input_buffer_array) {
        SDLTRACE("%s: getInputBuffers failed\n", __func__);
        return -1;
    }

    opaque->input_buffer_array = (jobjectArray)env->NewGlobalRef(local_input_buffer_array);
    JNI_DeleteLocalRefP(env, (jobject*)&local_input_buffer_array);
    if (JNI_CatchException(env) || !opaque->input_buffer_array) {
        SDLTRACE("%s: getInputBuffers.NewGlobalRef failed\n", __func__);
        return -1;
    }

    return 0;
}

inline static int getOutputBuffers(JNIEnv *env, AMediaCodec* acodec) {
    AMediaCodec_Opaque *opaque = (AMediaCodec_Opaque *)acodec->opaque;
    jobject android_media_codec = opaque->android_media_codec;
    JNI_DeleteGlobalRefP(env, (jobject*)&opaque->output_buffer_array);
    if (opaque->output_buffer_array)
        return 0;

    jobjectArray local_output_buffer_array = (jobjectArray)env->CallObjectMethod(android_media_codec, g_clazz.jmid_getOutputBuffers);
    if (JNI_CatchException(env) || !local_output_buffer_array) {
        SDLTRACE("%s: getInputBuffers failed\n", __func__);
        return -1;
    }

    opaque->output_buffer_array = (jobjectArray)env->NewGlobalRef(local_output_buffer_array);
    JNI_DeleteLocalRefP(env, (jobject*)&local_output_buffer_array);
    if (JNI_CatchException(env) || !opaque->output_buffer_array) {
        SDLTRACE("%s: getOutputBuffers.NewGlobalRef failed\n", __func__);
        return -1;
    }

    return 0;
}

static amedia_status_t AMediaCodecJava_configure_surface(
    JNIEnv*env,
    AMediaCodec* acodec,
    const AMediaFormat* aformat,
    jobject android_surface,
    AMediaCrypto *crypto,
    uint32_t flags) {
    SDLTRACE("AMediaCodecJava_configure_surface");

    // TODO: implement AMediaCrypto
    AMediaCodec_Opaque *opaque = (AMediaCodec_Opaque *)acodec->opaque;
    jobject android_media_format = AMediaFormatJava_getObject(env, aformat);
    jobject android_media_codec  = AMediaCodecJava_getObject(env, acodec);
    SDLTRACE("configure %p %p", android_media_codec, android_media_format);
    env->CallVoidMethod(android_media_codec, g_clazz.jmid_configure, android_media_format, android_surface, crypto, flags);
    if (JNI_CatchException(env)) {
        return AMEDIA_ERROR_UNKNOWN;
    }

    opaque->is_input_buffer_valid = true;
    JNI_DeleteGlobalRefP(env, (jobject*)&opaque->input_buffer_array);
    JNI_DeleteGlobalRefP(env, (jobject*)&opaque->output_buffer_array);
    return AMEDIA_OK;
}

static amedia_status_t AMediaCodecJava_start(AMediaCodec* acodec) {
    SDLTRACE("%s", __func__);

    JNIEnv *env = NULL;
    if (JNI_OK != JNI_SetupThreadEnv(&env)) {
        SDLTRACE("%s: SetupThreadEnv failed", __func__);
        return AMEDIA_ERROR_UNKNOWN;
    }

    AMediaCodec_Opaque *opaque = (AMediaCodec_Opaque *)acodec->opaque;
    jobject android_media_codec    = opaque->android_media_codec;
    env->CallVoidMethod(android_media_codec, g_clazz.jmid_start, android_media_codec);
    if (JNI_CatchException(env)) {
        SDLTRACE("%s: start failed", __func__);
        return AMEDIA_ERROR_UNKNOWN;
    }

    return AMEDIA_OK;
}

static amedia_status_t AMediaCodecJava_stop(AMediaCodec* acodec) {
    SDLTRACE("%s", __func__);

    JNIEnv *env = NULL;
    if (JNI_OK != JNI_SetupThreadEnv(&env)) {
        SDLTRACE("%s: SetupThreadEnv failed", __func__);
        return AMEDIA_ERROR_UNKNOWN;
    }

    jobject android_media_codec = AMediaCodecJava_getObject(env, acodec);
    env->CallVoidMethod(android_media_codec, g_clazz.jmid_stop, android_media_codec);
    if (JNI_CatchException(env)) {
        SDLTRACE("%s: stop", __func__);
        return AMEDIA_ERROR_UNKNOWN;
    }

    return AMEDIA_OK;
}

static amedia_status_t AMediaCodecJava_flush(AMediaCodec* acodec) {
    SDLTRACE("%s", __func__);

    JNIEnv *env = NULL;
    if (JNI_OK != JNI_SetupThreadEnv(&env)) {
        SDLTRACE("%s: SetupThreadEnv failed", __func__);
        return AMEDIA_ERROR_UNKNOWN;
    }

    jobject android_media_codec = AMediaCodecJava_getObject(env, acodec);
    env->CallVoidMethod(android_media_codec, g_clazz.jmid_flush, android_media_codec);
    if (JNI_CatchException(env)) {
        SDLTRACE("%s: flush", __func__);
        return AMEDIA_ERROR_UNKNOWN;
    }

    return AMEDIA_OK;
}

static uint8_t* AMediaCodecJava_getInputBuffer(AMediaCodec* acodec, size_t idx, size_t *out_size) {
//        SDLTRACE("%s", __func__);

    JNIEnv *env = NULL;
    if (JNI_OK != JNI_SetupThreadEnv(&env)) {
        SDLTRACE("%s: SetupThreadEnv failed", __func__);
        return NULL;
    }

    AMediaCodec_Opaque *opaque = (AMediaCodec_Opaque *)acodec->opaque;
    if (0 != getInputBuffers(env, acodec))
        return NULL;

    assert(opaque->input_buffer_array);
    int buffer_count = env->GetArrayLength(opaque->input_buffer_array);
    if (JNI_CatchException(env) || idx < 0 || idx >= buffer_count) {
        SDLTRACE("%s: idx(%d) < count(%d)\n", __func__, idx, buffer_count);
        return NULL;
    }

    JNI_DeleteGlobalRefP(env, &opaque->input_buffer);
    jobject local_input_buffer = env->GetObjectArrayElement(opaque->input_buffer_array, idx);
    if (JNI_CatchException(env) || !local_input_buffer) {
        SDLTRACE("%s: GetObjectArrayElement failed\n", __func__);
        return NULL;
    }
    opaque->input_buffer = env->NewGlobalRef(local_input_buffer);
    JNI_DeleteLocalRefP(env, &local_input_buffer);
    if (JNI_CatchException(env) || !opaque->input_buffer) {
        SDLTRACE("%s: GetObjectArrayElement.NewGlobalRef failed\n", __func__);
        return NULL;
    }

    jlong size = env->GetDirectBufferCapacity(opaque->input_buffer);
    void *ptr  = env->GetDirectBufferAddress(opaque->input_buffer);

    if (out_size)
        *out_size = size;
    return (uint8_t*)ptr;
}

static uint8_t* AMediaCodecJava_getOutputBuffer(AMediaCodec* acodec, size_t idx, size_t *out_size) {
//        SDLTRACE("%s", __func__);

    JNIEnv *env = NULL;
    if (JNI_OK != JNI_SetupThreadEnv(&env)) {
        SDLTRACE("AMediaCodecJava_getOutputBuffer: SetupThreadEnv failed");
        return NULL;
    }

    AMediaCodec_Opaque *opaque = (AMediaCodec_Opaque *)acodec->opaque;
    if (0 != getOutputBuffers(env, acodec))
        return NULL;

    assert(opaque->output_buffer_array);

    int buffer_count = env->GetArrayLength(opaque->output_buffer_array);
    if (JNI_CatchException(env) || idx < 0 || idx >= buffer_count) {
        return NULL;
    }

    JNI_DeleteGlobalRefP(env, &opaque->output_buffer);
    jobject local_output_buffer = env->GetObjectArrayElement(opaque->output_buffer_array, idx);
    if (JNI_CatchException(env) || !local_output_buffer) {
        return NULL;
    }
    opaque->output_buffer = env->NewGlobalRef(local_output_buffer);
    JNI_DeleteLocalRefP(env, &local_output_buffer);
    if (JNI_CatchException(env) || !opaque->output_buffer) {
        return NULL;
    }

    jlong size = env->GetDirectBufferCapacity(opaque->output_buffer);
    void *ptr  = env->GetDirectBufferAddress(opaque->output_buffer);

    if (out_size)
        *out_size = size;
    return (uint8_t*)ptr;
}

ssize_t AMediaCodecJava_dequeueInputBuffer(AMediaCodec* acodec, int64_t timeoutUs) {
//        SDLTRACE("%s(%d)", __func__, (int)timeoutUs);

    JNIEnv *env = NULL;
    if (JNI_OK != JNI_SetupThreadEnv(&env)) {
        SDLTRACE("%s: SetupThreadEnv failed", __func__);
        return -1;
    }

    AMediaCodec_Opaque *opaque = (AMediaCodec_Opaque *)acodec->opaque;
    // docs lie, getInputBuffers should be good after
    // m_codec->start() but the internal refs are not
    // setup until much later on some devices.
    //if (-1 == getInputBuffers(env, acodec)) {
    //    SDLTRACE("%s: getInputBuffers failed", __func__);
    //    return -1;
    //}

    jobject android_media_codec = opaque->android_media_codec;
    jint idx = env->CallIntMethod(android_media_codec, g_clazz.jmid_dequeueInputBuffer, (jlong)timeoutUs);
    if (JNI_CatchException(env)) {
        SDLTRACE("%s: dequeueInputBuffer failed", __func__);
        opaque->is_input_buffer_valid = false;
        return -1;
    }

    return idx;
}

amedia_status_t AMediaCodecJava_queueInputBuffer(AMediaCodec* acodec, size_t idx, off_t offset, size_t size, uint64_t time, uint32_t flags) {
//        SDLTRACE("%s: %d", __func__, (int)idx);

    JNIEnv *env = NULL;
    if (JNI_OK != JNI_SetupThreadEnv(&env)) {
        SDLTRACE("AMediaCodecJava_queueInputBuffer: SetupThreadEnv failed");
        return AMEDIA_ERROR_UNKNOWN;
    }

    AMediaCodec_Opaque *opaque = (AMediaCodec_Opaque *)acodec->opaque;
    jobject android_media_codec = opaque->android_media_codec;
    env->CallVoidMethod(android_media_codec, g_clazz.jmid_queueInputBuffer, (jint)idx, (jint)offset, (jint)size, (jlong)time, (jint)flags);
    if (JNI_CatchException(env)) {
        return AMEDIA_ERROR_UNKNOWN;
    }

    return AMEDIA_OK;
}

ssize_t AMediaCodecJava_dequeueOutputBuffer(AMediaCodec* acodec, AMediaCodecBufferInfo *info, int64_t timeoutUs) {
//        SDLTRACE("%s(%d)", __func__, (int)timeoutUs);

    JNIEnv *env = NULL;
    if (JNI_OK != JNI_SetupThreadEnv(&env)) {
        SDLTRACE("%s: SetupThreadEnv failed", __func__);
        return AMEDIACODEC__UNKNOWN_ERROR;
    }

    AMediaCodec_Opaque *opaque = (AMediaCodec_Opaque *)acodec->opaque;
    jobject android_media_codec = opaque->android_media_codec;
    if (!opaque->output_buffer_info) {
        opaque->output_buffer_info = JNI_NewObjectAsGlobalRef(env, g_clazz_BufferInfo.clazz, g_clazz_BufferInfo.jmid__ctor);
        if (JNI_CatchException(env) || !opaque->output_buffer_info) {
            SDLTRACE("%s: JNI_NewObjectAsGlobalRef failed", __func__);
            return AMEDIACODEC__UNKNOWN_ERROR;
        }
    }

    jint idx = AMEDIACODEC__UNKNOWN_ERROR;
    while (1) {
        idx = env->CallIntMethod(android_media_codec, g_clazz.jmid_dequeueOutputBuffer, opaque->output_buffer_info, (jlong)timeoutUs);
        if (JNI_CatchException(env)) {
            SDLTRACE("%s: Exception\n", __func__);
            return AMEDIACODEC__UNKNOWN_ERROR;
        }
        if (idx == AMEDIACODEC__INFO_OUTPUT_BUFFERS_CHANGED) {
            SDLTRACE("%s: INFO_OUTPUT_BUFFERS_CHANGED\n", __func__);
            JNI_DeleteGlobalRefP(env, (jobject*)&opaque->input_buffer_array);
            JNI_DeleteGlobalRefP(env, (jobject*)&opaque->output_buffer_array);
            continue;
        } else if (idx == AMEDIACODEC__INFO_OUTPUT_FORMAT_CHANGED) {
            SDLTRACE("%s: INFO_OUTPUT_FORMAT_CHANGED\n", __func__);
        } else if (idx >= 0) {
            SDLTRACE("%s: buffer ready (%d) ====================\n", __func__, idx);
            if (info) {
                info->offset              = env->GetIntField(opaque->output_buffer_info, g_clazz_BufferInfo.jfid_offset);
                info->size                = env->GetIntField(opaque->output_buffer_info, g_clazz_BufferInfo.jfid_size);
                info->presentationTimeUs  = env->GetLongField(opaque->output_buffer_info, g_clazz_BufferInfo.jfid_presentationTimeUs);
                info->flags               = env->GetIntField(opaque->output_buffer_info, g_clazz_BufferInfo.jfid_flags);
            }
        }
        break;
    }

    return idx;
}

amedia_status_t AMediaCodecJava_releaseOutputBuffer(AMediaCodec* acodec, size_t idx, bool render) {
//        SDLTRACE("%s", __func__);

    JNIEnv *env = NULL;
    if (JNI_OK != JNI_SetupThreadEnv(&env)) {
        SDLTRACE("AMediaCodecJava_releaseOutputBuffer: SetupThreadEnv failed");
        return AMEDIA_ERROR_UNKNOWN;
    }

    AMediaCodec_Opaque *opaque = (AMediaCodec_Opaque *)acodec->opaque;
    jobject android_media_codec = opaque->android_media_codec;
    env->CallVoidMethod(android_media_codec, g_clazz.jmid_releaseOutputBuffer, (jint)idx, (jboolean)render);
    if (JNI_CatchException(env)) {
        SDLTRACE("%s: releaseOutputBuffer\n", __func__);
        return AMEDIA_ERROR_UNKNOWN;
    }

    return AMEDIA_OK;
}

bool AMediaCodecJava_isInputBuffersValid(AMediaCodec* acodec) {
    AMediaCodec_Opaque *opaque = acodec->opaque;
    return opaque->is_input_buffer_valid;
}

static AMediaCodec* AMediaCodecJava_init(JNIEnv *env, jobject android_media_codec) {
    SDLTRACE("%s", __func__);

    jobject global_android_media_codec = env->NewGlobalRef(android_media_codec);
    if (JNI_CatchException(env) || !global_android_media_codec) {
        return NULL;
    }

    AMediaCodec *acodec = AMediaCodec_CreateInternal(sizeof(AMediaCodec_Opaque));
    if (!acodec) {
        JNI_DeleteGlobalRefP(env, &global_android_media_codec);
        return NULL;
    }

    AMediaCodec_Opaque *opaque = acodec->opaque;
    opaque->android_media_codec         = global_android_media_codec;

    acodec->opaque_class                = &g_amediacodec_class;
    acodec->func_delete                 = AMediaCodecJava_delete;
    // acodec->func_configure              = NULL;
    acodec->func_configure_surface      = AMediaCodecJava_configure_surface;

    acodec->func_start                  = AMediaCodecJava_start;
    acodec->func_stop                   = AMediaCodecJava_stop;
    acodec->func_flush                  = AMediaCodecJava_flush;

    acodec->func_getInputBuffer         = AMediaCodecJava_getInputBuffer;
    acodec->func_getOutputBuffer        = AMediaCodecJava_getOutputBuffer;

    acodec->func_dequeueInputBuffer     = AMediaCodecJava_dequeueInputBuffer;
    acodec->func_queueInputBuffer       = AMediaCodecJava_queueInputBuffer;

    acodec->func_dequeueOutputBuffer    = AMediaCodecJava_dequeueOutputBuffer;
    acodec->func_getOutputFormat        = AMediaCodecJava_getOutputFormat;
    acodec->func_releaseOutputBuffer    = AMediaCodecJava_releaseOutputBuffer;

    acodec->func_isInputBuffersValid    = AMediaCodecJava_isInputBuffersValid;

    AMediaCodec_increaseReference(acodec);
    return acodec;
}

AMediaCodec* AMediaCodecJava_createDecoderByType(JNIEnv *env, const char *mime_type) {
    SDLTRACE("%s", __func__);

    jstring jmime_type = env->NewStringUTF(mime_type);
    if (JNI_CatchException(env) || !jmime_type) {
        SDLTRACE("Mime type");
        return NULL;
    }

    jobject local_android_media_codec = env->CallStaticObjectMethod(g_clazz.clazz, g_clazz.jmid_createDecoderByType, jmime_type);
    JNI_DeleteLocalRefP(env, (jobject*)&jmime_type);
    if (JNI_CatchException(env) || !local_android_media_codec) {
        SDLTRACE("JNI_CatchException");
        return NULL;
    }

    AMediaCodec* acodec = AMediaCodecJava_init(env, local_android_media_codec);
    JNI_DeleteLocalRefP(env, &local_android_media_codec);
    return acodec;
}

AMediaCodec* AMediaCodecJava_createByCodecName(JNIEnv *env, const char *codec_name) {
    SDLTRACE("%s", __func__);

    jstring jcodec_name = env->NewStringUTF(codec_name);
    if (JNI_CatchException(env) || !jcodec_name) {
        return NULL;
    }

    jobject local_android_media_codec = env->CallStaticObjectMethod(g_clazz.clazz, g_clazz.jmid_createByCodecName, jcodec_name);
    JNI_DeleteLocalRefP(env, (jobject*)&jcodec_name);
    if (JNI_CatchException(env) || !local_android_media_codec) {
        return NULL;
    }

    AMediaCodec* acodec = AMediaCodecJava_init(env, local_android_media_codec);
    JNI_DeleteLocalRefP(env, &local_android_media_codec);
    return acodec;
}