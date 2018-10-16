#include "AndroidLong.h"

#include "AndroidJni.h"

typedef struct ASDK_Long_fields_t {
    jclass clazz;

    jmethodID jmid_valueOf;
} ASDK_Long_fields_t;
static ASDK_Long_fields_t g_clazz;

int ASDK_Long__loadClass(JNIEnv *env) {
    FIND_JAVA_CLASS( env, g_clazz.clazz, "java/lang/Long");

    FIND_JAVA_STATIC_METHOD(env, g_clazz.jmid_valueOf,   g_clazz.clazz,
        "valueOf",   "(J)Ljava/lang/Long;");

    SDLTRACE("java.lang.Long class loaded");

    return 0;
}

jobject ASDK_Long_valueOf(JNIEnv *env, int64_t value) {

    jobject local_ref = env->CallStaticObjectMethod(g_clazz.clazz, g_clazz.jmid_valueOf, value);

    if (JNI_RethrowException(env)) {
        return 0;
    }

    return local_ref;
}