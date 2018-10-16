//
// Created by parallels on 9/9/18.
//

#ifndef NBFFMPEGEXTRACTOR_H
#define NBFFMPEGEXTRACTOR_H

#include <NBMutex.h>
#include <NBTimedEventQueue.h>

#include "NBMediaExtractor.h"
#include "foundation/NBFoundation.h"

struct AVFormatContext;

class NBFFmpegExtractor : public NBMediaExtractor {
public:
    NBFFmpegExtractor(INBMediaSeekListener* seekListener, NBDataSource* dataSource);
    ~NBFFmpegExtractor();

public:
    virtual nb_status_t sniffMedia();

    virtual void start(NBMetaData* metaData);

    virtual void stop();

    virtual nb_status_t seekTo(int64_t millisec);

    virtual NBMediaSource* getTrack(size_t index);
    virtual NBMetaData* getTrackMetaData(
            size_t index, uint32_t flags = 0);

    //notify continue read
    virtual void notifyContinueRead();

    virtual bool getReachedEnd();

private:
    NBDataSource* mDataSource;
    AVFormatContext* mFormatContext;

    NBMutex mLock;

    NBTimedEventQueue mQueue;
    bool mQueueStarted;

    NBTimedEventQueue::Event* mExtractorEvent;
    bool mExtractorEventPending;

    void postExtractorEvent_l(int64_t delayUs = -1);
    void cancelExtractorEvent_l();
    void onExtractorEvent();
};

#endif //NBFFMPEGEXTRACTOR_H
