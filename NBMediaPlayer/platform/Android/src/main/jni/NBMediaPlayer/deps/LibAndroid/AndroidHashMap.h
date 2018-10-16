#ifndef ANDROID_HASH_MAP_H
#define ANDROID_HASH_MAP_H

#include <jni.h>
#include <stdint.h>

int ASDK_HashMap__loadClass(JNIEnv *env);

jobject  ASDK_HashMap__init(JNIEnv *env);

jobject ASDK_HashMap__put(JNIEnv *env, jobject thiz, jobject key, jobject elem);

#endif