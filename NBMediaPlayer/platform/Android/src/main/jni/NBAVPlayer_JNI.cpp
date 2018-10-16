//
// Created by liu enbao on 23/09/2018.
//

#include <jni.h>
#include <NBMediaPlayer.h>
#include <NBRendererInfo.h>
#include <NBLog.h>
#include <JNIUtils.h>
#include <AndroidJni.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LOG_TAG "NBAVPlayer_JNI"

struct fields_t {
    jfieldID context;
    jfieldID jniAudioTrack;
    jmethodID post_event;
};
static fields_t fields;

static const char *const kClassPathName = "com/ccsu/nbmediaplayer/NBAVPlayer";

class MediaPlayerListener : public INBMediaPlayerLister {
public:
    MediaPlayerListener(jobject thiz, jobject weak_thiz) {
        JNIEnv *env = getJNIEnv();
        // Hold onto the MediaPlayer class for use in calling the static method
        // that posts events to the application thread.
        jclass clazz = env->GetObjectClass(thiz);
        if (clazz != NULL) {
            mClass = (jclass) env->NewGlobalRef(clazz);

            // We use a weak reference so the MediaPlayer object can be garbage collected.
            // The reference is only used as a proxy for callbacks.
            mObject = env->NewGlobalRef(weak_thiz);
        } else {
            jniThrowException(env, "java/lang/Exception", kClassPathName);
        }
    }

    ~MediaPlayerListener() {
        // remove global references
        JNIEnv *env = getJNIEnv();
        env->DeleteGlobalRef(mObject);
        env->DeleteGlobalRef(mClass);
    }

public:
    virtual void sendEvent(int msg, int ext1=0, int ext2=0) {
        JNIEnv *env = getJNIEnv();
        NBLOG_INFO(LOG_TAG, "The mClass : %p mObject : %p ields.post_event : %p env : %p", mClass, mObject, fields.post_event, env);
        env->CallStaticVoidMethod(mClass, fields.post_event, mObject, msg, ext1, ext2, 0);
    }

private:
    jclass mClass;    // Reference to MediaPlayer class
    jobject mObject;  // Weak ref to MediaPlayer Java object to call on
};

typedef struct NBNativeContext {
    NBMediaPlayer* mp;
    MediaPlayerListener* listener;
    jobject surface;
    NBRendererTarget rendererTarget;
} NBNativeContext;

static NBNativeContext *getMediaPlayer(JNIEnv *env, jobject thiz) {
    return (NBNativeContext *)env->GetLongField(thiz, fields.context);
}

static void setMediaPlayer(JNIEnv* env, jobject thiz, NBNativeContext* player) {
    NBNativeContext *old = (NBNativeContext*)env->GetIntField(thiz, fields.context);
    if (old != NULL) {
        if (old->mp != NULL) {
            delete old->mp;
        }

        if (old->listener != NULL) {
            delete old->listener;
        }

        if (old->surface != NULL) {
            env->DeleteGlobalRef((jobject)old->surface);
        }

        delete old;
    }
    env->SetLongField(thiz, fields.context, (long) player);
}

// If exception is NULL and opStatus is not OK, this method sends an error
// event to the client application; otherwise, if exception is not NULL and
// opStatus is not OK, this method throws the given exception to the client
// application.
static void process_media_player_call(JNIEnv *env, jobject thiz, nb_status_t opStatus, const char* exception, const char *message) {
    if (exception != NULL) {
        if (opStatus == (nb_status_t) INVALID_OPERATION) {
            jniThrowException(env, "java/lang/IllegalStateException", NULL);
        } else if (opStatus != (nb_status_t) OK) {
            if (strlen(message) > 230) {
                // if the message is too long, don't bother displaying the status code
                jniThrowException(env, exception, message);
            } else {
                char msg[256];
                // append the status code to the message
                sprintf(msg, "%s: status=0x%X", message, opStatus);
                jniThrowException(env, exception, msg);
            }
        }
    } else {
        if (opStatus != (nb_status_t) OK) {
            NBLOG_ERROR(LOG_TAG, "call failed. op=%d msg=%s", opStatus, message);

            NBNativeContext *context = getMediaPlayer(env, thiz);
            if (context != NULL) {
                MediaPlayerListener *listener = context->listener;
                if (listener != NULL) {
                    listener->sendEvent(MEDIA_ERROR, opStatus, 0);
                }
            }
        }
    }
}

