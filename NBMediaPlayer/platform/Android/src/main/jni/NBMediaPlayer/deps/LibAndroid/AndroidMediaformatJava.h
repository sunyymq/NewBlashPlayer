#ifndef ANDROID_MEDIAFORMAT_JAVA_H
#define ANDROID_MEDIAFORMAT_JAVA_H

#include "AndroidMediaformat.h"

int AMediaFormatJava__loadClass(JNIEnv *env);

AMediaFormat *AMediaFormatJava_new(JNIEnv *env);
AMediaFormat *AMediaFormatJava_init(JNIEnv *env, jobject android_format);
AMediaFormat *AMediaFormatJava_createVideoFormat(JNIEnv *env, const char *mime, int width, int height);
jobject      AMediaFormatJava_getObject(JNIEnv *env, const AMediaFormat *thiz);

#endif