#ifndef ANDROID_JNI_H
#define ANDROID_JNI_H

#include <jni.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <NBLog.h>

#define API_1_BASE                      1   // 1.0
#define API_2_BASE_1_1                  2   // 1.1
#define API_3_CUPCAKE                   3   // 1.5
#define API_4_DONUT                     4   // 1.6
#define API_5_ECLAIR                    5   // 2.0
#define API_6_ECLAIR_0_1                6   // 2.0.1
#define API_7_ECLAIR_MR1                7   // 2.1
#define API_8_FROYO                     8   // 2.2
#define API_9_GINGERBREAD               9   // 2.3
#define API_10_GINGERBREAD_MR1          10  // 2.3.3
#define API_11_HONEYCOMB                11  // 3.0
#define API_12_HONEYCOMB_MR1            12  // 3.1
#define API_13_HONEYCOMB_MR2            13  // 3.2
#define API_14_ICE_CREAM_SANDWICH       14  // 4.0
#define API_15_ICE_CREAM_SANDWICH_MR1   15  // 4.0.3
#define API_16_JELLY_BEAN               16  // 4.1
#define API_17_JELLY_BEAN_MR1           17  // 4.2
#define API_18_JELLY_BEAN_MR2           18  // 4.3
#define API_19_KITKAT                   19  // 4.4
#define API_20_KITKAT_WATCH             20  // 4.4W
#define API_21_LOLLIPOP                 21  // 5.0

int LibAndroid_OnLoad(JNIEnv* env);
void LibAndroid_OnUnLoad(JNIEnv* env);

jint    JNI_SetupThreadEnv(JNIEnv **p_env);

void     JNI_ThrowException(JNIEnv *env, const char* msg);
jboolean JNI_RethrowException(JNIEnv *env);
jboolean JNI_CatchException(JNIEnv *env);

jobject JNI_NewObjectAsGlobalRef(JNIEnv *env, jclass clazz, jmethodID methodID, ...);

void    JNI_DeleteGlobalRefP(JNIEnv *env, jobject *obj_ptr);
void    JNI_DeleteLocalRefP(JNIEnv *env, jobject *obj_ptr);

int     Android_GetApiLevel();

#define FIND_JAVA_CLASS(env__, var__, classsign__) \
    do { \
        jclass clazz = env__->FindClass(classsign__); \
        if (JNI_CatchException(env) || !(clazz)) { \
            NBLOG_ERROR("AndroidJNI" ,"FindClass failed: %s", classsign__); \
            return -1; \
        } \
        var__ = (jclass)env__->NewGlobalRef(clazz); \
        if (JNI_CatchException(env) || !(var__)) { \
            NBLOG_ERROR("AndroidJNI" ,"FindClass::NewGlobalRef failed: %s", classsign__); \
            env__->DeleteLocalRef(clazz); \
            return -1; \
        } \
        env__->DeleteLocalRef(clazz); \
    } while(0);

#define FIND_JAVA_METHOD(env__, var__, clazz__, name__, sign__) \
    do { \
        (var__) = env__->GetMethodID((clazz__), (name__), (sign__)); \
        if (JNI_CatchException(env) || !(var__)) { \
            NBLOG_ERROR("AndroidJNI" ,"GetMethodID failed: %s", name__); \
            return -1; \
        } \
    } while(0);

#define FIND_JAVA_STATIC_METHOD(env__, var__, clazz__, name__, sign__) \
    do { \
        (var__) = env__->GetStaticMethodID((clazz__), (name__), (sign__)); \
        if (JNI_CatchException(env) || !(var__)) { \
            NBLOG_ERROR("AndroidJNI" ,"GetStaticMethodID failed: %s", name__); \
            return -1; \
        } \
    } while(0);

#define FIND_JAVA_FIELD(env__, var__, clazz__, name__, sign__) \
    do { \
        (var__) = env__->GetFieldID((clazz__), (name__), (sign__)); \
        if (JNI_CatchException(env) || !(var__)) { \
            NBLOG_ERROR("AndroidJNI" ,"GetFieldID failed: %s", name__); \
            return -1; \
        } \
    } while(0);

#define FIND_JAVA_STATIC_FIELD(env__, var__, clazz__, name__, sign__) \
    do { \
        (var__) = env__->GetStaticFieldID((clazz__), (name__), (sign__)); \
        if (JNI_CatchException(env) || !(var__)) { \
            NBLOG_ERROR("AndroidJNI" ,"GetStaticFieldID failed: %s", name__); \
            return -1; \
        } \
    } while(0);

#ifndef IJKMAX
#define IJKMAX(a, b)    ((a) > (b) ? (a) : (b))
#endif

#ifndef IJKMIN
#define IJKMIN(a, b)    ((a) < (b) ? (a) : (b))
#endif

#ifndef IJKALIGN
#define IJKALIGN(x, align) ((( x ) + (align) - 1) / (align) * (align))
#endif

#define CHECK_RET(condition__, retval__, ...) \
    if (!(condition__)) { \
        ERROR(__VA_ARGS__); \
        return (retval__); \
    }

inline static void *mallocz(size_t size)
{
    void *mem = malloc(size);
    if (!mem)
        return mem;

    memset(mem, 0, size);
    return mem;
}

inline static void freep(void **mem)
{
    if (mem && *mem) {
        free(*mem);
        *mem = NULL;
    }
}

#define JNI_CHECK_GOTO(condition__, env__, exception__, msg__, label__) \
    do { \
        if (!(condition__)) { \
            if (exception__) { \
                jniThrowException(env__, exception__, msg__); \
            } \
            goto label__; \
        } \
    }while(0)

#define JNI_CHECK_RET_VOID(condition__, env__, exception__, msg__) \
    do { \
        if (!(condition__)) { \
            if (exception__) { \
                jniThrowException(env__, exception__, msg__); \
            } \
            return; \
        } \
    }while(0)

#define JNI_CHECK_RET(condition__, env__, exception__, msg__, ret__) \
    do { \
        if (!(condition__)) { \
            if (exception__) { \
                jniThrowException(env__, exception__, msg__); \
            } \
            return ret__; \
        } \
    }while(0)

#ifndef NELEM
# define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))
#endif

typedef struct Class {
    const char *name;
} Class;

#define SDLTRACE

#endif
