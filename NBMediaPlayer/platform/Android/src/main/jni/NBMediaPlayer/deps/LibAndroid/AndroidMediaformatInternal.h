#ifndef ANDROID_MEDIAFORMAT_INTERNAL_H
#define ANDROID_MEDIAFORMAT_INTERNAL_H

#include "AndroidMediaformat.h"

inline static AMediaFormat *AMediaFormat_CreateInternal(size_t opaque_size) {
    AMediaFormat *aformat = (AMediaFormat*) mallocz(sizeof(AMediaFormat));
    if (!aformat)
        return NULL;

    aformat->opaque = (AMediaFormat_Opaque*)mallocz(opaque_size);
    if (!aformat->opaque) {
        free(aformat);
        return NULL;
    }

    return aformat;
}

inline static void AMediaFormat_FreeInternal(AMediaFormat *aformat) {
    if (!aformat)
        return;

    free(aformat->opaque);
    memset(aformat, 0, sizeof(AMediaFormat));
    free(aformat);
}

#endif

