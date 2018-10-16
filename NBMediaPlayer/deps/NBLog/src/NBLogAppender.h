//
// Created by liuenbao on 18-9-21.
//

#ifndef NBLOGAPPENDER_H
#define NBLOGAPPENDER_H

#include <NBLog.h>

struct NBLogAppender {
    //The appender name
    const char* name;

    //The appender operation
    INBLogAppender logAppender;
    void* appenderData;

    //The appender list
    struct NBLogAppender* next;

    bool isOpened;
};

int logAppenderOpen(NBLogCtx* ctx, NBLogAppender* appender);
void logAppenderClose(NBLogAppender* appender);

#endif //NBLOGAPPENDER_H
