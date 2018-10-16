//
// Created by parallels on 9/8/18.
//

#include <NBString.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define OS_PATH_SEPARATOR '/'
#define RES_PATH_SEPARATOR '/'

static NBSharedBuffer* gEmptyStringBuf = NULL;
static char* gEmptyString = NULL;

void initialize_NBString()
{
    // HACK: This dummy dependency forces linking libutils Static.cpp,
    // which is needed to initialize NBString/String16 classes.
    // These variables are named for Darwin, but are needed elsewhere too,
    // including static linking on any platform.
//    gDarwinIsReallyAnnoying = gDarwinCantLoadAllObjects;

    NBSharedBuffer* buf = NBSharedBuffer::alloc(1);
    char* str = (char*)buf->data();
    *str = 0;
    gEmptyStringBuf = buf;
    gEmptyString = str;
}

void deinitialize_NBString()
{
    NBSharedBuffer::bufferFromData(gEmptyString)->release();
    gEmptyStringBuf = NULL;
    gEmptyString = NULL;
}

static inline char* getEmptyString()
{
    gEmptyStringBuf->acquire();
    return gEmptyString;
}

static char* allocFromUTF8(const char* in, size_t len)
{
    if (len > 0) {
        NBSharedBuffer* buf = NBSharedBuffer::alloc(len+1);
//        ALOG_ASSERT(buf, "Unable to allocate shared buffer");
        if (buf) {
            char* str = (char*)buf->data();
            memcpy(str, in, len);
            str[len] = 0;
            return str;
        }
        return NULL;
    }

    return getEmptyString();
}

NBString::NBString()
        : mString(getEmptyString())
{
}

NBString::NBString(StaticLinkage)
        : mString(0)
{
    // this constructor is used when we can't rely on the static-initializers
    // having run. In this case we always allocate an empty string. It's less
    // efficient than using getEmptyString(), but we assume it's uncommon.

    char* data = static_cast<char*>(
            NBSharedBuffer::alloc(sizeof(char))->data());
    data[0] = 0;
    mString = data;
}

NBString::NBString(const NBString& o)
        : mString(o.mString)
{
    NBSharedBuffer::bufferFromData(mString)->acquire();
}

NBString::NBString(const char* o)
        : mString(allocFromUTF8(o, strlen(o)))
{
    if (mString == NULL) {
        mString = getEmptyString();
    }
}

NBString::NBString(const char* o, size_t len)
        : mString(allocFromUTF8(o, len))
{
    if (mString == NULL) {
        mString = getEmptyString();
    }
}

//NBString::NBString(const String16& o)
//        : mString(allocFromUTF16(o.string(), o.size()))
//{
//}
//
//NBString::NBString(const char16_t* o)
//        : mString(allocFromUTF16(o, strlen16(o)))
//{
//}
//
//NBString::NBString(const char16_t* o, size_t len)
//        : mString(allocFromUTF16(o, len))
//{
//}
//
//NBString::NBString(const char32_t* o)
//        : mString(allocFromUTF32(o, strlen32(o)))
//{
//}
//
//NBString::NBString(const char32_t* o, size_t len)
//        : mString(allocFromUTF32(o, len))
//{
//}

NBString::~NBString()
{
    NBSharedBuffer::bufferFromData(mString)->release();
}

NBString NBString::format(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    NBString result(formatV(fmt, args));

    va_end(args);
    return result;
}

NBString NBString::formatV(const char* fmt, va_list args)
{
    NBString result;
    result.appendFormatV(fmt, args);
    return result;
}

void NBString::clear() {
    NBSharedBuffer::bufferFromData(mString)->release();
    mString = getEmptyString();
}

void NBString::setTo(const NBString& other)
{
    NBSharedBuffer::bufferFromData(other.mString)->acquire();
    NBSharedBuffer::bufferFromData(mString)->release();
    mString = other.mString;
}

nb_status_t NBString::setTo(const char* other)
{
    const char *newString = allocFromUTF8(other, strlen(other));
    NBSharedBuffer::bufferFromData(mString)->release();
    mString = newString;
    if (mString) return NO_ERROR;

    mString = getEmptyString();
    return NO_MEMORY;
}

nb_status_t NBString::setTo(const char* other, size_t len)
{
    const char *newString = allocFromUTF8(other, len);
    NBSharedBuffer::bufferFromData(mString)->release();
    mString = newString;
    if (mString) return NO_ERROR;

    mString = getEmptyString();
    return NO_MEMORY;
}

