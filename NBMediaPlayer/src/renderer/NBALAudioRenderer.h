//
// Created by parallels on 9/10/18.
//

#ifndef NBALAUDIORENDERER_H
#define NBALAUDIORENDERER_H

#include <NBTimedEventQueue.h>
#include "foundation/NBMetaData.h"

//OpenAL Context
typedef struct ALCdevice_struct ALCdevice;
typedef struct ALCcontext_struct ALCcontext;

#define MAX_AL_BUFFER_COUNT 40

class NBALAudioRenderer {
public:
    NBALAudioRenderer();
    ~NBALAudioRenderer();

    nb_status_t start(NBMetaData *metaData = NULL);

    nb_status_t stop();

    nb_status_t pause(bool at_eos);

    nb_status_t play();

    nb_status_t flushALContext();

    uint32_t acquireBuffer(uint8_t** pPtr);
    void releaseBuffer(bool shoudRender);

    inline size_t getFrameSize() {
        return mFrameSize;
    }

    inline int getDstChannnels() {
        return mDstChannels;
    }

    inline int64_t getDstChLayout() {
        return mDstChanLayout;
    }

    inline int getDstSampleFmt() {
        return mDstSampleFmt;
    }

    inline int32_t getBufferLen() {
        return mBufferLen;
    }

    inline int32_t getDstSampleRate() {
        return mDstSampleRate;
    }

    int64_t getCurrentClock();

    inline void setCurrentPts(int64_t curPts) {
        mCurrentPts = curPts;
    }

    inline void addCurrentPts(int64_t added) {
        mCurrentPts += added;
    }

private:
    ALCdevice* mALCDev;
    ALCcontext* mALCCtx;

    uint32_t mALBuffers[MAX_AL_BUFFER_COUNT];
    uint32_t mBufferIdx;

    uint8_t * mBuffer;
    int32_t mBufferLen;

    uint32_t mALSource;

    //target value
    uint64_t mDstChanLayout;
    int32_t mDstChannels;
    int mDstSampleFmt;
    int32_t mDstSampleRate;
    size_t mFrameSize;
    int mFormat;

    int64_t mCurrentPts;
};

#endif //NBALAUDIORENDERER_H
