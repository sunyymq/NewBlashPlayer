//
// Created by parallels on 9/8/18.
//

#ifndef NBSTRING_H
#define NBSTRING_H

#include <string.h>
#include <stdarg.h>
#include <NBMacros.h>
#include <NBSharedBuffer.h>

//! This is a string holding UTF-8 characters. Does not allow the value more
// than 0x10FFFF, which is not valid unicode codepoint.
class NBString
{
public:
    /* use NBString(StaticLinkage) if you're statically linking against
     * libutils and declaring an empty static NBString, e.g.:
     *
     *   static NBString sAStaticEmptyString(NBString::kEmptyString);
     *   static NBString sAnotherStaticEmptyString(sAStaticEmptyString);
     */
    enum StaticLinkage { kEmptyString };

    NBString();
    explicit                    NBString(StaticLinkage);
    NBString(const NBString& o);
    explicit                    NBString(const char* o);
    explicit                    NBString(const char* o, size_t numChars);

//    explicit                    NBString(const String16& o);
//    explicit                    NBString(const char16_t* o);
//    explicit                    NBString(const char16_t* o, size_t numChars);
//    explicit                    NBString(const char32_t* o);
//    explicit                    NBString(const char32_t* o, size_t numChars);
    ~NBString();

    static inline const NBString empty();

    static NBString              format(const char* fmt, ...) __attribute__((format (printf, 1, 2)));
    static NBString              formatV(const char* fmt, va_list args);

    inline  const char*         string() const;
    inline  size_t              size() const;
    inline  size_t              length() const;
    inline  size_t              bytes() const;
    inline  bool                isEmpty() const;

    inline  const NBSharedBuffer* sharedBuffer() const;

    void                clear();

    void                setTo(const NBString& other);
    nb_status_t            setTo(const char* other);
    nb_status_t            setTo(const char* other, size_t numChars);
//    nb_status_t            setTo(const char16_t* other, size_t numChars);
//    nb_status_t            setTo(const char32_t* other,
//                              size_t length);

    nb_status_t            append(const NBString& other);
    nb_status_t            append(const char* other);
    nb_status_t            append(const char* other, size_t numChars);

    nb_status_t            appendFormat(const char* fmt, ...)

    __attribute__((format (printf, 2, 3)));
    nb_status_t            appendFormatV(const char* fmt, va_list args);

//    // Note that this function takes O(N) time to calculate the value.
//    // No cache value is stored.
//    size_t              getUtf32Length() const;
//    int32_t             getUtf32At(size_t index,
//                                   size_t *next_index) const;
//    void                getUtf32(char32_t* dst) const;

    inline  NBString&            operator=(const NBString& other);
    inline  NBString&            operator=(const char* other);

    inline  NBString&            operator+=(const NBString& other);
    inline  NBString             operator+(const NBString& other) const;

    inline  NBString&            operator+=(const char* other);
    inline  NBString             operator+(const char* other) const;

    inline  int                 compare(const NBString& other) const;

    inline  bool                operator<(const NBString& other) const;
    inline  bool                operator<=(const NBString& other) const;
    inline  bool                operator==(const NBString& other) const;
    inline  bool                operator!=(const NBString& other) const;
    inline  bool                operator>=(const NBString& other) const;
    inline  bool                operator>(const NBString& other) const;

    inline  bool                operator<(const char* other) const;
    inline  bool                operator<=(const char* other) const;
    inline  bool                operator==(const char* other) const;
    inline  bool                operator!=(const char* other) const;
    inline  bool                operator>=(const char* other) const;
    inline  bool                operator>(const char* other) const;

    inline                      operator const char*() const;

    char*               lockBuffer(size_t size);
    void                unlockBuffer();
    nb_status_t            unlockBuffer(size_t size);

    // return the index of the first byte of other in this at or after
    // start, or -1 if not found
    ssize_t             find(const char* other, size_t start = 0) const;

    void                toLower();
    void                toLower(size_t start, size_t numChars);
    void                toUpper();
    void                toUpper(size_t start, size_t numChars);

    /*
     * These methods operate on the string as if it were a path name.
     */

    /*
     * Set the filename field to a specific value.
     *
     * Normalizes the filename, removing a trailing '/' if present.
     */
    void setPathName(const char* name);
    void setPathName(const char* name, size_t numChars);

    /*
     * Get just the filename component.
     *
     * "/tmp/foo/bar.c" --> "bar.c"
     */
    NBString getPathLeaf(void) const;

    /*
     * Remove the last (file name) component, leaving just the directory
     * name.
     *
     * "/tmp/foo/bar.c" --> "/tmp/foo"
     * "/tmp" --> "" // ????? shouldn't this be "/" ???? XXX
     * "bar.c" --> ""
     */
    NBString getPathDir(void) const;

    /*
     * Retrieve the front (root dir) component.  Optionally also return the
     * remaining components.
     *
     * "/tmp/foo/bar.c" --> "tmp" (remain = "foo/bar.c")
     * "/tmp" --> "tmp" (remain = "")
     * "bar.c" --> "bar.c" (remain = "")
     */
    NBString walkPath(NBString* outRemains = NULL) const;

