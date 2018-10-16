//
// Created by parallels on 9/9/18.
//

#ifndef NBFFMPEGSOURCE_H
#define NBFFMPEGSOURCE_H

#include "NBDataSource.h"

struct AVFormatContext;

class NBFFmpegSource : public NBDataSource {
public:
    NBFFmpegSource(const NBString& uri);
    ~NBFFmpegSource();

public:
    virtual nb_status_t initCheck(const NBMap<NBString, NBString>* params);
    
    // return the current io context
    virtual void* getContext() {
        return mFormatCtx;
    }

    virtual NBString getUri() {
        return mUri;
    }

public:
    static void RegisterSource();
    static void UnRegisterSource();

private:
    const NBString& mUri;
    AVFormatContext* mFormatCtx;

private:
    static int CheckUserInterupted(void* opaque);
};

#endif //NBFFMPEGSOURCE_H
