#include "AndroidBuild.h"

#include "AndroidJni.h"

typedef struct ASDK_Build_VERSION_fields_t {
    jclass clazz;

    jfieldID jfid_SDK_INT;
} ASDK_Build_VERSION_fields_t;
static ASDK_Build_VERSION_fields_t g_clazz_VERSION;

int ASDK_Build__loadClass(JNIEnv *env) {
    FIND_JAVA_CLASS( env, g_clazz_VERSION.clazz, "android/os/Build$VERSION");

    FIND_JAVA_STATIC_FIELD(env, g_clazz_VERSION.jfid_SDK_INT,   g_clazz_VERSION.clazz,
        "SDK_INT",   "I");

    SDLTRACE("android.os.Build$VERSION$ class loaded");

    return 0;
}

int ASDK_Build_VERSION__SDK_INT(JNIEnv *env) {
    jint sdk_int = env->GetStaticIntField(g_clazz_VERSION.clazz, g_clazz_VERSION.jfid_SDK_INT);
    if (JNI_RethrowException(env)) {
        return 0;
    }

    return sdk_int;
}