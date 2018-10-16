#include "AndroidInteger.h"

#include "AndroidJni.h"

typedef struct ASDK_Integer_fields_t {
    jclass clazz;

    jmethodID jmid_valueOf;
} ASDK_Integer_fields_t;
static ASDK_Integer_fields_t g_clazz;

int ASDK_Integer__loadClass(JNIEnv *env) {

    FIND_JAVA_CLASS( env, g_clazz.clazz, "java/lang/Integer");

    FIND_JAVA_STATIC_METHOD(env, g_clazz.jmid_valueOf,   g_clazz.clazz,
        "valueOf",   "(I)Ljava/lang/Integer;");

    SDLTRACE("java.lang.Integer class loaded");
    return 0;
}

jobject ASDK_Integer_valueOf(JNIEnv *env, int32_t value) {
    jobject local_ref = env->CallStaticObjectMethod(g_clazz.clazz, g_clazz.jmid_valueOf, value);

    if (JNI_RethrowException(env)) {
        return NULL;
    }

    return local_ref;
}