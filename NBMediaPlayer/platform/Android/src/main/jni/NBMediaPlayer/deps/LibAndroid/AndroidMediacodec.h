#ifndef ANDROID_MEDIACODEC_H
#define ANDROID_MEDIACODEC_H

#include <stdbool.h>
#include <jni.h>
#include <sys/types.h>
#include "AndroidMediadef.h"

struct Class;

typedef struct AMediaCodecBufferInfo {
    int32_t offset;
    int32_t size;
    int64_t presentationTimeUs;
    uint32_t flags;
} AMediaCodecBufferInfo;

typedef struct AMediaFormat             AMediaFormat;
typedef struct AMediaCrypto             AMediaCrypto;

typedef struct AMediaCodec_Opaque       AMediaCodec_Opaque;
typedef struct AMediaCodec              AMediaCodec;
typedef struct AMediaCodec {
    volatile int  ref_count;

    Class              *opaque_class;
    AMediaCodec_Opaque *opaque;
    bool                    is_configured;
    bool                    is_started;

    amedia_status_t (*func_delete)(AMediaCodec *acodec);

    // amedia_status_t (*func_configure)(
    //     AMediaCodec* acodec,
    //     const AMediaFormat* aformat,
    //     ANativeWindow* surface,
    //     AMediaCrypto *crypto,
    //     uint32_t flags);
    amedia_status_t (*func_configure_surface)(
        JNIEnv*env,
        AMediaCodec* acodec,
        const AMediaFormat* aformat,
        jobject android_surface,
        AMediaCrypto *crypto,
        uint32_t flags);

    amedia_status_t     (*func_start)(AMediaCodec* acodec);
    amedia_status_t     (*func_stop)(AMediaCodec* acodec);
    amedia_status_t     (*func_flush)(AMediaCodec* acodec);

    uint8_t*                (*func_getInputBuffer)(AMediaCodec* acodec, size_t idx, size_t *out_size);
    uint8_t*                (*func_getOutputBuffer)(AMediaCodec* acodec, size_t idx, size_t *out_size);

    ssize_t                 (*func_dequeueInputBuffer)(AMediaCodec* acodec, int64_t timeoutUs);
    amedia_status_t     (*func_queueInputBuffer)(AMediaCodec* acodec, size_t idx, off_t offset, size_t size, uint64_t time, uint32_t flags);

    ssize_t                 (*func_dequeueOutputBuffer)(AMediaCodec* acodec, AMediaCodecBufferInfo *info, int64_t timeoutUs);
    AMediaFormat*       (*func_getOutputFormat)(AMediaCodec* acodec);
    amedia_status_t     (*func_releaseOutputBuffer)(AMediaCodec* acodec, size_t idx, bool render);

    bool                    (*func_isInputBuffersValid)(AMediaCodec* acodec);
} AMediaCodec;

typedef struct CodecAndroid_BufferInfo {
    int32_t offset;
    int32_t size;
    int64_t presentationTimeUs;
    uint32_t flags;
} CodecAndroid_BufferInfo;

// use AMediaCodec_decreaseReference instead
// amedia_status_t     AMediaCodec_delete(AMediaCodec* acodec);
// amedia_status_t     AMediaCodec_deleteP(AMediaCodec** acodec);

// amedia_status_t     AMediaCodec_configure(
//     AMediaCodec* acodec,
//     const AMediaFormat* aformat,
//     ANativeWindow* surface,
//     AMediaCrypto *crypto,
//     uint32_t flags);

amedia_status_t     AMediaCodec_configure_surface(
    JNIEnv*env,
    AMediaCodec* acodec,
    const AMediaFormat* aformat,
    jobject android_surface,
    AMediaCrypto *crypto,
    uint32_t flags);

void                    AMediaCodec_increaseReference(AMediaCodec *acodec);
void                    AMediaCodec_decreaseReference(AMediaCodec *acodec);
void                    AMediaCodec_decreaseReferenceP(AMediaCodec **acodec);

bool                    AMediaCodec_isConfigured(AMediaCodec *acodec);
bool                    AMediaCodec_isStarted(AMediaCodec *acodec);

amedia_status_t     AMediaCodec_start(AMediaCodec* acodec);
amedia_status_t     AMediaCodec_stop(AMediaCodec* acodec);
amedia_status_t     AMediaCodec_flush(AMediaCodec* acodec);

uint8_t*                AMediaCodec_getInputBuffer(AMediaCodec* acodec, size_t idx, size_t *out_size);
uint8_t*                AMediaCodec_getOutputBuffer(AMediaCodec* acodec, size_t idx, size_t *out_size);

ssize_t                 AMediaCodec_dequeueInputBuffer(AMediaCodec* acodec, int64_t timeoutUs);
amedia_status_t     AMediaCodec_queueInputBuffer(AMediaCodec* acodec, size_t idx, off_t offset, size_t size, uint64_t time, uint32_t flags);

ssize_t                 AMediaCodec_dequeueOutputBuffer(AMediaCodec* acodec, AMediaCodecBufferInfo *info, int64_t timeoutUs);
AMediaFormat*       AMediaCodec_getOutputFormat(AMediaCodec* acodec);
amedia_status_t     AMediaCodec_releaseOutputBuffer(AMediaCodec* acodec, size_t idx, bool render);

bool                    AMediaCodec_isInputBuffersValid(AMediaCodec* acodec);

#endif