JNIEXPORT void JNICALL NBAVPlayer_setAudioTrack(JNIEnv *env, jobject thiz, jobject jParamAudioTrack) {

}

JNIEXPORT void JNICALL NBAVPlayer_setSurface(JNIEnv *env, jobject thiz, jobject jParamSurface) {
    NBNativeContext* nativeContext = getMediaPlayer(env, thiz);
    if (nativeContext != NULL) {
        nativeContext->surface = env->NewGlobalRef(jParamSurface);
        nativeContext->rendererTarget.params = nativeContext->surface;
        nativeContext->mp->setVideoOutput(&nativeContext->rendererTarget);
    }
}

JNIEXPORT void JNICALL NBAVPlayer_start(JNIEnv *env, jobject thiz) {
    NBNativeContext* nativeContext = getMediaPlayer(env, thiz);
    if (nativeContext != NULL) {
        NBNativeContext* nativeContext = getMediaPlayer(env, thiz);
        if (nativeContext != NULL) {
            nativeContext->mp->play();
        }
    }
}

JNIEXPORT void JNICALL NBAVPlayer_stop(JNIEnv *env, jobject thiz) {
    NBNativeContext* nativeContext = getMediaPlayer(env, thiz);
    if (nativeContext != NULL) {
        nativeContext->mp->reset();
    }
}

JNIEXPORT void JNICALL NBAVPlayer_pause(JNIEnv *env, jobject thiz) {
    NBNativeContext* nativeContext = getMediaPlayer(env, thiz);
    if (nativeContext != NULL) {
        nativeContext->mp->pause();
    }
}

JNIEXPORT void JNICALL NBAVPlayer_release(JNIEnv *env, jobject thiz) {
    NBNativeContext* nativeContext = getMediaPlayer(env, thiz);
    if (nativeContext != NULL) {
        // // this prevents native callbacks after the object is released
        nativeContext->mp->setListener(NULL);

        setMediaPlayer(env, thiz, NULL);
    }
}

JNIEXPORT void JNICALL NBAVPlayer_reset(JNIEnv *env, jobject thiz) {
    NBNativeContext* nativeContext = getMediaPlayer(env, thiz);
    if (nativeContext != NULL) {
        nativeContext->mp->reset();
    }
}

JNIEXPORT void JNICALL NBAVPlayer_native_init(JNIEnv *env, jobject thiz) {
    JavaVM* vm = NULL;
    env->GetJavaVM(&vm);

    setJavaVM(vm);

    jclass clazz;
    clazz = env->FindClass(kClassPathName);
    if (clazz == NULL) {
        jniThrowException(env, "java/lang/RuntimeException", "Can't find android/media/MediaPlayer");
        return ;
    }

    fields.context = env->GetFieldID(clazz, "mNativeContext", "J");
    if (fields.context == NULL) {
        jniThrowException(env, "java/lang/RuntimeException", "Can't find MediaPlayer.mNativeContext");
        return ;
    }

    fields.jniAudioTrack = env->GetFieldID(clazz, "mAudioTrack", "Landroid/media/AudioTrack;");
    if (fields.jniAudioTrack == NULL) {
        jniThrowException(env, "java/lang/RuntimeException", "Can't find MediaPlayer.mAudioTrack");
        return ;
    }

    fields.post_event = env->GetStaticMethodID(clazz,
                                               "postEventFromNative",
                                               "(Ljava/lang/Object;IIILjava/lang/Object;)V");
    if (fields.post_event == NULL) {
        jniThrowException(env, "java/lang/RuntimeException", "Can't find FFMpegMediaPlayer.postEventFromNative");
        return ;
    }
}

