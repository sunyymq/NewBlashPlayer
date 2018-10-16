#ifndef ANDROID_BUILD_H
#define ANDROID_BUILD_H

#include <jni.h>

int ASDK_Build__loadClass(JNIEnv *env);

int ASDK_Build_VERSION__SDK_INT(JNIEnv *env);

#endif
