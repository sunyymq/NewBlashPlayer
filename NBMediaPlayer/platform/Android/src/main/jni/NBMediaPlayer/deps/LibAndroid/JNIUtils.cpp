#include "JNIUtils.h"

static JavaVM *sVm = NULL;

/*
 * Throw an exception with the specified class and an optional message.
 */
int jniThrowException(JNIEnv *env, const char *className, const char *msg) {
    if (env->ExceptionCheck()) {
        jthrowable exception = env->ExceptionOccurred();
        env->ExceptionClear();

        if (exception != NULL) {
            env->DeleteLocalRef(exception);
        }
    }

    jclass exceptionClass = env->FindClass(className);
    if (exceptionClass == NULL) {
        /* ClassNotFoundException now pending */
        goto fail;
    }

    if (env->ThrowNew(exceptionClass, msg) != JNI_OK) {
        /* an exception, most likely OOM, will now be pending */
        goto fail;
    }

    return 0;
fail:
    if (exceptionClass)
        env->DeleteLocalRef(exceptionClass);
    return -1;
}

JNIEnv *getJNIEnv() {
    JNIEnv *env = NULL;

    if (sVm->GetEnv((void **) &env, JNI_VERSION_1_4) != JNI_OK) {

    }

    return env;
}

void setJavaVM(JavaVM* jvm) {
    sVm = jvm;
}

JavaVM* getJavaVM() {
    return sVm;
}

/*
 * Register native JNI-callable methods.
 *
 * "className" looks like "java/lang/String".
 */
int jniRegisterNativeMethods(JNIEnv *env,
                             const char *className,
                             const JNINativeMethod *gMethods,
                             int numMethods) {
    int result = -1;

    jclass clazz;

    clazz = env->FindClass(className);
    if (clazz != NULL) {
        if (env->RegisterNatives(clazz, gMethods, numMethods) >= 0) {
            result = 0;
        } else {
        }
    } else {
    }

    return result;
}

void jniUnRegisterNativeMethods(JNIEnv* env,
                               const char* className) {
    jclass clazz;

    clazz = env->FindClass(className);

    if (clazz != NULL) {
        env->UnregisterNatives(clazz);
    }

}