JNIEXPORT void JNICALL NBAVPlayer_native_setup(JNIEnv *env, jobject thiz, jobject weak_this, jobject context) {
    NBNativeContext* nativeContext = new NBNativeContext();

    if (nativeContext == NULL) {
        jniThrowException(env, "java/lang/RuntimeException", "Out of memory");
        return ;
    }
    memset(nativeContext, 0, sizeof(NBNativeContext));

    nativeContext->listener = new MediaPlayerListener(thiz, weak_this);

    nativeContext->mp = new NBMediaPlayer();
    nativeContext->mp->setListener(nativeContext->listener);

    // Stow our new C++ MediaPlayer in an opaque field in the Java object.
    setMediaPlayer(env, thiz, nativeContext);
}

JNIEXPORT void JNICALL NBAVPlayer_native_finalize(JNIEnv *env, jobject thiz) {
    NBAVPlayer_release(env, thiz);
}

JNIEXPORT void JNICALL NBAVPlayer_setDataSourceAndHeaders(JNIEnv *env, jobject thiz, jstring jPath, jobjectArray jKeys, jobjectArray jValues) {
    NBNativeContext *nativeContext = getMediaPlayer(env, thiz);
    if (nativeContext != NULL) {
        const char *pathStr = env->GetStringUTFChars(jPath, NULL);
        NBLOG_INFO(LOG_TAG, "setDataSource: path %s", pathStr);

        NBMap<NBString, NBString> params;
        if (jKeys != NULL && jValues != NULL) {
            int keysLength = env->GetArrayLength(jKeys);
            int valuesLength = env->GetArrayLength(jValues);
            if (keysLength != valuesLength) {
                NBLOG_ERROR(LOG_TAG, "The keys and the values must be equal");

                env->ReleaseStringUTFChars(jPath, pathStr);

                process_media_player_call(env, thiz, -1, "java/io/IOException", "setDataSource failed.");
                return ;
            }

            for (int i = 0; i < keysLength; ++i) {
                jobject key = env->GetObjectArrayElement(jKeys, i);
                jobject value = env->GetObjectArrayElement(jValues, i);

                const char* szKey = env->GetStringUTFChars((jstring)key, 0);
                const char* szValue = env->GetStringUTFChars((jstring)value, 0);

                NBLOG_INFO(LOG_TAG, "The params key : %s value : %s", szKey, szValue);

//                params[szKey] = szValue;

                env->ReleaseStringUTFChars((jstring)key, szKey);
                env->ReleaseStringUTFChars((jstring)value, szValue);
            }
        }

        nb_status_t opStatus = nativeContext->mp->setDataSource(pathStr, &params);

        // Make sure that local ref is released before a potential exception
        env->ReleaseStringUTFChars(jPath, pathStr);

        process_media_player_call(env, thiz, opStatus, "java/io/IOException", "setDataSource failed.");
    } else {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
    }
}

JNIEXPORT void JNICALL NBAVPlayer_setDataSourceWithId(JNIEnv *env, jobject thiz, jobject jParamFileDescriptor, jlong jParamLong1, jlong jParamLong2) {

}

JNIEXPORT void JNICALL NBAVPlayer_prepare(JNIEnv *env, jobject thiz) {

}

JNIEXPORT void JNICALL NBAVPlayer_prepareAsync(JNIEnv *env, jobject thiz) {
    NBNativeContext *nativeContext = getMediaPlayer(env, thiz);
    if (nativeContext != NULL) {
        NBLOG_INFO(LOG_TAG, "prepareAsync called");
        process_media_player_call(env, thiz, nativeContext->mp->prepareAsync(), "java/io/IOException", "PrepareAsync failed.");
    } else {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
    }
}

