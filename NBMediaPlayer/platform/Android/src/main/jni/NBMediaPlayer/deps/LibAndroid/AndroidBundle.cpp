#include "AndroidBundle.h"

#include <assert.h>
#include "AndroidJni.h"

typedef struct ASDK_Bundle_fields_t {
    jclass clazz;

    jmethodID jmid_init;
    jmethodID jmid_putString;
    jmethodID jmid_putParcelableArrayList;
} ASDK_Bundle_fields_t;
static ASDK_Bundle_fields_t g_clazz;

int ASDK_Bundle__loadClass(JNIEnv *env) {
    FIND_JAVA_CLASS( env, g_clazz.clazz, "android/os/Bundle");

    FIND_JAVA_METHOD(env, g_clazz.jmid_init,                      g_clazz.clazz,
        "<init>",                   "()V");
    FIND_JAVA_METHOD(env, g_clazz.jmid_putString,                 g_clazz.clazz,
        "putString",                "(Ljava/lang/String;Ljava/lang/String;)V");
    FIND_JAVA_METHOD(env, g_clazz.jmid_putParcelableArrayList,    g_clazz.clazz,
        "putParcelableArrayList",   "(Ljava/lang/String;Ljava/util/ArrayList;)V");

    SDLTRACE("android.os.Bundle class loaded");
    return 0;
}

jobject ASDK_Bundle__init(JNIEnv *env) {
    jobject local_ref = env->NewObject(g_clazz.clazz, g_clazz.jmid_init);
    if (JNI_RethrowException(env) || !local_ref) {
        return NULL;
    }

    return local_ref;
}

void ASDK_Bundle__putString_c(JNIEnv *env, jobject thiz, const char *key, const char *value) {
    assert(key);
    jstring jkey = NULL;
    jstring jvalue = NULL;

    jkey = env->NewStringUTF(key);
    if (JNI_RethrowException(env) || !jkey) {
        goto fail;
    }

    if (value) {
        jvalue = env->NewStringUTF(value);
        if (JNI_RethrowException(env) || !jvalue) {
            goto fail;
        }
    }

    env->CallVoidMethod(thiz, g_clazz.jmid_putString, jkey, jvalue);
    if (JNI_RethrowException(env)) {
        goto fail;
    }

fail:
    JNI_DeleteLocalRefP(env, (jobject*)&jkey);
    JNI_DeleteLocalRefP(env, (jobject*)&jvalue);
    return;
}

void ASDK_Bundle__putParcelableArrayList_c(JNIEnv *env, jobject thiz, const char *key, jobject value) {
    assert(key);
    jstring jkey = NULL;

    jkey = env->NewStringUTF(key);
    if (JNI_RethrowException(env) || !jkey) {
        goto fail;
    }

    env->CallVoidMethod(thiz, g_clazz.jmid_putParcelableArrayList, jkey, value);
    if (JNI_RethrowException(env)) {
        goto fail;
    }

fail:
    JNI_DeleteLocalRefP(env, (jobject*)&jkey);
    return;
}