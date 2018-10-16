#include "AndroidBytebuffer.h"

#include "AndroidJni.h"

typedef struct ASDK_ByteBuffer_fields_t {
    jclass clazz;

    jmethodID jmid_allocateDirect;
    jmethodID jmid_limit;
} ASDK_ByteBuffer_fields_t;
static ASDK_ByteBuffer_fields_t g_clazz;

int ASDK_ByteBuffer__loadClass(JNIEnv *env) {
    jint sdk_int = Android_GetApiLevel();
    if (sdk_int < API_16_JELLY_BEAN) {
        return 0;
    }

    FIND_JAVA_CLASS( env, g_clazz.clazz, "java/nio/ByteBuffer");

    FIND_JAVA_STATIC_METHOD(env, g_clazz.jmid_allocateDirect,   g_clazz.clazz,
        "allocateDirect",   "(I)Ljava/nio/ByteBuffer;");

    FIND_JAVA_METHOD(env, g_clazz.jmid_limit,                   g_clazz.clazz,
        "limit",            "(I)Ljava/nio/Buffer;");

    SDLTRACE("java.nio.ByteBuffer class loaded");
    return 0;
}

jlong ASDK_ByteBuffer__getDirectBufferCapacity(JNIEnv *env, jobject byte_buffer) {
    return env->GetDirectBufferCapacity(byte_buffer);
}

void *ASDK_ByteBuffer__getDirectBufferAddress(JNIEnv *env, jobject byte_buffer) {
    return env->GetDirectBufferAddress(byte_buffer);
}

void ASDK_ByteBuffer__setDataLimited(JNIEnv *env, jobject byte_buffer, void* data, size_t size) {
    jobject ret_byte_buffer = ASDK_ByteBuffer_limit(env, byte_buffer, size);
    JNI_DeleteLocalRefP(env, &ret_byte_buffer);
    if (JNI_RethrowException(env)) {
        return;
    }

    uint8_t *buffer = (uint8_t*)ASDK_ByteBuffer__getDirectBufferAddress(env, byte_buffer);
    if (JNI_RethrowException(env) || !buffer) {
        return;
    }

    memcpy(buffer, data, size);
}

jobject ASDK_ByteBuffer_allocateDirect(JNIEnv *env, jint capacity) {
    SDLTRACE("ASDK_ByteBuffer_allocateDirect");

    jobject byte_buffer = env->CallStaticObjectMethod(g_clazz.clazz, g_clazz.jmid_allocateDirect, capacity);
    if (JNI_RethrowException(env) || !byte_buffer) {
        return NULL;
    }

    return byte_buffer;
}

jobject ASDK_ByteBuffer_allocateDirectAsGlobalRef(JNIEnv *env, jint capacity) {
    jobject local_byte_buffer = ASDK_ByteBuffer_allocateDirect(env, capacity);
    if (JNI_RethrowException(env) || !local_byte_buffer) {
         return NULL;
    }

    jobject global_byte_buffer = env->NewGlobalRef(local_byte_buffer);
    JNI_DeleteLocalRefP(env, &local_byte_buffer);
    return global_byte_buffer;
}

jobject ASDK_ByteBuffer_limit(JNIEnv *env, jobject byte_buffer, jint newLimit) {
    SDLTRACE("ASDK_ByteBuffer_limit");

    return env->CallObjectMethod(byte_buffer, g_clazz.jmid_limit, newLimit);
}