JNIEXPORT jint JNICALL NBAVPlayer_getVideoWidth(JNIEnv *env, jobject thiz) {
    NBNativeContext *nativeContext = getMediaPlayer(env, thiz);
    if (nativeContext != NULL) {
        int32_t width = -1;

        nb_status_t opStatus = nativeContext->mp->getVideoWidth(&width);

        process_media_player_call(env, thiz, opStatus, "java/io/IOException", "setDataSource failed.");

        return width;
    } else {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
    }
    return -1;
}

JNIEXPORT jint JNICALL NBAVPlayer_getVideoHeight(JNIEnv *env, jobject thiz) {
    NBNativeContext *nativeContext = getMediaPlayer(env, thiz);
    if (nativeContext != NULL) {
        int32_t height = -1;

        nb_status_t opStatus = nativeContext->mp->getVideoHeight(&height);

        process_media_player_call(env, thiz, opStatus, "java/io/IOException", "setDataSource failed.");

        return height;
    } else {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
    }
    return -1;
}

JNIEXPORT jboolean JNICALL NBAVPlayer_isPlaying(JNIEnv *env, jobject thiz) {
    NBNativeContext *nativeContext = getMediaPlayer(env, thiz);
    if (nativeContext != NULL) {
        int32_t height = -1;

        bool isPlaying = nativeContext->mp->isPlaying();

        return isPlaying;
    } else {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
    }
    return false;
}

JNIEXPORT void JNICALL NBAVPlayer_setLooping(JNIEnv *env, jobject thiz, jboolean jLooping) {

}

JNIEXPORT void JNICALL NBAVPlayer_seekTo(JNIEnv *env, jobject thiz, jint jParamInt) {
    NBNativeContext *nativeContext = getMediaPlayer(env, thiz);
    if (nativeContext != NULL) {
        int32_t height = -1;

        nb_status_t opStatus = nativeContext->mp->seekTo(jParamInt);

        process_media_player_call(env, thiz, opStatus, "java/io/IOException", "setDataSource failed.");
    } else {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
    }
}

JNIEXPORT jint JNICALL NBAVPlayer_getCurrentPosition(JNIEnv *env, jobject thiz) {
    NBNativeContext *nativeContext = getMediaPlayer(env, thiz);
    if (nativeContext != NULL) {
        int64_t position = -1;

        nb_status_t opStatus = nativeContext->mp->getCurrentPosition(&position);

        process_media_player_call(env, thiz, opStatus, "java/io/IOException", "setDataSource failed.");

        return position;
    } else {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
    }
    return 0;
}

JNIEXPORT jint JNICALL NBAVPlayer_getDuration(JNIEnv *env, jobject thiz) {
    NBNativeContext *nativeContext = getMediaPlayer(env, thiz);
    if (nativeContext != NULL) {
        int64_t duration = -1;

        nb_status_t opStatus = nativeContext->mp->getDuration(&duration);

        process_media_player_call(env, thiz, opStatus, "java/io/IOException", "setDataSource failed.");

        return duration;
    } else {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
    }
    return 0;
}

JNIEXPORT jint JNICALL NBAVPlayer_getVideoFrameRate(JNIEnv *env, jobject thiz) {

}

JNIEXPORT jobject JNICALL NBAVPlayer_getMetadata(JNIEnv *env, jobject thiz, jint jTrack) {

}

JNIEXPORT void JNICALL NBAVPlayer_setNextMediaPlayer(JNIEnv *env, jobject thiz, jobject jParamMediaPlayer) {

}

JNIEXPORT void JNICALL NBAVPlayer_setScreenMetrics(JNIEnv *env, jobject thiz, jint jScreenWidth, jint jScreenHeight) {

}

JNIEXPORT void JNICALL NBAVPlayer_setVolume(JNIEnv *env, jobject thiz, jint jVolume) {

}