//nb_status_t NBString::setTo(const char16_t* other, size_t len)
//{
//    const char *newString = allocFromUTF16(other, len);
//    SharedBuffer::bufferFromData(mString)->release();
//    mString = newString;
//    if (mString) return NO_ERROR;
//
//    mString = getEmptyString();
//    return NO_MEMORY;
//}
//
//nb_status_t NBString::setTo(const char32_t* other, size_t len)
//{
//    const char *newString = allocFromUTF32(other, len);
//    SharedBuffer::bufferFromData(mString)->release();
//    mString = newString;
//    if (mString) return NO_ERROR;
//
//    mString = getEmptyString();
//    return NO_MEMORY;
//}

nb_status_t NBString::append(const NBString& other)
{
    const size_t otherLen = other.bytes();
    if (bytes() == 0) {
        setTo(other);
        return NO_ERROR;
    } else if (otherLen == 0) {
        return NO_ERROR;
    }

    return real_append(other.string(), otherLen);
}

nb_status_t NBString::append(const char* other)
{
    return append(other, strlen(other));
}

nb_status_t NBString::append(const char* other, size_t otherLen)
{
    if (bytes() == 0) {
        return setTo(other, otherLen);
    } else if (otherLen == 0) {
        return NO_ERROR;
    }

    return real_append(other, otherLen);
}

nb_status_t NBString::appendFormat(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    nb_status_t result = appendFormatV(fmt, args);

    va_end(args);
    return result;
}

nb_status_t NBString::appendFormatV(const char* fmt, va_list args)
{
    int result = NO_ERROR;
    int n = vsnprintf(NULL, 0, fmt, args);
    if (n != 0) {
        size_t oldLength = length();
        char* buf = lockBuffer(oldLength + n);
        if (buf) {
            vsnprintf(buf + oldLength, n + 1, fmt, args);
        } else {
            result = NO_MEMORY;
        }
    }
    return result;
}

nb_status_t NBString::real_append(const char* other, size_t otherLen)
{
    const size_t myLen = bytes();

    NBSharedBuffer* buf = NBSharedBuffer::bufferFromData(mString)
            ->editResize(myLen+otherLen+1);
    if (buf) {
        char* str = (char*)buf->data();
        mString = str;
        str += myLen;
        memcpy(str, other, otherLen);
        str[otherLen] = '\0';
        return NO_ERROR;
    }
    return NO_MEMORY;
}

char* NBString::lockBuffer(size_t size)
{
    NBSharedBuffer* buf = NBSharedBuffer::bufferFromData(mString)
            ->editResize(size+1);
    if (buf) {
        char* str = (char*)buf->data();
        mString = str;
        return str;
    }
    return NULL;
}

void NBString::unlockBuffer()
{
    unlockBuffer(strlen(mString));
}

nb_status_t NBString::unlockBuffer(size_t size)
{
    if (size != this->size()) {
        NBSharedBuffer* buf = NBSharedBuffer::bufferFromData(mString)
                ->editResize(size+1);
        if (! buf) {
            return NO_MEMORY;
        }

        char* str = (char*)buf->data();
        str[size] = 0;
        mString = str;
    }

    return NO_ERROR;
}

ssize_t NBString::find(const char* other, size_t start) const
{
    size_t len = size();
    if (start >= len) {
        return -1;
    }
    const char* s = mString+start;
    const char* p = strstr(s, other);
    return p ? p-mString : -1;
}

void NBString::toLower()
{
    toLower(0, size());
}

void NBString::toLower(size_t start, size_t length)
{
    const size_t len = size();
    if (start >= len) {
        return;
    }
    if (start+length > len) {
        length = len-start;
    }
    char* buf = lockBuffer(len);
    buf += start;
    while (length > 0) {
        *buf = tolower(*buf);
        buf++;
        length--;
    }
    unlockBuffer(len);
}

void NBString::toUpper()
{
    toUpper(0, size());
}

void NBString::toUpper(size_t start, size_t length)
{
    const size_t len = size();
    if (start >= len) {
        return;
    }
    if (start+length > len) {
        length = len-start;
    }
    char* buf = lockBuffer(len);
    buf += start;
    while (length > 0) {
        *buf = toupper(*buf);
        buf++;
        length--;
    }
    unlockBuffer(len);
}

