#ifndef ANDROID_MEDIACODEC_INTERNAL_H
#define ANDROID_MEDIACODEC_INTERNAL_H

#include "AndroidMediacodec.h"

inline static AMediaCodec *AMediaCodec_CreateInternal(size_t opaque_size) {
    AMediaCodec *acodec = (AMediaCodec*) mallocz(sizeof(AMediaCodec));
    if (!acodec)
        return NULL;

    acodec->opaque = (AMediaCodec_Opaque*)mallocz(opaque_size);
    if (!acodec->opaque) {
        free(acodec);
        return NULL;
    }

    return acodec;
}

inline static void AMediaCodec_FreeInternal(AMediaCodec *acodec) {
    if (!acodec)
        return;

    free(acodec->opaque);
    memset(acodec, 0, sizeof(AMediaCodec));
    free(acodec);
}

#endif

