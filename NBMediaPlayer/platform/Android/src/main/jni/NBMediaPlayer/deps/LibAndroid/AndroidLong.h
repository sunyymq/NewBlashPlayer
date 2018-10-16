#ifndef ANDROID_LONG_H
#define ANDROID_LONG_H

#include <jni.h>
#include <stdint.h>

int ASDK_Long__loadClass(JNIEnv *env);

jobject ASDK_Long_valueOf(JNIEnv *env, int64_t value);

#endif