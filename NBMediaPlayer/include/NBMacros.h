//
// Created by parallels on 9/7/18.
//

#ifndef NBMACROS_H
#define NBMACROS_H

#include <stdint.h>
#include <sys/types.h>
#include <errno.h>
#include <stdlib.h>

typedef struct NBRenderInfo {
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
} NBRenderInfo;

#define DISABLE_INSTANCES(name)         \
        name() {                        \
        }                               \
        ~name() {                       \
        }                               \

// the constructor & destructor macro
#define CONSTRUCTOR_DEFINE(name)                                \
        static void __attribute__((constructor))  name()        \

#define DESTRUCTOR_DEFINE(name)                                \
        static void __attribute__((destructor))  name()        \

// use this type to return error codes
typedef uint32_t     nb_status_t;

/*
 * Error codes.
 * All error codes are negative values.
 */

enum {
    OK                = 0,    // Everything's swell.
    NO_ERROR          = 0,    // No errors.

    UNKNOWN_ERROR       = 0x80000000,

    NO_MEMORY           = -ENOMEM,
    INVALID_OPERATION   = -ENOSYS,
    BAD_VALUE           = -EINVAL,
    BAD_TYPE            = 0x80000001,
    NAME_NOT_FOUND      = -ENOENT,
    PERMISSION_DENIED   = -EPERM,
    NO_INIT             = -ENODEV,
    ALREADY_EXISTS      = -EEXIST,
    DEAD_OBJECT         = -EPIPE,
    FAILED_TRANSACTION  = 0x80000002,
    JPARKS_BROKE_IT     = -EPIPE,
    BAD_INDEX           = -EOVERFLOW,
    NOT_ENOUGH_DATA     = -ENODATA,
    WOULD_BLOCK         = -EWOULDBLOCK,
    TIMED_OUT           = -ETIMEDOUT,
    UNKNOWN_TRANSACTION = -EBADMSG,
    FDS_NOT_ALLOWED     = 0x80000007,

    UNINITIALIZED       = FDS_NOT_ALLOWED + 1024,
};

enum {
    MEDIA_ERROR_BASE        = -1000,

    ERROR_ALREADY_CONNECTED = MEDIA_ERROR_BASE,
    ERROR_NOT_CONNECTED     = MEDIA_ERROR_BASE - 1,
    ERROR_UNKNOWN_HOST      = MEDIA_ERROR_BASE - 2,
    ERROR_CANNOT_CONNECT    = MEDIA_ERROR_BASE - 3,
    ERROR_IO                = MEDIA_ERROR_BASE - 4,
    ERROR_CONNECTION_LOST   = MEDIA_ERROR_BASE - 5,
    ERROR_MALFORMED         = MEDIA_ERROR_BASE - 7,
    ERROR_OUT_OF_RANGE      = MEDIA_ERROR_BASE - 8,
    ERROR_BUFFER_TOO_SMALL  = MEDIA_ERROR_BASE - 9,
    ERROR_UNSUPPORTED       = MEDIA_ERROR_BASE - 10,
    ERROR_END_OF_STREAM     = MEDIA_ERROR_BASE - 11,
    ERROR_INTERRUPTED       = MEDIA_ERROR_BASE - 12,

    //The ffmpeg may be seek with an error
    ERROR_MEDIA_SEEK        = MEDIA_ERROR_BASE - 15,

    // Not technically an error.
    INFO_FORMAT_CHANGED    = MEDIA_ERROR_BASE - 13,
    INFO_DISCONTINUITY     = MEDIA_ERROR_BASE - 14,

    // The following constant values should be in sync with
    // drm/drm_framework_common.h
    DRM_ERROR_BASE = -2000,

    ERROR_DRM_UNKNOWN                       = DRM_ERROR_BASE,
    ERROR_DRM_NO_LICENSE                    = DRM_ERROR_BASE - 1,
    ERROR_DRM_LICENSE_EXPIRED               = DRM_ERROR_BASE - 2,
    ERROR_DRM_SESSION_NOT_OPENED            = DRM_ERROR_BASE - 3,
    ERROR_DRM_DECRYPT_UNIT_NOT_INITIALIZED  = DRM_ERROR_BASE - 4,
    ERROR_DRM_DECRYPT                       = DRM_ERROR_BASE - 5,
    ERROR_DRM_CANNOT_HANDLE                 = DRM_ERROR_BASE - 6,
    ERROR_DRM_TAMPER_DETECTED               = DRM_ERROR_BASE - 7,

    // Heartbeat Error Codes
    HEARTBEAT_ERROR_BASE = -3000,

    ERROR_HEARTBEAT_AUTHENTICATION_FAILURE                  = HEARTBEAT_ERROR_BASE,
    ERROR_HEARTBEAT_NO_ACTIVE_PURCHASE_AGREEMENT            = HEARTBEAT_ERROR_BASE - 1,
    ERROR_HEARTBEAT_CONCURRENT_PLAYBACK                     = HEARTBEAT_ERROR_BASE - 2,
    ERROR_HEARTBEAT_UNUSUAL_ACTIVITY                        = HEARTBEAT_ERROR_BASE - 3,
    ERROR_HEARTBEAT_STREAMING_UNAVAILABLE                   = HEARTBEAT_ERROR_BASE - 4,
    ERROR_HEARTBEAT_CANNOT_ACTIVATE_RENTAL                  = HEARTBEAT_ERROR_BASE - 5,
    ERROR_HEARTBEAT_TERMINATE_REQUESTED                     = HEARTBEAT_ERROR_BASE - 6,
};

#endif //NBMACROS_H
