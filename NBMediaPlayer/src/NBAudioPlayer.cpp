//
// Created by liuenbao on 18-9-12.
//

#include "NBAudioPlayer.h"
#include "NBMediaPlayer.h"
#include "decoder/NBMediaSource.h"
#include "foundation/NBMetaData.h"
#include "foundation/NBFoundation.h"
#include "renderer/NBALAudioRenderer.h"

#include <NBLog.h>

#ifdef __cplusplus
extern "C"
{
#endif
#include "libavcodec/avcodec.h"
#include "libswresample/swresample.h"
#ifdef __cplusplus
}
#endif

const int64_t AVNoSyncThreshold = 10000;
const int64_t AudioSyncThreshold = 30;
/* Averaging filter coefficient for audio sync. */
#define AUDIO_DIFF_AVG_NB 20
static double AudioAvgFilterCoeff;
const int64_t AudioSampleCorrectionMax = 50;

struct NBAudioEvent : public NBTimedEventQueue::Event {
    NBAudioEvent(
            NBAudioPlayer *audioPlayer,
            void (NBAudioPlayer::*method)())
            : mAudioPlayer(audioPlayer),
              mMethod(method) {
    }

protected:
    virtual ~NBAudioEvent() {}

    virtual void fire(NBTimedEventQueue *queue, int64_t /* now_us */) const {
        (mAudioPlayer->*mMethod)();
    }

private:
    NBAudioPlayer *mAudioPlayer;
    void (NBAudioPlayer::*mMethod)();

    NBAudioEvent(const NBAudioEvent &);
    NBAudioEvent &operator=(const NBAudioEvent &);
};

NBAudioPlayer::NBAudioPlayer(NBMediaPlayer* mediaPlayer, NBClockManager* clkManager)
        :mQueueStart(false)
        ,mALAudioRenderer(NULL)
        ,mObserver(mediaPlayer)
        ,mClockManager(clkManager) {

    mQueue.setQueueName(NBString("NBAudioPlayer"));

    mAudioEvent = new NBAudioEvent(this, &NBAudioPlayer::onAudioEvent);
    mAudioEventPending = false;

    mSamples = NULL;

    AudioAvgFilterCoeff = pow(0.01, 1.0/AUDIO_DIFF_AVG_NB);
}

NBAudioPlayer::~NBAudioPlayer() {
    if (mQueueStart) {
        mQueue.stop();
        mQueueStart = false;
    }

    if (mAudioEvent != NULL) {
        delete mAudioEvent;
        mAudioEvent = NULL;
    }

}

void NBAudioPlayer::setAudioSource(NBMediaSource* mediaSource) {
    NBAutoMutex autoMutex(mLock);
    mMediaSource = mediaSource;
}

bool NBAudioPlayer::reachedEOS(nb_status_t *finalStatus) {
    NBAutoMutex autoMutex(mLock);
    if (mFinalStatus == ERROR_END_OF_STREAM) {
        *finalStatus = mFinalStatus;
        return true;
    }
    return false;
}

nb_status_t NBAudioPlayer::start(bool sourceAlreadyStarted) {
    NBAutoMutex autoLock(mLock);

    mALAudioRenderer = new NBALAudioRenderer();
    mALAudioRenderer->start(mMediaSource->getFormat());

    int codecFormat = 0;
    mMediaSource->getFormat()->findInt32(kKeyAudioFromat, &codecFormat);

    uint64_t codecChLayout = 0;
    mMediaSource->getFormat()->findInt64(kKeyAudioChLayout, (int64_t*)&codecChLayout);

    int codecSampleRate = 0;
    mMediaSource->getFormat()->findInt32(kKeySampleRate, &codecSampleRate);

    int codecChannels = 0;
    mMediaSource->getFormat()->findInt32(kKeyChannelCount, &codecChannels);

    //Create swr context
    mSwrCtx = swr_alloc_set_opts(NULL,
                                 mALAudioRenderer->getDstChLayout(),
                                 (AVSampleFormat)mALAudioRenderer->getDstSampleFmt(),
                                 mALAudioRenderer->getDstSampleRate(),
                                 codecChLayout ? codecChLayout : (uint64_t)av_get_default_channel_layout(codecChannels),
                                 (AVSampleFormat)codecFormat,
                                 mALAudioRenderer->getDstSampleRate(),
                                 0, NULL);

    if(!mSwrCtx || swr_init(mSwrCtx) != 0) {
        printf("Failed to initialize audio converter");
        return UNKNOWN_ERROR;
    }

    //start event queue
    if (!mQueueStart) {
        mQueue.start();
        mQueueStart = true;
    }

    mSamplesMax = 0;
    mSamplesLen = 0;
    mSamplesPos = 0;

    mSeeking = NO_SEEK;

    // set clock manager for sync
    mClockManager->registerClockProvider(NBClockManager::AV_SYNC_AUDIO_MASTER, this);

    mOutSamples = NULL;
    mOutSamplesLen = 0;

    mAudioQueued = MAX_AL_BUFFER_COUNT / 2;

    postAudioEvent_l();

    return OK;
}

