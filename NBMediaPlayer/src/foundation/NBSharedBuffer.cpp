//
// Created by parallels on 9/8/18.
//

#include "NBSharedBuffer.h"
#include "NBAtomic.h"
#include <stdlib.h>
#include <string.h>

NBSharedBuffer* NBSharedBuffer::alloc(size_t size)
{
    NBSharedBuffer* sb = static_cast<NBSharedBuffer *>(malloc(sizeof(NBSharedBuffer) + size));
    if (sb) {
        sb->mRefs = 1;
        sb->mSize = size;
    }
    return sb;
}


ssize_t NBSharedBuffer::dealloc(const NBSharedBuffer* released)
{
    if (released->mRefs != 0) return -1; // XXX: invalid operation
    free(const_cast<NBSharedBuffer*>(released));
    return 0;
}

NBSharedBuffer* NBSharedBuffer::edit() const
{
    if (onlyOwner()) {
        return const_cast<NBSharedBuffer*>(this);
    }
    NBSharedBuffer* sb = alloc(mSize);
    if (sb) {
        memcpy(sb->data(), data(), size());
        release();
    }
    return sb;
}

NBSharedBuffer* NBSharedBuffer::editResize(size_t newSize) const
{
    if (onlyOwner()) {
        NBSharedBuffer* buf = const_cast<NBSharedBuffer*>(this);
        if (buf->mSize == newSize) return buf;
        buf = (NBSharedBuffer*)realloc(buf, sizeof(NBSharedBuffer) + newSize);
        if (buf != NULL) {
            buf->mSize = newSize;
            return buf;
        }
    }
    NBSharedBuffer* sb = alloc(newSize);
    if (sb) {
        const size_t mySize = mSize;
        memcpy(sb->data(), data(), newSize < mySize ? newSize : mySize);
        release();
    }
    return sb;
}

NBSharedBuffer* NBSharedBuffer::attemptEdit() const
{
    if (onlyOwner()) {
        return const_cast<NBSharedBuffer*>(this);
    }
    return 0;
}

NBSharedBuffer* NBSharedBuffer::reset(size_t new_size) const
{
    // cheap-o-reset.
    NBSharedBuffer* sb = alloc(new_size);
    if (sb) {
        release();
    }
    return sb;
}

void NBSharedBuffer::acquire() const {
    nb_atomic_inc(&mRefs);
}

int32_t NBSharedBuffer::release(uint32_t flags) const
{
    int32_t prev = 1;
    if (onlyOwner() || ((prev = nb_atomic_dec(&mRefs)) == 1)) {
        mRefs = 0;
        if ((flags & eKeepStorage) == 0) {
            free(const_cast<NBSharedBuffer*>(this));
        }
    }
    return prev;
}