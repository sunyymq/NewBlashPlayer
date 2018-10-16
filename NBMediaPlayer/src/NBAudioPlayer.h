//
// Created by liuenbao on 18-9-12.
//

#ifndef NBAUDIOPLAYER_H
#define NBAUDIOPLAYER_H

#include <NBTimedEventQueue.h>
#include "NBClockManager.h"
#include "decoder/NBMediaSource.h"
#include "foundation/NBMetaData.h"

class NBMediaSource;
class NBALAudioRenderer;
class NBClockManager;
class NBMediaPlayer;

struct SwrContext;

class NBAudioPlayer : public NBClockProvider {
public:
    NBAudioPlayer(NBMediaPlayer* mediaPlayer, NBClockManager* clkManager);
    ~NBAudioPlayer();

public:
    void setAudioSource(NBMediaSource* mediaSource);

    bool reachedEOS(nb_status_t *finalStatus);

    nb_status_t start(bool sourceAlreadyStarted = false);

    nb_status_t pause(bool at_eos = false);

    nb_status_t resume();

    nb_status_t seekTo(int64_t timeUs);

    int64_t getMediaTimeUs();

    void stop();

public:
    virtual int64_t getCurrentClock();

private:
    NBTimedEventQueue mQueue;
    bool mQueueStart;

    NBMutex mLock;

    void postAudioEvent_l(int64_t delayUs = -1);
    void cancelAudioEvent_l();
    void onAudioEvent();

    NBTimedEventQueue::Event* mAudioEvent;
    bool mAudioEventPending;

    enum SeekType {
        NO_SEEK,
        SEEK,
        SEEK_VIDEO_ONLY,
        SEEK_ACTIONED,
    };
    SeekType mSeeking;
    int64_t mSeekTimeUs;

    NBMediaPlayer* mObserver;

private:
    NBMediaSource* mMediaSource;
    NBALAudioRenderer* mALAudioRenderer;
    NBClockManager* mClockManager;
    int64_t mClockDiffAvg;

    nb_status_t mFinalStatus;

private:
    nb_status_t readAudio(uint8_t *samples, uint32_t reqLength, uint32_t* pRemLength, NBMediaSource::ReadOptions* options = NULL);
    int64_t getSync();

    uint8_t* mSamples;
    int32_t mSamplesLen; /* In samples */
    int64_t mSamplesPos;
    int32_t mSamplesMax;

    uint8_t* mOutSamples;
    uint32_t mOutSamplesLen;

    uint32_t mAudioQueued;

    //The ffmpeg related
    SwrContext* mSwrCtx;
};

#endif //NBAUDIOPLAYER_H
