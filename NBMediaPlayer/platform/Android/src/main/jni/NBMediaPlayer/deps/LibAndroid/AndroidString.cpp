#include "AndroidString.h"

#include "AndroidJni.h"

typedef struct ASDK_String_fields_t {
    jclass clazz;

    jmethodID jmid_init;
} ASDK_String_fields_t;

static ASDK_String_fields_t g_clazz;

int ASDK_String__loadClass(JNIEnv *env) {

    FIND_JAVA_CLASS( env, g_clazz.clazz, "java/lang/String");


    FIND_JAVA_METHOD(env, g_clazz.jmid_init,    g_clazz.clazz,
        "<init>",                   "([B)V");

    SDLTRACE("java.lang.String class loaded");
    return 0;
}

jobject  ASDK_String__init(JNIEnv *env, const char *value) {
    jbyteArray keybytes = env->NewByteArray(strlen(value));

    env->SetByteArrayRegion(keybytes, 0, strlen(value), (jbyte*)value);

    jobject local_ref = env->NewObject(g_clazz.clazz, g_clazz.jmid_init, keybytes);

    if (JNI_CatchException(env) || !local_ref) {
        return NULL;
    }

    return local_ref;
}