//
// Created by liuenbao on 18-9-21.
//

#ifndef NBLOG_H
#define NBLOG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NBLOG_DISABLE
#define NBLOG_DEFINE() NBLogCtx* global_logCtx = NULL;
#define NBLOG_INIT(log_root, mode) global_logCtx = nb_log_init(log_root, mode)
#define NBLOG_DEINIT() nb_log_deinit(global_logCtx)
#define NBLOG_VERBOSE(tag, fmt, args...) do { extern NBLogCtx* global_logCtx; nb_log_print(global_logCtx, NBLOG_PRI_VERBOSE, tag, fmt, ##args); } while(0)
#define NBLOG_DEBUG(tag, fmt, args...) do { extern NBLogCtx* global_logCtx; nb_log_print(global_logCtx, NBLOG_PRI_DEBUG, tag, fmt, ##args); } while(0)
#define NBLOG_INFO(tag, fmt, args...) do { extern NBLogCtx* global_logCtx; nb_log_print(global_logCtx, NBLOG_PRI_INFO, tag, fmt, ##args); } while (0)
#define NBLOG_WARN(tag, fmt, args...) do { extern NBLogCtx* global_logCtx; nb_log_print(global_logCtx, NBLOG_PRI_WARN, tag, fmt, ##args); } while (0)
#define NBLOG_ERROR(tag, fmt, args...) do { extern NBLogCtx* global_logCtx; nb_log_print(global_logCtx, NBLOG_PRI_ERROR, tag, fmt, ##args); } while (0)
#define NBLOG_FATAL(tag, fmt, args...) do { extern NBLogCtx* global_logCtx; nb_log_print(global_logCtx, NBLOG_PRI_FATAL, tag, fmt, ##args); } while (0)
#else
#define NBLOG_DEFINE()
#define NBLOG_INIT(log_root, mode)
#define NBLOG_DEINIT()
#define NBLOG_VERBOSE(tag, fmt...)
#define NBLOG_DEBUG(tag, fmt...)
#define NBLOG_INFO(tag, fmt...)
#define NBLOG_WARN(tag, fmt...)
#define NBLOG_ERROR(tag, fmt...)
#define NBLOG_FATAL(tag, fmt...)
#endif

/**
* NBLog priority values, in ascending priority order.
*/
typedef enum NBLogPriority {
    NBLOG_PRI_UNKNOWN = 0,
    NBLOG_PRI_DEFAULT,    /* only for SetMinPriority() */
    NBLOG_PRI_VERBOSE,
    NBLOG_PRI_DEBUG,
    NBLOG_PRI_INFO,
    NBLOG_PRI_WARN,
    NBLOG_PRI_ERROR,
    NBLOG_PRI_FATAL,
    NBLOG_PRI_SILENT,     /* only for SetMinPriority(); must be last */
} NBLogPriority;

typedef struct NBLogCtx NBLogCtx;

//The log message define
typedef struct LogMessage {
    //the log message time value
    struct timeval msg_time;

    NBLogPriority priority;

    const char* tag;
    const char* msg;
    int msg_size;
} LogMessage;

//The appender context
typedef struct NBLogAppender NBLogAppender;

typedef struct INBLogAppender {
    void* (*open)(NBLogCtx* ctx);
    void (*close)(void* apdCtx);
    int (*append)(void* apdCtx, const LogMessage* message);
} INBLogAppender;

// log with mmap file appender , this mode need log_root must not be writable
#define NBLOG_MODE_MFILE       0x01
// log with console appender
#define NBLOG_MODE_CONSOLE     0x02

/**
 * NBLog init function
 * @param configure file path
 * @mode the log configure mode, see above
 * @return
 */
NBLogCtx* nb_log_init(const char* log_root, unsigned int mode);

/**
 * NBLog deinit function
 */
void nb_log_deinit(NBLogCtx* ctx);

/**
 * NBLog get log root
 */
const char* nb_log_get_root(NBLogCtx* ctx);

/**
 * Set the log priority
 * @param ctx
 * @param logPriority
 */
void nb_log_set_priority(NBLogCtx* ctx, NBLogPriority logPriority);

/**
 * NBLog register appender
 */
int nb_log_reg_appender(NBLogCtx* ctx, const char* name, INBLogAppender* appender);

/**
 * NBLog unregister appender
 */
void nb_log_unreg_appender(NBLogCtx* ctx, const char* name);

/**
 * Send a simple string to the log.
 */
int nb_log_write(NBLogCtx* ctx, int prio, const char *tag, const char *text);

/**
 * Send a formatted string to the log, used like printf(fmt,...)
 */
int nb_log_print(NBLogCtx* ctx, int prio, const char *tag,  const char *fmt, ...)
#if defined(__GNUC__)
__attribute__ ((format(printf, 4, 5)))
#endif
;

/**
 * A variant of __android_log_print() that takes a va_list to list
 * additional parameters.
 */
int nb_log_vprint(NBLogCtx* ctx, int prio, const char *tag,
                         const char *fmt, va_list ap);


/**
 * Log an assertion failure and SIGTRAP the process to have a chance
 * to inspect it, if a debugger is attached. This uses the FATAL priority.
 */
void nb_log_assert(NBLogCtx* ctx, const char *cond, const char *tag,
                          const char *fmt, ...)
#if defined(__GNUC__)
__attribute__ ((noreturn))
__attribute__ ((format(printf, 4, 5)))
#endif
;

#ifdef __cplusplus
}
#endif

#endif //NBLOG_H
