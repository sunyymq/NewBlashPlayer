//
// Created by parallels on 9/7/18.
//

#ifndef NBMEDIAEXTRACTOR_H
#define NBMEDIAEXTRACTOR_H

#include <NBMediaPlayer.h>

#include "NBMacros.h"

class NBDataSource;
class NBMediaSource;
class NBMetaData;

#define MAX_META_DATA_COUNT 32

class NBMediaExtractor {
public:
    NBMediaExtractor(INBMediaSeekListener* seekListener)
        :mHasVideoStream(false)
        ,mHasAudioStream(false)
        ,mReachedEnd(false)
        ,mSeekTimeUs(-1)
        ,mFlags(CAN_SEEK)
        ,mMetaDataIndex(0)
        ,mSeekListener(seekListener) {

    }

    virtual ~NBMediaExtractor() {
    }

    enum Flags {
        CAN_SEEK_BACKWARD  = 1,  // the "seek 10secs back button"
        CAN_SEEK_FORWARD   = 2,  // the "seek 10secs forward button"
        CAN_PAUSE          = 4,
        CAN_SEEK           = 8,  // the "seek bar"
    };

public:
    static NBMediaExtractor* Create(INBMediaSeekListener* seekListener, NBDataSource* dataSource, const char *mime = NULL);
    static void Destroy(NBMediaExtractor* extractor);

    virtual nb_status_t sniffMedia() {
        return OK;
    }

    virtual void start(NBMetaData* metaData) {
    }

    virtual void stop() {
    }

    virtual nb_status_t seekTo(int64_t millisec) {
        return OK;
    }

    int64_t getSeekTimeUs() {
        return mSeekTimeUs;
    }

    size_t countTracks() {
        return mMetaDataIndex;
    }

    virtual NBMediaSource* getTrack(size_t index) = 0;
    virtual NBMetaData* getTrackMetaData(
            size_t index, uint32_t flags = 0) = 0;

    virtual uint32_t flags() const {
        return mFlags;
    }

    // the request queue is empty
    virtual void notifyContinueRead() = 0;

    // retrieve the reachend file
    virtual bool getReachedEnd() { return true; }

protected:
    // Return container specific meta-data. The default implementation
    // returns an empty metadata object.
    virtual NBMetaData* getMetaData();

protected:
    bool mHasVideoStream;
    bool mHasAudioStream;

    NBMetaData* mMetaDatas[MAX_META_DATA_COUNT];
    size_t mMetaDataIndex;

    uint32_t mFlags;
    uint32_t mSeekFlag;

    int64_t mSeekTimeUs;

    bool mReachedEnd;

    INBMediaSeekListener* mSeekListener;
};

#endif //NBMEDIAEXTRACTOR_H
