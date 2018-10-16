#ifndef ANDROID_BUNDLE_H
#define ANDROID_BUNDLE_H

#include <jni.h>

int ASDK_Bundle__loadClass(JNIEnv *env);

jobject ASDK_Bundle__init(JNIEnv *env);
void    ASDK_Bundle__putString_c(JNIEnv *env, jobject thiz, const char *key, const char *value);
void    ASDK_Bundle__putParcelableArrayList_c(JNIEnv *env, jobject thiz, const char *key, jobject value);

#endif