nb_status_t NBAudioPlayer::pause(bool at_eos) {
    NBAutoMutex autoLock(mLock);

    cancelAudioEvent_l();

    return mALAudioRenderer->pause(at_eos);
}

nb_status_t NBAudioPlayer::resume() {
    NBAutoMutex autoMutex(mLock);
    postAudioEvent_l();
    return OK;
}

nb_status_t NBAudioPlayer::seekTo(int64_t timeUs) {
    NBAutoMutex autoMutex(mLock);
    mSeeking = SEEK;
    mSeekTimeUs = timeUs;
    postAudioEvent_l();
    return OK;
}

int64_t NBAudioPlayer::getMediaTimeUs() {
    NBAutoMutex autoMutex(mLock);
    return mALAudioRenderer->getCurrentClock();
}

void NBAudioPlayer::stop() {
    NBAutoMutex autoLock(mLock);

    mClockManager->unregisterClockProvider(NBClockManager::AV_SYNC_AUDIO_MASTER);

    cancelAudioEvent_l();

    if (mALAudioRenderer != NULL) {
        mALAudioRenderer->stop();
        delete mALAudioRenderer;
        mALAudioRenderer = NULL;
    }
}

int64_t NBAudioPlayer::getSync() {
    if(mClockManager->getMasterSyncType() == NBClockManager::AV_SYNC_AUDIO_MASTER)
        return 0;

    int64_t ref_clock = mClockManager->getMasterClock();
    int64_t diff = ref_clock - mALAudioRenderer->getCurrentClock();

    if(!(diff < AVNoSyncThreshold && diff > -AVNoSyncThreshold))
    {
        /* Difference is TOO big; reset accumulated average */
        mClockDiffAvg = 0;
        return 0;
    }

    /* Accumulate the diffs */
    mClockDiffAvg = mClockDiffAvg*AudioAvgFilterCoeff + diff;
    int64_t avg_diff = mClockDiffAvg*(1.0 - AudioAvgFilterCoeff);
    if(avg_diff < AudioSyncThreshold/2.0 && avg_diff > -AudioSyncThreshold)
        return 0;

    /* Constrain the per-update difference to avoid exceedingly large skips */
    diff = FFMIN(FFMAX(diff, -AudioSampleCorrectionMax),  AudioSampleCorrectionMax);

    return diff * mALAudioRenderer->getDstSampleRate();
}

nb_status_t NBAudioPlayer::readAudio(uint8_t *samples, uint32_t reqLength, uint32_t* pRemLength, NBMediaSource::ReadOptions* options) {
    nb_status_t err = OK;
    int64_t sample_skip = getSync();
    NBMediaBuffer* audioBuffer = NULL;
    size_t frameSize = mALAudioRenderer->getFrameSize();

    uint32_t audio_size = 0;

    /* Read the next chunk of data, refill the buffer, and queue it
     * on the source */
    reqLength /= frameSize;

    while (audio_size < reqLength) {
        if(mSamplesLen <= 0 || mSamplesPos >= mSamplesLen) {
            err = mMediaSource->read(&audioBuffer, options);
            if (err != OK) {
                //Deocde audio error
                break;
            }

            AVFrame *decodeFrame = (AVFrame *) audioBuffer->data();

//            printf("The decoded audioFrame pts : %lld\n", decodeFrame->pts);

            if (decodeFrame->nb_samples > mSamplesMax) {
                av_freep(&mSamples);
                av_samples_alloc(
                        &mSamples, NULL, mALAudioRenderer->getDstChannnels(),
                        decodeFrame->nb_samples, (AVSampleFormat) mALAudioRenderer->getDstSampleFmt(), 0
                );
                mSamplesMax = decodeFrame->nb_samples;
            }

            /* Return the amount of sample frames converted */
            int data_size = swr_convert(mSwrCtx, &mSamples, decodeFrame->nb_samples,
                                        (const uint8_t **) decodeFrame->data, decodeFrame->nb_samples
            );

            mSamplesLen = data_size;
            mSamplesPos = FFMIN(mSamplesLen, sample_skip);
            mALAudioRenderer->setCurrentPts(decodeFrame->pts);

            sample_skip -= mSamplesPos;

            // Adjust the device start time and current pts by the amount we're
            // skipping/duplicating, so that the clock remains correct for the
            // current stream position.
            int64_t skip = mSamplesPos * 1000.0f / mALAudioRenderer->getDstSampleRate();
            mALAudioRenderer->addCurrentPts(skip);

            delete audioBuffer;

            //maybe need decode a new frame
            continue;
        }

        int64_t rem = reqLength - audio_size;
        if(mSamplesPos >= 0)
        {
            int64_t len = mSamplesLen - mSamplesPos;
            if(rem > len) rem = len;
            memcpy(samples, mSamples + mSamplesPos*frameSize, rem*frameSize);
        }
        else
        {
            rem = FFMIN(rem, -mSamplesPos);

            /* Add samples by copying the first sample */
            memcpy(samples, mSamples, rem * frameSize);
        }

//        printf("the audio value is : %lld\n", (int64_t)(rem * 1000.0f / mALAudioRenderer->getDstSampleRate()));

        mSamplesPos += rem;
        mALAudioRenderer->addCurrentPts(rem * 1000.0f / mALAudioRenderer->getDstSampleRate());
        samples += rem*frameSize;
        audio_size += rem;
    }

    if (audio_size < reqLength) {
        *pRemLength = (reqLength - audio_size) * frameSize;
        return err;
    }

//    if(audio_size < length) {
//        int rem = length - audio_size;
//
//        memset(samples, (mALAudioRenderer->getDstSampleFmt() == AV_SAMPLE_FMT_U8) ? 0x80 : 0x00, rem* mALAudioRenderer->getFrameSize());
//
//        mALAudioRenderer->addCurrentPts(rem * 1000.0f / mALAudioRenderer->getDstSampleRate());
//        audio_size += rem;
//    }

    return OK;
}

