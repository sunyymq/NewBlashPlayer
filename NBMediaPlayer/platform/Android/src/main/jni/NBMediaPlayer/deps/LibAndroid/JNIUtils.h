#ifndef JNI_UTILS_H
#define JNI_UTILS_H

#include <stdlib.h>
#include <string>
#include <jni.h>

int jniThrowException(JNIEnv* env, const char* className, const char* msg);

void setJavaVM(JavaVM* jvm);

JavaVM* getJavaVM();

JNIEnv* getJNIEnv();

int jniRegisterNativeMethods(JNIEnv* env,
                             const char* className,
                             const JNINativeMethod* gMethods,
                             int numMethods);

void jniUnRegisterNativeMethods(JNIEnv* env,
                             const char* className);

#endif /* _JNI_UTILS_H_ */