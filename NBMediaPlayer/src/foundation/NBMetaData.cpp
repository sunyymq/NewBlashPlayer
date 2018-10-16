//
// Created by parallels on 9/9/18.
//

#include "NBMetaData.h"

#include <stdlib.h>
#include <string.h>

NBMetaData::NBMetaData() {
}

NBMetaData::NBMetaData(const NBMetaData &from)
        : mItems(from.mItems) {
}

NBMetaData::~NBMetaData() {
    clear();
}

void NBMetaData::clear() {
    mItems.clear();
}

bool NBMetaData::remove(uint32_t key) {
    return mItems.erase(key) != 0;
}

bool NBMetaData::setCString(uint32_t key, const char *value) {
    return setData(key, TYPE_C_STRING, value, strlen(value) + 1);
}

bool NBMetaData::setInt32(uint32_t key, int32_t value) {
    return setData(key, TYPE_INT32, &value, sizeof(value));
}

bool NBMetaData::setInt64(uint32_t key, int64_t value) {
    return setData(key, TYPE_INT64, &value, sizeof(value));
}

bool NBMetaData::setFloat(uint32_t key, float value) {
    return setData(key, TYPE_FLOAT, &value, sizeof(value));
}

bool NBMetaData::setPointer(uint32_t key, void *value) {
    return setData(key, TYPE_POINTER, &value, sizeof(value));
}

bool NBMetaData::findCString(uint32_t key, const char **value) {
    uint32_t type;
    const void *data;
    size_t size;
    if (!findData(key, &type, &data, &size) || type != TYPE_C_STRING) {
        return false;
    }

    *value = (const char *)data;

    return true;
}

bool NBMetaData::findInt32(uint32_t key, int32_t *value) {
    uint32_t type;
    const void *data;
    size_t size;
    if (!findData(key, &type, &data, &size) || type != TYPE_INT32) {
        return false;
    }

    *value = *(int32_t *)data;

    return true;
}

bool NBMetaData::findInt64(uint32_t key, int64_t *value) {
    uint32_t type;
    const void *data;
    size_t size;
    if (!findData(key, &type, &data, &size) || type != TYPE_INT64) {
        return false;
    }

    *value = *(int64_t *)data;

    return true;
}

bool NBMetaData::findFloat(uint32_t key, float *value) {
    uint32_t type;
    const void *data;
    size_t size;
    if (!findData(key, &type, &data, &size) || type != TYPE_FLOAT) {
        return false;
    }

    *value = *(float *)data;

    return true;
}

bool NBMetaData::findPointer(uint32_t key, void **value) {
    uint32_t type;
    const void *data;
    size_t size;
    if (!findData(key, &type, &data, &size) || type != TYPE_POINTER) {
        return false;
    }

    *value = *(void **)data;

    return true;
}

bool NBMetaData::setData(
        uint32_t key, uint32_t type, const void *data, size_t size) {
    bool overwrote_existing = true;

    NBMap<uint32_t, typed_data>::const_iterator i = mItems.find(key);
    if (i == mItems.end()) {
        typed_data item;
        mItems[key] = item;

        overwrote_existing = false;
    }

    typed_data &item = mItems[key];

    item.setData(type, data, size);

    return overwrote_existing;
}

bool NBMetaData::findData(uint32_t key, uint32_t *type,
                        const void **data, size_t *size) const {
    NBMap<uint32_t, typed_data>::const_iterator i = mItems.find(key);

    if (i == mItems.end()) {
        return false;
    }

    const typed_data &item = i->second;

    item.getData(type, data, size);

    return true;
}

NBMetaData::typed_data::typed_data()
        : mType(0),
          mSize(0) {
}

NBMetaData::typed_data::~typed_data() {
    clear();
}

NBMetaData::typed_data::typed_data(const typed_data &from)
        : mType(from.mType),
          mSize(0) {
    allocateStorage(from.mSize);
    memcpy(storage(), from.storage(), mSize);
}

NBMetaData::typed_data &NBMetaData::typed_data::operator=(
        const NBMetaData::typed_data &from) {
    if (this != &from) {
        clear();
        mType = from.mType;
        allocateStorage(from.mSize);
        memcpy(storage(), from.storage(), mSize);
    }

    return *this;
}

void NBMetaData::typed_data::clear() {
    freeStorage();

    mType = 0;
}

bool NBMetaData::findRect(
        uint32_t key,
        int32_t *left, int32_t *top,
        int32_t *right, int32_t *bottom) {
    uint32_t type;
    const void *data;
    size_t size;
    if (!findData(key, &type, &data, &size) || type != TYPE_RECT) {
        return false;
    }

    const Rect *r = (const Rect *)data;
    *left = r->mLeft;
    *top = r->mTop;
    *right = r->mRight;
    *bottom = r->mBottom;

    return true;
}

void NBMetaData::typed_data::setData(
        uint32_t type, const void *data, size_t size) {
    clear();

    mType = type;
    allocateStorage(size);
    memcpy(storage(), data, size);
}

void NBMetaData::typed_data::getData(
        uint32_t *type, const void **data, size_t *size) const {
    *type = mType;
    *size = mSize;
    *data = storage();
}

void NBMetaData::typed_data::allocateStorage(size_t size) {
    mSize = size;

    if (usesReservoir()) {
        return;
    }

    u.ext_data = malloc(mSize);
}

void NBMetaData::typed_data::freeStorage() {
    if (!usesReservoir()) {
        if (u.ext_data) {
            free(u.ext_data);
        }
    }

    mSize = 0;
}