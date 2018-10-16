#include "AndroidMediaformat.h"
#include <assert.h>

// FIXME: release AMediaFormat
amedia_status_t AMediaFormat_delete(AMediaFormat* aformat) {
    if (!aformat)
        return AMEDIA_OK;
    assert(aformat->func_delete);
    return aformat->func_delete(aformat);
}

amedia_status_t AMediaFormat_deleteP(AMediaFormat** aformat) {
    if (!aformat)
        return AMEDIA_OK;
    amedia_status_t amc_ret = AMediaFormat_delete(*aformat);
    aformat = NULL;
    return amc_ret;
}

bool AMediaFormat_getInt32(AMediaFormat* aformat, const char* name, int32_t *out) {
    assert(aformat->func_getInt32);
    return aformat->func_getInt32(aformat, name, out);
}

void AMediaFormat_setInt32(AMediaFormat* aformat, const char* name, int32_t value) {
    assert(aformat->func_setInt32);
    aformat->func_setInt32(aformat, name, value);
}

void AMediaFormat_setBuffer(AMediaFormat* aformat, const char* name, void* data, size_t size) {
    assert(aformat->func_setInt32);
    aformat->func_setBuffer(aformat, name, data, size);
}