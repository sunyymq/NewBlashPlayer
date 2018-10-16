#ifndef ANDROID_SURFACE_H
#define ANDROID_SURFACE_H

#include <jni.h>
#include <stdint.h>

int ASDK_Surface__loadClass(JNIEnv *env);

jobject  ASDK_Surface__init(JNIEnv *env, jobject surfaceTexture);

jobject ASDK_Surface__initWithGlobalRef(JNIEnv* env, jobject surfaceTexture);

#endif