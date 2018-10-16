#ifndef ANDROID_SURFACE_TEXTURE_H
#define ANDROID_SURFACE_TEXTURE_H

#include <jni.h>
#include <stdint.h>

int ASDK_SurfaceTexture__loadClass(JNIEnv *env);

jobject  ASDK_SurfaceTexture__init(JNIEnv *env, int texName);

jobject ASDK_SurfaceTexture__initWithGlobalRef(JNIEnv* env, int texName);

void ASDK_SurfaceTexture__setOnFrameAvailableListener(JNIEnv* env, jobject surfaceTexture, jobject listener);

bool ASDK_SurfaceTexture__updateTexImage(JNIEnv* env, jobject surfaceTexture);

bool ASDK_SurfaceTexture__releaseTexImage(JNIEnv* env, jobject surfaceTexture);

bool ASDK_SurfaceTexture__release(JNIEnv* env, jobject surfaceTexture);

#endif