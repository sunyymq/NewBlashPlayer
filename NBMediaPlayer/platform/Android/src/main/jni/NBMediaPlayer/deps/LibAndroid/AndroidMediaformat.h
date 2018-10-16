#ifndef ANDROID_MEDIAFORMAT_H
#define ANDROID_MEDIAFORMAT_H

#include <stdbool.h>
#include <jni.h>
#include <sys/types.h>
#include "AndroidMediadef.h"

typedef struct AMediaFormat_Opaque      AMediaFormat_Opaque;
typedef struct AMediaFormat             AMediaFormat;
typedef struct AMediaFormat {
    AMediaFormat_Opaque *opaque;

    amedia_status_t (*func_delete)(AMediaFormat *aformat);

    bool (*func_getInt32)(AMediaFormat* aformat, const char* name, int32_t *out);
    void (*func_setInt32)(AMediaFormat* aformat, const char* name, int32_t value);

    void (*func_setBuffer)(AMediaFormat* aformat, const char* name, void* data, size_t size);
} AMediaFormat;

amedia_status_t AMediaFormat_delete(AMediaFormat* aformat);
amedia_status_t AMediaFormat_deleteP(AMediaFormat** aformat);

bool AMediaFormat_getInt32(AMediaFormat* aformat, const char* name, int32_t *out);
void AMediaFormat_setInt32(AMediaFormat* aformat, const char* name, int32_t value);

void AMediaFormat_setBuffer(AMediaFormat* aformat, const char* name, void* data, size_t size);

#endif