//size_t NBString::getUtf32Length() const
//{
//    return utf8_to_utf32_length(mString, length());
//}
//
//int32_t NBString::getUtf32At(size_t index, size_t *next_index) const
//{
//    return utf32_from_utf8_at(mString, length(), index, next_index);
//}
//
//void NBString::getUtf32(char32_t* dst) const
//{
//    utf8_to_utf32(mString, length(), dst);
//}

// ---------------------------------------------------------------------------
// Path functions

void NBString::setPathName(const char* name)
{
    setPathName(name, strlen(name));
}

void NBString::setPathName(const char* name, size_t len)
{
    char* buf = lockBuffer(len);

    memcpy(buf, name, len);

    // remove trailing path separator, if present
    if (len > 0 && buf[len-1] == OS_PATH_SEPARATOR)
        len--;

    buf[len] = '\0';

    unlockBuffer(len);
}

NBString NBString::getPathLeaf(void) const
{
    const char* cp;
    const char*const buf = mString;

    cp = strrchr(buf, OS_PATH_SEPARATOR);
    if (cp == NULL)
        return NBString(*this);
    else
        return NBString(cp+1);
}

NBString NBString::getPathDir(void) const
{
    const char* cp;
    const char*const str = mString;

    cp = strrchr(str, OS_PATH_SEPARATOR);
    if (cp == NULL)
        return NBString("");
    else
        return NBString(str, cp - str);
}

NBString NBString::walkPath(NBString* outRemains) const
{
    const char* cp;
    const char*const str = mString;
    const char* buf = str;

    cp = strchr(buf, OS_PATH_SEPARATOR);
    if (cp == buf) {
        // don't include a leading '/'.
        buf = buf+1;
        cp = strchr(buf, OS_PATH_SEPARATOR);
    }

    if (cp == NULL) {
        NBString res = buf != str ? NBString(buf) : *this;
        if (outRemains) *outRemains = NBString("");
        return res;
    }

    NBString res(buf, cp-buf);
    if (outRemains) *outRemains = NBString(cp+1);
    return res;
}

/*
 * Helper function for finding the start of an extension in a pathname.
 *
 * Returns a pointer inside mString, or NULL if no extension was found.
 */
char* NBString::find_extension(void) const
{
    const char* lastSlash;
    const char* lastDot;
//    int extLen;
    const char* const str = mString;

    // only look at the filename
    lastSlash = strrchr(str, OS_PATH_SEPARATOR);
    if (lastSlash == NULL)
        lastSlash = str;
    else
        lastSlash++;

    // find the last dot
    lastDot = strrchr(lastSlash, '.');
    if (lastDot == NULL)
        return NULL;

    // looks good, ship it
    return const_cast<char*>(lastDot);
}

NBString NBString::getPathExtension(void) const
{
    char* ext;

    ext = find_extension();
    if (ext != NULL)
        return NBString(ext);
    else
        return NBString("");
}

NBString NBString::getBasePath(void) const
{
    char* ext;
    const char* const str = mString;

    ext = find_extension();
    if (ext == NULL)
        return NBString(*this);
    else
        return NBString(str, ext - str);
}

NBString& NBString::appendPath(const char* name)
{
    // TODO: The test below will fail for Win32 paths. Fix later or ignore.
    if (name[0] != OS_PATH_SEPARATOR) {
        if (*name == '\0') {
            // nothing to do
            return *this;
        }

        size_t len = length();
        if (len == 0) {
            // no existing filename, just use the new one
            setPathName(name);
            return *this;
        }

        // make room for oldPath + '/' + newPath
        size_t newlen = strlen(name);

        char* buf = lockBuffer(len+1+newlen);

        // insert a '/' if needed
        if (buf[len-1] != OS_PATH_SEPARATOR)
            buf[len++] = OS_PATH_SEPARATOR;

        memcpy(buf+len, name, newlen+1);
        len += newlen;

        unlockBuffer(len);

        return *this;
    } else {
        setPathName(name);
        return *this;
    }
}

NBString& NBString::convertToResPath()
{
#if OS_PATH_SEPARATOR != RES_PATH_SEPARATOR
    size_t len = length();
    if (len > 0) {
        char * buf = lockBuffer(len);
        for (char * end = buf + len; buf < end; ++buf) {
            if (*buf == OS_PATH_SEPARATOR)
                *buf = RES_PATH_SEPARATOR;
        }
        unlockBuffer(len);
    }
#endif
    return *this;
}
