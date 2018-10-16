#ifndef ANDROID_ARRAYLIST_H
#define ANDROID_ARRAYLIST_H

#include <jni.h>

int ASDK_ArrayList__loadClass(JNIEnv *env);

jobject  ASDK_ArrayList__init(JNIEnv *env);
jboolean ASDK_ArrayList__add(JNIEnv *env, jobject thiz, jobject elem);

#endif
