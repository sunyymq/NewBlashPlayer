#include "AndroidHashMap.h"

#include "AndroidJni.h"

typedef struct ASDK_HashMap_fields_t {
    jclass clazz;

    jmethodID jmid_init;
    jmethodID jmid_put;
} ASDK_HashMap_fields_t;

static ASDK_HashMap_fields_t g_clazz;

int ASDK_HashMap__loadClass(JNIEnv *env) {
    FIND_JAVA_CLASS( env, g_clazz.clazz, "java/util/HashMap");

    FIND_JAVA_METHOD(env, g_clazz.jmid_init,    g_clazz.clazz,
        "<init>",                   "()V");
    FIND_JAVA_METHOD(env, g_clazz.jmid_put,     g_clazz.clazz,
        "put",                      "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");

    SDLTRACE("java.lang.HashMap class loaded");
    return 0;
}

jobject  ASDK_HashMap__init(JNIEnv *env) {
    jobject local_ref = env->NewObject(g_clazz.clazz, g_clazz.jmid_init);
    if (JNI_RethrowException(env) || !local_ref) {
        return NULL;
    }

    return local_ref;
}

jobject ASDK_HashMap__put(JNIEnv *env, jobject thiz, jobject key, jobject elem) {
    return env->CallObjectMethod(thiz, g_clazz.jmid_put, key, elem);
}