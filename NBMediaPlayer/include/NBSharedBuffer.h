//
// Created by parallels on 9/8/18.
//

#ifndef NBSHAREDBUFFER_H
#define NBSHAREDBUFFER_H

#include <stdint.h>
#include <sys/types.h>

class NBSharedBuffer
{
public:

    /* flags to use with release() */
    enum {
        eKeepStorage = 0x00000001
    };

    /*! allocate a buffer of size 'size' and acquire() it.
     *  call release() to free it.
     */
    static          NBSharedBuffer*           alloc(size_t size);

    /*! free the memory associated with the SharedBuffer.
     * Fails if there are any users associated with this SharedBuffer.
     * In other words, the buffer must have been release by all its
     * users.
     */
    static          ssize_t                 dealloc(const NBSharedBuffer* released);

    //! access the data for read
    inline          const void*             data() const;

    //! access the data for read/write
    inline          void*                   data();

    //! get size of the buffer
    inline          size_t                  size() const;

    //! get back a SharedBuffer object from its data
    static  inline  NBSharedBuffer*           bufferFromData(void* data);

    //! get back a SharedBuffer object from its data
    static  inline  const NBSharedBuffer*     bufferFromData(const void* data);

    //! get the size of a SharedBuffer object from its data
    static  inline  size_t                  sizeFromData(const void* data);

    //! edit the buffer (get a writtable, or non-const, version of it)
    NBSharedBuffer*           edit() const;

    //! edit the buffer, resizing if needed
    NBSharedBuffer*           editResize(size_t size) const;

    //! like edit() but fails if a copy is required
    NBSharedBuffer*           attemptEdit() const;

    //! resize and edit the buffer, loose it's content.
    NBSharedBuffer*           reset(size_t size) const;

    //! acquire/release a reference on this buffer
    void                    acquire() const;

    /*! release a reference on this buffer, with the option of not
     * freeing the memory associated with it if it was the last reference
     * returns the previous reference count
     */
                    int32_t                 release(uint32_t flags = 0) const;

    //! returns wether or not we're the only owner
    inline          bool                    onlyOwner() const;


private:
    inline NBSharedBuffer() { }
    inline ~NBSharedBuffer() { }
    NBSharedBuffer(const NBSharedBuffer&);
    NBSharedBuffer& operator = (const NBSharedBuffer&);

    // 16 bytes. must be sized to preserve correct alignment.
    mutable int32_t        mRefs;
            size_t         mSize;
            uint32_t       mReserved[2];
};

// ---------------------------------------------------------------------------

const void* NBSharedBuffer::data() const {
    return this + 1;
}

void* NBSharedBuffer::data() {
    return this + 1;
}

size_t NBSharedBuffer::size() const {
    return mSize;
}

NBSharedBuffer* NBSharedBuffer::bufferFromData(void* data) {
    return data ? static_cast<NBSharedBuffer *>(data)-1 : 0;
}

const NBSharedBuffer* NBSharedBuffer::bufferFromData(const void* data) {
    return data ? static_cast<const NBSharedBuffer *>(data)-1 : 0;
}

size_t NBSharedBuffer::sizeFromData(const void* data) {
    return data ? bufferFromData(data)->mSize : 0;
}

bool NBSharedBuffer::onlyOwner() const {
    return (mRefs == 1);
}

#endif //NBSHAREDBUFFER_H
