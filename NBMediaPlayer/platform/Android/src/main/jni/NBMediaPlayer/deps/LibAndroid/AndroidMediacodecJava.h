#ifndef ANDROID_MEDIACODEC_JAVA_H
#define ANDROID_MEDIACODEC_JAVA_H

#include "AndroidMediacodec.h"

typedef struct ASDK_MediaCodec ASDK_MediaCodec;

int AMediaCodecJava__loadClass(JNIEnv *env);

AMediaCodec  *AMediaCodecJava_createByCodecName(JNIEnv *env, const char *codec_name);
AMediaCodec  *AMediaCodecJava_createDecoderByType(JNIEnv *env, const char *mime_type);
jobject           AMediaCodecJava_getObject(JNIEnv *env, const AMediaCodec *thiz);

#endif