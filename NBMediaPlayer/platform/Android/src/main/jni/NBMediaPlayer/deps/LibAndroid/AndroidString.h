#ifndef ANDROID_STRING_H
#define ANDROID_STRING_H

#include <jni.h>
#include <stdint.h>

int ASDK_String__loadClass(JNIEnv *env);

jobject  ASDK_String__init(JNIEnv *env, const char *value);

#endif