    /*
     * Return the filename extension.  This is the last '.' and any number
     * of characters that follow it.  The '.' is included in case we
     * decide to expand our definition of what constitutes an extension.
     *
     * "/tmp/foo/bar.c" --> ".c"
     * "/tmp" --> ""
     * "/tmp/foo.bar/baz" --> ""
     * "foo.jpeg" --> ".jpeg"
     * "foo." --> ""
     */
    NBString getPathExtension(void) const;

    /*
     * Return the path without the extension.  Rules for what constitutes
     * an extension are described in the comment for getPathExtension().
     *
     * "/tmp/foo/bar.c" --> "/tmp/foo/bar"
     */
    NBString getBasePath(void) const;

    /*
     * Add a component to the pathname.  We guarantee that there is
     * exactly one path separator between the old path and the new.
     * If there is no existing name, we just copy the new name in.
     *
     * If leaf is a fully qualified path (i.e. starts with '/', it
     * replaces whatever was there before.
     */
    NBString& appendPath(const char* leaf);
    NBString& appendPath(const NBString& leaf)  { return appendPath(leaf.string()); }

    /*
     * Like appendPath(), but does not affect this string.  Returns a new one instead.
     */
    NBString appendPathCopy(const char* leaf) const
    { NBString p(*this); p.appendPath(leaf); return p; }
    NBString appendPathCopy(const NBString& leaf) const { return appendPathCopy(leaf.string()); }

    /*
     * Converts all separators in this string to /, the default path separator.
     *
     * If the default OS separator is backslash, this converts all
     * backslashes to slashes, in-place. Otherwise it does nothing.
     * Returns self.
     */
    NBString& convertToResPath();

private:
    nb_status_t            real_append(const char* other, size_t numChars);
    char*               find_extension(void) const;

    const char* mString;
};

//// NBString can be trivially moved using memcpy() because moving does not
//// require any change to the underlying SharedBuffer contents or reference count.
//ANDROID_TRIVIAL_MOVE_TRAIT(NBString)

// ---------------------------------------------------------------------------
// No user servicable parts below.

inline int compare_type(const NBString& lhs, const NBString& rhs)
{
    return lhs.compare(rhs);
}

inline int strictly_order_type(const NBString& lhs, const NBString& rhs)
{
    return compare_type(lhs, rhs) < 0;
}

inline const NBString NBString::empty() {
    return NBString();
}

inline const char* NBString::string() const
{
    return mString;
}

inline size_t NBString::length() const
{
    return NBSharedBuffer::sizeFromData(mString)-1;
}

inline size_t NBString::size() const
{
    return length();
}

inline bool NBString::isEmpty() const
{
    return length() == 0;
}

inline size_t NBString::bytes() const
{
    return NBSharedBuffer::sizeFromData(mString)-1;
}

inline const NBSharedBuffer* NBString::sharedBuffer() const
{
    return NBSharedBuffer::bufferFromData(mString);
}

inline NBString& NBString::operator=(const NBString& other)
{
    setTo(other);
    return *this;
}

inline NBString& NBString::operator=(const char* other)
{
    setTo(other);
    return *this;
}

inline NBString& NBString::operator+=(const NBString& other)
{
    append(other);
    return *this;
}

inline NBString NBString::operator+(const NBString& other) const
{
    NBString tmp(*this);
    tmp += other;
    return tmp;
}

inline NBString& NBString::operator+=(const char* other)
{
    append(other);
    return *this;
}

inline NBString NBString::operator+(const char* other) const
{
    NBString tmp(*this);
    tmp += other;
    return tmp;
}

inline int NBString::compare(const NBString& other) const
{
    return strcmp(mString, other.mString);
}

inline bool NBString::operator<(const NBString& other) const
{
    return strcmp(mString, other.mString) < 0;
}

inline bool NBString::operator<=(const NBString& other) const
{
    return strcmp(mString, other.mString) <= 0;
}

inline bool NBString::operator==(const NBString& other) const
{
    return strcmp(mString, other.mString) == 0;
}

inline bool NBString::operator!=(const NBString& other) const
{
    return strcmp(mString, other.mString) != 0;
}

inline bool NBString::operator>=(const NBString& other) const
{
    return strcmp(mString, other.mString) >= 0;
}

inline bool NBString::operator>(const NBString& other) const
{
    return strcmp(mString, other.mString) > 0;
}

inline bool NBString::operator<(const char* other) const
{
    return strcmp(mString, other) < 0;
}

inline bool NBString::operator<=(const char* other) const
{
    return strcmp(mString, other) <= 0;
}

inline bool NBString::operator==(const char* other) const
{
    return strcmp(mString, other) == 0;
}

inline bool NBString::operator!=(const char* other) const
{
    return strcmp(mString, other) != 0;
}

inline bool NBString::operator>=(const char* other) const
{
    return strcmp(mString, other) >= 0;
}

inline bool NBString::operator>(const char* other) const
{
    return strcmp(mString, other) > 0;
}

inline NBString::operator const char*() const
{
    return mString;
}

#endif //NBSTRING_H