static JNINativeMethod gMethods[] = {
    {
        "_setDataSource",
        "(Ljava/lang/String;[Ljava/lang/String;[Ljava/lang/String;)V",
        (void *)NBAVPlayer_setDataSourceAndHeaders
    },
    {"setDataSource",                   "(Ljava/io/FileDescriptor;JJ)V",                  (void *)NBAVPlayer_setDataSourceWithId},
    {"_setAudioTrack",                  "(Landroid/media/AudioTrack;)V",                  (void *)NBAVPlayer_setAudioTrack},
    {"_setVideoSurface",                "(Landroid/view/Surface;)V",                      (void *)NBAVPlayer_setSurface},
    {"_start",                          "()V",                                            (void *)NBAVPlayer_start},
    {"_stop",                           "()V",                                            (void *)NBAVPlayer_stop},
    {"_pause",                          "()V",                                            (void *)NBAVPlayer_pause},
    {"_release",                        "()V",                                            (void *)NBAVPlayer_release},
    {"_reset",                          "()V",                                            (void *)NBAVPlayer_reset},
    {"native_init",                     "()V",                                            (void *)NBAVPlayer_native_init},
    {"native_setup",                    "(Ljava/lang/Object;Landroid/content/Context;)V", (void *)NBAVPlayer_native_setup},
    {"native_finalize",                 "()V",                                            (void *)NBAVPlayer_native_finalize},
    {"prepare",                         "()V",                                            (void *)NBAVPlayer_prepare},
    {"prepareAsync",                    "()V",                                            (void *)NBAVPlayer_prepareAsync},
    {"getVideoWidth",                   "()I",                                            (void *)NBAVPlayer_getVideoWidth},
    {"getVideoHeight",                  "()I",                                            (void *)NBAVPlayer_getVideoHeight},
    {"isPlaying",                       "()Z",                                            (void *)NBAVPlayer_isPlaying},
    {"setLooping",                      "(Z)V",                                           (void *)NBAVPlayer_setLooping},
    {"seekTo",                          "(I)V",                                           (void *)NBAVPlayer_seekTo},
    {"getCurrentPosition",              "()I",                                            (void *)NBAVPlayer_getCurrentPosition},
    {"getDuration",                     "()I",                                            (void *)NBAVPlayer_getDuration},
    {"getMetadata",                     "(I)Ljava/util/Map;",                             (void *)NBAVPlayer_getMetadata},
    {"setNextMediaPlayer",              "(Lcom/ccsu/nbmediaplayer/NBAVPlayer;)V",  (void *)NBAVPlayer_setNextMediaPlayer},
    {"setScreenMetrics",                "(II)V",                                          (void *)NBAVPlayer_setScreenMetrics},
    {"getVideoFrameRate",               "()I",                                            (void *)NBAVPlayer_getVideoFrameRate},
};

NBLOG_DEFINE();

JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    //Init the log system
    NBLOG_INIT(NULL, NBLOG_MODE_CONSOLE);

    NBLOG_INFO(LOG_TAG, "JNI_OnLoad");

    jint result = JNI_ERR;

    setJavaVM(vm);

    JNIEnv *env = getJNIEnv();
    if (env == NULL) {
        return JNI_ERR;
    }

    LibAndroid_OnLoad(env);

    result = jniRegisterNativeMethods(env, kClassPathName, gMethods, sizeof(gMethods) / sizeof(gMethods[0]));
    if (result != 0) {
        return result;
    }

    return JNI_VERSION_1_4;
}

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM* vm, void* reserved) {
    //deinit the log system
    NBLOG_DEINIT();

    JNIEnv *env = getJNIEnv();
    if (env == NULL) {
        return ;
    }

    jniUnRegisterNativeMethods(env, kClassPathName);

    LibAndroid_OnUnLoad(env);
}

#ifdef __cplusplus
}
#endif