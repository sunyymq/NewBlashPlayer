#include "AndroidArraylist.h"

#include "AndroidJni.h"

typedef struct ASDK_ArrayList_fields_t {
    jclass clazz;

    jmethodID jmid_init;
    jmethodID jmid_add;
} ASDK_ArrayList_fields_t;
static ASDK_ArrayList_fields_t g_clazz;

int ASDK_ArrayList__loadClass(JNIEnv *env) {
    FIND_JAVA_CLASS( env, g_clazz.clazz, "java/util/ArrayList");

    FIND_JAVA_METHOD(env, g_clazz.jmid_init,    g_clazz.clazz,
        "<init>",                   "()V");
    FIND_JAVA_METHOD(env, g_clazz.jmid_add,     g_clazz.clazz,
        "add",                      "(Ljava/lang/Object;)Z");

    SDLTRACE("java.util/ArrayList class loaded");
    return 0;
}

jobject ASDK_ArrayList__init(JNIEnv *env) {
    jobject local_ref = env->NewObject(g_clazz.clazz, g_clazz.jmid_init);
    if (JNI_RethrowException(env) || !local_ref) {
        return NULL;
    }

    return local_ref;
}

jboolean ASDK_ArrayList__add(JNIEnv *env, jobject thiz, jobject elem) {
    jboolean ret = env->CallBooleanMethod(thiz, g_clazz.jmid_add, elem);
    if (JNI_RethrowException(env)) {
        ret = JNI_FALSE;
        goto fail;
    }

    ret = JNI_TRUE;
fail:
    return ret;
}