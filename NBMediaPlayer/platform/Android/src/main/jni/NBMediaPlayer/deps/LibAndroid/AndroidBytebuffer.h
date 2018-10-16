#ifndef ANDROID_BYTEBUFFER_H
#define ANDROID_BYTEBUFFER_H

#include <jni.h>
#include <stdint.h>

int     ASDK_ByteBuffer__loadClass(JNIEnv *env);
jlong   ASDK_ByteBuffer__getDirectBufferCapacity(JNIEnv *env, jobject byte_buffer);
void   *ASDK_ByteBuffer__getDirectBufferAddress(JNIEnv *env, jobject byte_buffer);
void    ASDK_ByteBuffer__setDataLimited(JNIEnv *env, jobject byte_buffer, void* data, size_t size);

jobject ASDK_ByteBuffer_allocateDirect(JNIEnv *env, jint capacity);
jobject ASDK_ByteBuffer_allocateDirectAsGlobalRef(JNIEnv *env, jint capacity);
jobject ASDK_ByteBuffer_limit(JNIEnv *env, jobject byte_buffer, jint newLimit);

#endif