void NBAudioPlayer::onAudioEvent() {
    NBAutoMutex _lock(mLock);

    if (!mAudioEventPending) {
        return;
    }
    mAudioEventPending = false;
    NBMediaSource::ReadOptions options;
    
    if (mSeeking != NO_SEEK) {
        mALAudioRenderer->flushALContext();
        mAudioQueued = MAX_AL_BUFFER_COUNT / 2;
        mOutSamples = NULL;
        mOutSamplesLen = 0;
        mSamplesLen = 0;
        
        options.setSeekTo(
                          mSeekTimeUs,
                          mSeeking == SEEK_VIDEO_ONLY
                          ? NBMediaSource::ReadOptions::SEEK_NEXT_SYNC
                          : NBMediaSource::ReadOptions::SEEK_CLOSEST_SYNC);
    }

    if (mOutSamples == NULL)
        mOutSamplesLen = mALAudioRenderer->acquireBuffer(&mOutSamples);

    if (mOutSamples != NULL) {
//        uint32_t reqLen = mOutSamplesLen;
        nb_status_t err = readAudio(mOutSamples + mALAudioRenderer->getBufferLen() - mOutSamplesLen, mOutSamplesLen, &mOutSamplesLen);

//        printf("The audio sample len : %d rem : %d err : %d\n", reqLen, mOutSamplesLen, err);

        if (err == OK) {
            postAudioEvent_l(0);
            mOutSamples = NULL;
            mOutSamplesLen = 0;

            if (mAudioQueued > 0) {
                --mAudioQueued;
            }

            mALAudioRenderer->releaseBuffer(true);
        } else {
            if (err == INFO_DISCONTINUITY) {
                mSamplesLen = 0;
                mSamplesPos = 0;
                mSeeking = NO_SEEK;
                postAudioEvent_l(0);
            } else if (err != ERROR_END_OF_STREAM) {
                postAudioEvent_l(100);
            } else {
                //We reached the end of file
                mFinalStatus = ERROR_END_OF_STREAM;
                mObserver->postAudioEOS();
            }
        }
    } else {
        postAudioEvent_l(100000);
    }

    if (mAudioQueued <= 0 && mSeeking == NO_SEEK) {
        mALAudioRenderer->play();
    }
}

void NBAudioPlayer::postAudioEvent_l(int64_t delayUs) {
    if (mAudioEventPending) {
        return ;
    }
    mAudioEventPending = true;
    mQueue.postEventWithDelay(mAudioEvent, delayUs < 0 ? 10000 : delayUs);
}

void NBAudioPlayer::cancelAudioEvent_l() {
    if (!mAudioEventPending) {
        return ;
    }
    mAudioEventPending = false;
    mQueue.cancelEvent(mAudioEvent->eventID());
}

int64_t NBAudioPlayer::getCurrentClock() {
    NBAutoMutex autoLock(mLock);
    return mALAudioRenderer->getCurrentClock();
}
