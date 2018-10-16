#include "AndroidMediacodec.h"
#include <assert.h>
#include "AndroidJni.h"

// FIXME: release AMediaCodec
amedia_status_t AMediaCodec_delete(AMediaCodec* acodec) {
    if (!acodec)
        return AMEDIA_OK;

    assert(acodec->func_delete);
    return acodec->func_delete(acodec);
}

amedia_status_t AMediaCodec_deleteP(AMediaCodec** acodec) {
    if (!acodec)
        return AMEDIA_OK;
    return AMediaCodec_delete(*acodec);
}

amedia_status_t AMediaCodec_configure_surface(
    JNIEnv*env,
    AMediaCodec* acodec,
    const AMediaFormat* aformat,
    jobject android_surface,
    AMediaCrypto *crypto,
    uint32_t flags) {
    assert(acodec->func_configure_surface);
    amedia_status_t ret = acodec->func_configure_surface(env, acodec, aformat, android_surface, crypto, flags);
    acodec->is_configured = true;
    acodec->is_started    = false;
    return ret;
}

void AMediaCodec_increaseReference(AMediaCodec *acodec) {
    assert(acodec);
    __sync_fetch_and_add(&acodec->ref_count, 1);
}

void AMediaCodec_decreaseReference(AMediaCodec *acodec) {
    if (!acodec)
        return;

    int ref_count = __sync_sub_and_fetch(&acodec->ref_count, 1);
    if (ref_count == 0) {
        SDLTRACE("%s(): ref=0\n", __func__);
        if (AMediaCodec_isStarted(acodec)) {
            AMediaCodec_stop(acodec);
        }
        AMediaCodec_delete(acodec);
    }
}

void AMediaCodec_decreaseReferenceP(AMediaCodec **acodec) {
    if (!acodec)
        return;

    AMediaCodec_decreaseReference(*acodec);
    *acodec = NULL;
}

bool AMediaCodec_isConfigured(AMediaCodec *acodec) {
    return acodec->is_configured;
}

bool AMediaCodec_isStarted(AMediaCodec *acodec) {
    return acodec->is_started;
}

amedia_status_t AMediaCodec_start(AMediaCodec* acodec) {
    assert(acodec->func_start);
    amedia_status_t amc_ret = acodec->func_start(acodec);
    if (amc_ret == AMEDIA_OK) {
        acodec->is_started = true;
    }
    return amc_ret;
}

amedia_status_t AMediaCodec_stop(AMediaCodec* acodec) {
    assert(acodec->func_stop);
    acodec->is_started = false;
    return acodec->func_stop(acodec);
}

amedia_status_t AMediaCodec_flush(AMediaCodec* acodec) {
    assert(acodec->func_flush);
    return acodec->func_flush(acodec);
}

uint8_t* AMediaCodec_getInputBuffer(AMediaCodec* acodec, size_t idx, size_t *out_size) {
    assert(acodec->func_getInputBuffer);
    return acodec->func_getInputBuffer(acodec, idx, out_size);
}

uint8_t* AMediaCodec_getOutputBuffer(AMediaCodec* acodec, size_t idx, size_t *out_size) {
    assert(acodec->func_getOutputBuffer);
    return acodec->func_getOutputBuffer(acodec, idx, out_size);
}

ssize_t AMediaCodec_dequeueInputBuffer(AMediaCodec* acodec, int64_t timeoutUs) {
    assert(acodec->func_dequeueInputBuffer);
    return acodec->func_dequeueInputBuffer(acodec, timeoutUs);
}

amedia_status_t AMediaCodec_queueInputBuffer(AMediaCodec* acodec, size_t idx, off_t offset, size_t size, uint64_t time, uint32_t flags) {
    assert(acodec->func_queueInputBuffer);
    return acodec->func_queueInputBuffer(acodec, idx, offset, size, time, flags);
}

ssize_t AMediaCodec_dequeueOutputBuffer(AMediaCodec* acodec, AMediaCodecBufferInfo *info, int64_t timeoutUs) {
    assert(acodec->func_dequeueOutputBuffer);
    return acodec->func_dequeueOutputBuffer(acodec, info, timeoutUs);
}

AMediaFormat* AMediaCodec_getOutputFormat(AMediaCodec* acodec) {
    assert(acodec->func_getOutputFormat);
    return acodec->func_getOutputFormat(acodec);
}

amedia_status_t AMediaCodec_releaseOutputBuffer(AMediaCodec* acodec, size_t idx, bool render) {
    assert(acodec->func_releaseOutputBuffer);
    return acodec->func_releaseOutputBuffer(acodec, idx, render);
}

bool AMediaCodec_isInputBuffersValid(AMediaCodec* acodec) {
    assert(acodec->func_isInputBuffersValid);
    return acodec->func_isInputBuffersValid(acodec);
}
