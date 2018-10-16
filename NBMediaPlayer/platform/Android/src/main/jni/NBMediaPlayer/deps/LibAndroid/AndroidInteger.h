#ifndef ANDROID_INTEGER_H
#define ANDROID_INTEGER_H

#include <jni.h>
#include <stdint.h>

int ASDK_Integer__loadClass(JNIEnv *env);

jobject ASDK_Integer_valueOf(JNIEnv *env, int32_t value);

#endif