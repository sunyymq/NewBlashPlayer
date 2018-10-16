//
// Created by parallels on 9/7/18.
//

#include "NBMediaPlayer.h"

#include "foundation/NBFoundation.h"
#include "datasource/NBFFmpegSource.h"
#include "download_engine.h"

CONSTRUCTOR_DEFINE(loadPrerequestPlugin) {
    initialize_NBString();

    NBDataSource::RegisterSource();
}

DESTRUCTOR_DEFINE(unloadPrerequestPlugin) {
    
    NBDataSource::UnRegisterSource();

    deinitialize_NBString();
}

#include "NBAudioPlayer.h"
#include "NBClockManager.h"

#include "foundation/NBMetaData.h"
#include "datasource/NBDataSource.h"
#include "foundation/NBFoundation.h"
#include "decoder/NBMediaSource.h"
#include "decoder/NBMediaDecoder.h"
#include "extractor/NBMediaExtractor.h"
#include "renderer/NBGLVideoRenderer.h"
#include "renderer/NBGLYUV420PRenderer.h"
#include "renderer/NBMediaCodecVRenderer.h"

#ifdef BUILD_TARGET_IOS
#include "renderer/NBCVOpenGLESRenderer.h"
#endif

#include <stdio.h>

#include <NBLog.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <libavformat/avformat.h>

#ifdef __cplusplus
}
#endif

#define LOG_TAG "NBMediaPlayer"

class NBClockProxy : public NBClockManager {
public:
    NBClockProxy(NBMediaPlayer* mp)
            :mMediaPlayer(mp) {
        mClockManager = new NBClockManager();
    }
    ~NBClockProxy() {
        if (mClockManager != NULL) {
            delete mClockManager;
            mClockManager = NULL;
        }
    }

public:
    NBClockManager* getClockManager() {
        return mClockManager;
    }

public:
    virtual int64_t getCurrentClock() {
        return mMediaPlayer->getCurrentClock();
    }

private:
    NBMediaPlayer* mMediaPlayer;
    NBClockManager* mClockManager;
};

struct NBMediaPlayerEvent : public NBTimedEventQueue::Event {
    NBMediaPlayerEvent(
            NBMediaPlayer *player,
            void (NBMediaPlayer::*method)())
            : mPlayer(player),
              mMethod(method) {
    }

protected:
    virtual ~NBMediaPlayerEvent() {}

    virtual void fire(NBTimedEventQueue *queue, int64_t /* now_us */) const {
        (mPlayer->*mMethod)();
    }

private:
    NBMediaPlayer *mPlayer;
    void (NBMediaPlayer::*mMethod)();

    NBMediaPlayerEvent(const NBMediaPlayerEvent &);
    NBMediaPlayerEvent &operator=(const NBMediaPlayerEvent &);
};

NBMediaPlayer::NBMediaPlayer()
:mListener(NULL)
,mAsyncPrepareEvent(NULL)
,mQueueStarted(false)
,mDataSource(NULL)
,mMediaExtractor(NULL)
,mVideoTrack(NULL)
,mAudioTrackIndex(0)
,mActiveAudioTrackIndex(-1)
,mVideoSource(NULL)
,mAudioSource(NULL)
,mVideoOutput(NULL)
,mVideoBuffer(NULL)
,mAudioPlayer(NULL)
,mVideoRenderer(NULL)
,mVideoRendererIsPreview(false)
,mWatchForAudioEOS(true)
,mDurationUs(-1)
,mSeeking(NO_SEEK)
,mFlags(0) {
    
//    download_engine_init(NULL, 0);
    
    mQueue.setQueueName(NBString("NBMediaPlayer"));
    
    mVideoEvent = new NBMediaPlayerEvent(this, &NBMediaPlayer::onVideoEvent);
    mVideoEventPending = false;
    
    mStreamDoneEvent = new NBMediaPlayerEvent(this, &NBMediaPlayer::onStreamDone);
    mStreamDoneEventPending = false;
    
    mCheckAudioStatusEvent = new NBMediaPlayerEvent(
                                                    this, &NBMediaPlayer::onCheckAudioStatus);
    mAudioStatusEventPending = false;
    
    mClockProxy = new NBClockProxy(this);
    
    reset();
}

NBMediaPlayer::~NBMediaPlayer() {
    
    if (mQueueStarted) {
        mQueue.stop();
        mQueueStarted = false;
    }
    
    // release the video event if needed
    reset();
    
//    download_engine_uninit();
    
    if (mVideoEvent != NULL) {
        delete mVideoEvent;
        mVideoEvent = NULL;
    }
    
    if (mStreamDoneEvent != NULL) {
        delete mStreamDoneEvent;
        mStreamDoneEvent = NULL;
    }
    
    if (mCheckAudioStatusEvent != NULL) {
        delete mCheckAudioStatusEvent;
        mCheckAudioStatusEvent = NULL;
    }
    
    if (mClockProxy != NULL) {
        delete mClockProxy;
        mClockProxy = NULL;
    }
}

nb_status_t NBMediaPlayer::prepareAsync() {
    NBAutoMutex _lock(mLock);
    // return imediately if not reset
    if (mFlags ^ 0) {
        return UNKNOWN_ERROR;
    }
    //record we are prepare async
    mIsAsyncPrepare = true;
    return prepareAsync_l();
}

nb_status_t NBMediaPlayer::prepareAsync_l() {
    if (!mQueueStarted) {
        mQueue.start();
        mQueueStarted = true;
    }

    modifyFlags(PREPARING, SET);
    mAsyncPrepareEvent = new NBMediaPlayerEvent(
            this, &NBMediaPlayer::onPrepareAsyncEvent);

    mQueue.postEvent(mAsyncPrepareEvent);

    return OK;
}

nb_status_t NBMediaPlayer::finishSetDataSource_l() {
    NBMediaExtractor* mediaExtractor = NULL;
    mDataSource = NBDataSource::CreateFromURI(mUri);
    if (mDataSource == NULL) {
        return UNKNOWN_ERROR;
    }

    if (mDataSource->initCheck(&mParams) != OK) {
        // Fix data source abort , then send message
        // delete mActiveDataSource;
        // mActiveDataSource = NULL;
        return UNKNOWN_ERROR;
    }

    mediaExtractor = NBMediaExtractor::Create(this, mDataSource, mDataSource->getUri().string());
    if (mediaExtractor == NULL) {
        NBDataSource::Destroy(mDataSource);
        mDataSource = NULL;
        return UNKNOWN_ERROR;
    }

    if (mediaExtractor->sniffMedia() != OK) {
        NBDataSource::Destroy(mDataSource);
        mDataSource = NULL;

        delete mediaExtractor;
        return UNKNOWN_ERROR;
    }
    
    // set the io is connected & format is parsed
    modifyFlags(PREPARING_CONNECTED, SET);

    if (setDataSource_l(mediaExtractor) != OK) {
        NBDataSource::Destroy(mDataSource);
        mDataSource = NULL;

        delete mediaExtractor;
        return UNKNOWN_ERROR;
    }

    return OK;
}

nb_status_t NBMediaPlayer::setDataSource_l(NBDataSource *dataSource) {
    NBMediaExtractor *extractor = NBMediaExtractor::Create(this, dataSource);

    if (extractor == NULL) {
        return UNKNOWN_ERROR;
    }

//        dataSource->getDrmInfo(mDecryptHandle, &mDrmManagerClient);
//        if (mDecryptHandle != NULL) {
//            CHECK(mDrmManagerClient);
//            if (RightsStatus::RIGHTS_VALID != mDecryptHandle->status) {
//                notifyListener_l(MEDIA_ERROR, MEDIA_ERROR_UNKNOWN, ERROR_DRM_NO_LICENSE);
//            }
//        }

    return setDataSource_l(extractor);
}

nb_status_t NBMediaPlayer::setDataSource_l(NBMediaExtractor *extractor) {
    // Attempt to approximate overall stream bitrate by summing all
    // tracks' individual bitrates, if not all of them advertise bitrate,
    // we have to fail.

//    int64_t totalBitRate = 0;
    int32_t frameRate = 0;

//        for (size_t i = 0; i < extractor->countTracks(); ++i) {
//            MetaData * meta = extractor->getTrackMetaData(i);
//
//            int32_t bitrate;
//            if (!meta->findInt32(kKeyBitRate, &bitrate)) {
//                const char *mime;
//                CHECK(meta->findCString(kKeyMIMEType, &mime));
//                LOGV("track of type '%s' does not publish bitrate", mime);
//
//                totalBitRate = -1;
//                break;
//            }
//
//            totalBitRate += bitrate;
//        }
//
//    mBitrate = totalBitRate;
//    {
//        Mutex::Autolock autoLock(mStatsLock);
//        mStats.mBitrate = mBitrate;
//        mStats.mTracks.clear();
//        mStats.mAudioTrackIndex = -1;
//        mStats.mVideoTrackIndex = -1;
//    }

    bool haveAudio = false;
    bool haveVideo = false;
    for (size_t i = 0; i < extractor->countTracks(); ++i) {
        NBMetaData *meta = extractor->getTrackMetaData(i);

        const char *_mime;
        meta->findCString(kKeyMIMEType, &_mime);

        if (_mime == NULL) {
            continue;
        }

        if (!haveVideo && !strncasecmp(_mime, "video/", 6)) {
            setVideoSource(extractor->getTrack(i));
            haveVideo = true;

            // Set the presentation/display size
            int32_t displayWidth = 0;
            int32_t displayHeight = 0;
            bool success = meta->findInt32(kKeyDisplayWidth, &displayWidth);
            if (success) {
                success = meta->findInt32(kKeyDisplayHeight, &displayHeight);
            }
            if (success) {
                mDisplayWidth = displayWidth;
                mDisplayHeight = displayHeight;
            }

            success = meta->findInt32(kKeyFrameRate, &frameRate);
            if(success){
                mVideoFrameRate = frameRate;
            }

            {
//                    Mutex::Autolock autoLock(mStatsLock);
//                    mStats.mVideoTrackIndex = mStats.mTracks.size();
//                    mStats.mTracks.push();
//                    TrackStat *stat =
//                    &mStats.mTracks.editItemAt(mStats.mVideoTrackIndex);
//                    stat->mMIME = mime.string();
            }
        } else if (/*!haveAudio &&*/!strncasecmp(_mime, "audio/", 6)) {
            addAudioSource(extractor->getTrack(i));
            haveAudio = true;
//                {
//                    Mutex::Autolock autoLock(mStatsLock);
//                    mStats.mAudioTrackIndex = mStats.mTracks.size();
//                    mStats.mTracks.push();
//                    TrackStat *stat =
//                    &mStats.mTracks.editItemAt(mStats.mAudioTrackIndex);
//                    stat->mMIME = mime.string();
//                }

//                if (!strcasecmp(mime.string(), MEDIA_MIMETYPE_AUDIO_VORBIS)) {
//                    // Only do this for vorbis audio, none of the other audio
//                    // formats even support this ringtone specific hack and
//                    // retrieving the metadata on some extractors may turn out
//                    // to be very expensive.
//                    sp<MetaData> fileMeta = extractor->getMetaData();
//                    int32_t loop;
//                    if (fileMeta != NULL
//                        && fileMeta->findInt32(kKeyAutoLoop, &loop) && loop != 0) {
//                        modifyFlags(AUTO_LOOPING, SET);
//                    }
//                }
        } else if (!strncasecmp(_mime, "subtitle/", 9)) {
//            addSubtitleSource(extractor->getTrack(i));
        }
//            else if (!strcasecmp(mime.string(), MEDIA_MIMETYPE_TEXT_3GPP)) {
//                addTextSource(extractor->getTrack(i));
//            }
    }

    if (!haveAudio && !haveVideo) {
        return UNKNOWN_ERROR;
    }

//    //Default select the first audio track
//    if (mCurrentAudioTrackIndex == -1
//        && mAudioTracks.size() > 0) {
//        mCurrentAudioTrackIndex = 0;
//    }
//
    mExtractorFlags = extractor->flags();
    mMediaExtractor = extractor;

    return OK;
}

void NBMediaPlayer::addAudioSource(NBMediaSource *source) {
    mAudioTracks[mAudioTrackIndex++] = source;
    mActiveAudioTrackIndex = 0;
}

void NBMediaPlayer::setVideoSource(NBMediaSource *source) {
    mVideoTrack = source;
}

void NBMediaPlayer::onPrepareAsyncEvent() {
    nb_status_t err = OK;
    {
        NBAutoMutex autoLock(mLock);
        if (mFlags & PREPARE_CANCELLED) {
            abortPrepare(UNKNOWN_ERROR);
            return;
        }

        if (mUri.isEmpty()) {
            abortPrepare(UNKNOWN_ERROR);
            return;
        }
    }

    err = finishSetDataSource_l();

    if (err != OK) {
        NBAutoMutex autoLock(mLock);
        abortPrepare(err);
        return;
    }

    if (mVideoTrack != NULL && mVideoSource == NULL) {
        nb_status_t err = initVideoDecoder();

        if (err != OK) {
            abortPrepare(err);
            return;
        }
    }

    if (mAudioTrackIndex >= 0 && mAudioSource == NULL) {
        nb_status_t err = initAudioDecoder();

        if (err != OK) {
            NBAutoMutex autoLock(mLock);
            abortPrepare(err);
            return;
        }
    }

//    err = initVideoRenderer();
//    if (err != OK) {
//        NBAutoMutex autoLock(mLock);
//        abortPrepare(err);
//        return;
//    }
//
//    err = initAudioRenderer();
//    if (err != OK) {
//        NBAutoMutex autoLock(mLock);
//        abortPrepare(err);
//        return;
//    }

//    if (isStreaming()) {
//        postBufferingEvent_l(3000000L);
//        postDownloadEvent_l();
//    }
    {
//        NBAutoMutex autoLock(mLock);
        finishAsyncPrepare_l();
    }
}

nb_status_t NBMediaPlayer::initAudioDecoder() {
    NBMediaSource* audioTrack = mAudioTracks[mActiveAudioTrackIndex];

    NBMetaData *meta = audioTrack->getFormat();

    const char *mime;
    meta->findCString(kKeyMIMEType, &mime);

    if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_RAW)) {
        mAudioSource = audioTrack;
    } else {
//            mAudioSource = OMXCodec::Create(
//                                            mClient.interface(), mAudioTrack->getFormat(),
//                                            false, // createEncoder
//                                            mAudioTrack);
        mAudioSource = NBMediaDecoder::Create(audioTrack->getFormat(), audioTrack, NULL);
    }

    if (mAudioSource != NULL) {
        int64_t durationUs;
        if (audioTrack->getFormat()->findInt64(kKeyDuration, &durationUs)) {
            NBAutoMutex autoLock(mMiscStateLock);
            if (mDurationUs < 0 || durationUs > mDurationUs) {
                mDurationUs = durationUs;
            }
        }
        nb_status_t err = mAudioSource->start();
        if (err != OK) {
//                mAudioSource.clear();
            return err;
        }
    }
//        else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_QCELP)) {
//            // For legacy reasons we're simply going to ignore the absence
//            // of an audio decoder for QCELP instead of aborting playback
//            // altogether.
//            return OK;
//        }

//        if (mAudioSource != NULL) {
//            Mutex::Autolock autoLock(mStatsLock);
//            TrackStat *stat = &mStats.mTracks.editItemAt(mStats.mAudioTrackIndex);
//
//            const char *component;
//            if (!mAudioSource->getFormat()
//                ->findCString(kKeyDecoderComponent, &component)) {
//                component = "none";
//            }
//
//            stat->mDecoderName = component;
//        }
    return mAudioSource != NULL ? OK : UNKNOWN_ERROR;
}

nb_status_t NBMediaPlayer::initVideoDecoder(uint32_t flags) {
    nb_status_t err = UNKNOWN_ERROR;

    NBLOG_INFO(LOG_TAG, "mVideoTrack is : %p", mVideoTrack);
    
    int64_t durationUs;
    if (mVideoTrack->getFormat()->findInt64(kKeyDuration, &durationUs)) {
        NBAutoMutex autoLock(mMiscStateLock);
        if (mDurationUs < 0 || durationUs > mDurationUs) {
            mDurationUs = durationUs;
        }
    }

    NBLOG_INFO(LOG_TAG, "mVideoSource is : %p", mVideoSource);

    // videooutput params is surface
    mVideoSource = NBMediaDecoder::Create(mVideoTrack->getFormat(), mVideoTrack, mVideoOutput->params);
    if (mVideoSource != NULL) {
        err = mVideoSource->start();
    }

    NBLOG_INFO(LOG_TAG, "mVideoSource is : %p err : %d", mVideoSource, err);
    
    if (err != OK) {
        const char* decodeComponent = NULL;
        mVideoSource->getFormat()->findCString(kKeyDecoderComponent, &decodeComponent);
        if (decodeComponent != NULL && strcmp(decodeComponent, MEDIA_DECODE_COMPONENT_FFMPEG) == 0) {
            delete mVideoSource;
            mVideoSource = NULL;
            return err;
        }
        
        //delete prev memory first
        delete mVideoSource;
        mVideoSource = NULL;
        
        //fall back to software decoder
        mVideoSource = NBMediaDecoder::Create(mVideoTrack->getFormat(), mVideoTrack, NULL, NBMediaDecoder::NB_DECODER_FLAG_FORCE_SOFTWARE);
        if (mVideoSource != NULL) {
            err = mVideoSource->start();
        }
        
        //realy create decode error
        if (err != OK) {
            delete mVideoSource;
            mVideoSource = NULL;
            return err;
        }
    }

    return mVideoSource != NULL ? OK : UNKNOWN_ERROR;
}

void NBMediaPlayer::finishSeekIfNecessary(int64_t videoTimeUs) {
    if (mSeeking == SEEK_VIDEO_ONLY) {
        mSeeking = NO_SEEK;
        return;
    }

    if (mSeeking == NO_SEEK || (mFlags & SEEK_PREVIEW)) {
        return;
    }

//    // If we paused, then seeked, then resumed, it is possible that we have
//    // signaled SEEK_COMPLETE at a copmletely different media time than where
//    // we are now resuming.  Signal new position to media time provider.
//    // Cannot signal another SEEK_COMPLETE, as existing clients may not expect
//    // multiple SEEK_COMPLETE responses to a single seek() request.
//    if (mSeekNotificationSent && abs(mSeekTimeUs - videoTimeUs) > 10000) {
//        // notify if we are resuming more than 10ms away from desired seek time
//        notifyListener_l(MEDIA_SKIPPED);
//    }

    if (mAudioPlayer != NULL) {
        NBLOG_INFO(LOG_TAG, "seeking audio to %lld us (%.2f secs).", videoTimeUs, videoTimeUs / 1E6);

        // If we don't have a video time, seek audio to the originally
        // requested seek time instead.

        mAudioPlayer->seekTo(videoTimeUs < 0 ? mSeekTimeUs : videoTimeUs);
        mWatchForAudioSeekComplete = true;
        mWatchForAudioEOS = true;
    } else if (!mSeekNotificationSent) {
        // If we're playing video only, report seek complete now,
        // otherwise audio player will notify us later.
        notifyListener_l(MEDIA_SEEK_COMPLETE);
        mSeekNotificationSent = true;
    }

    modifyFlags(FIRST_FRAME, SET);
    mSeeking = NO_SEEK;

//    if (mDecryptHandle != NULL) {
//        mDrmManagerClient->setPlaybackStatus(mDecryptHandle,
//                                                Playback::PAUSE, 0);
//        mDrmManagerClient->setPlaybackStatus(mDecryptHandle,
//                        Playback::START, videoTimeUs / 1000);
//    }
}

nb_status_t NBMediaPlayer::createAudioPlayer_l() {
    mAudioPlayer = new NBAudioPlayer(this, mClockProxy->getClockManager());
    if (mAudioPlayer == NULL) {
        return UNKNOWN_ERROR;
    }

    mAudioPlayer->setAudioSource(mAudioSource);

//    if (mAudioPlayer->prepare() != OK) {
//        delete mAudioPlayer;
//        return UNKNOWN_ERROR;
//    }

    return OK;
}

nb_status_t NBMediaPlayer::startAudioPlayer_l() {
    nb_status_t err = OK;

    if (mAudioSource == NULL || mAudioPlayer == NULL) {
        return OK;
    }

//    if (mOffloadAudio) {
//        mQueue.cancelEvent(mAudioTearDownEvent->eventID());
//        mAudioTearDownEventPending = false;
//    }

    if (!(mFlags & AUDIOPLAYER_STARTED)) {
        // We've already started the MediaSource in order to enable
        // the prefetcher to read its data.
        err = mAudioPlayer->start(true /* sourceAlreadyStarted */);

        if (err != OK) {
//            if (sendErrorNotification) {
//                notifyListener_l(MEDIA_ERROR, MEDIA_ERROR_UNKNOWN, err);
//            }
            return err;
        }

        modifyFlags(AUDIOPLAYER_STARTED, SET);

//        if (wasSeeking) {
//            CHECK(!mAudioPlayer->isSeeking());
//
//            // We will have finished the seek while starting the audio player.
//            postAudioSeekComplete();
//        } else {
//            notifyIfMediaStarted_l();
//        }
    } else {
        err = mAudioPlayer->resume();
    }

    if (err == OK) {
        modifyFlags(AUDIO_RUNNING, SET);

        mWatchForAudioEOS = true;
    }
    return err;
}

nb_status_t NBMediaPlayer::initVideoRenderer_l() {
    if (mVideoSource != NULL) {
        const char* decodeComponent = NULL;
        
        NBMetaData* metaData = mVideoSource->getFormat();
        
        if (metaData->findCString(kKeyDecoderComponent, &decodeComponent)) {

            NBLOG_INFO(LOG_TAG, "The decodeComponent is : %s", decodeComponent);

#ifdef BUILD_TARGET_IOS
            if (strcmp(decodeComponent, MEDIA_DECODE_COMPONENT_VIDEOTOOLBOX) == 0) {
                // For headware support
                mVideoRenderer = new NBCVOpenGLESRenderer();
            } else
#elif BUILD_TARGET_ANDROID
            if (strcmp(decodeComponent, MEDIA_DECODE_COMPONENT_MEDIACODEC) == 0) {
                // for media codec video render
                mVideoRenderer = new NBMediaCodecVRenderer();
            } else
#endif
            if (strcmp(decodeComponent, MEDIA_DECODE_COMPONENT_FFMPEG) == 0) {
                mVideoRenderer = new NBGLYUV420PRenderer();
            } else {
                NBLOG_ERROR(LOG_TAG, "unsupported decode component error : %s use yuv420prenderer", decodeComponent);
                mVideoRenderer = new NBGLYUV420PRenderer();
            }
        } else {
            //default use yuv420p render
            mVideoRenderer = new NBGLYUV420PRenderer();
        }

        mVideoRenderer->setVideoOutput(mVideoOutput);

        //setup the opengl context
        mVideoRenderer->start();
    }
    return OK;
}

void NBMediaPlayer::finishAsyncPrepare_l() {

    {
        NBAutoMutex autoMutex(mLock);

        modifyFlags((PREPARING | PREPARE_CANCELLED | PREPARING_CONNECTED), CLEAR);
        modifyFlags(PREPARED, SET);
    }

    if (mIsAsyncPrepare) {
        if (mVideoSource == NULL) {
            notifyListener_l(MEDIA_SET_VIDEO_SIZE, 0, 0);
        } else {
            notifyVideoSize_l();
        }

        notifyListener_l(MEDIA_PREPARED);
    }

//    // start the media extractor
//    if (mMediaExtractor != NULL) {
//        mMediaExtractor->start(NULL);
//    }
    
    {
        NBAutoMutex autoMutex(mLock);

        mPrepareResult = OK;

        delete mAsyncPrepareEvent;
        mAsyncPrepareEvent = NULL;
        mPreparedCondition.broadcast();
    }
}

void NBMediaPlayer::abortPrepare(nb_status_t err) {

    if (mIsAsyncPrepare && (mDataSource != NULL && !mDataSource->isUserInterrupted())) {
        notifyListener_l(MEDIA_ERROR, MEDIA_ERROR_UNKNOWN, err);
    }

    mPrepareResult = err;
    modifyFlags((PREPARING|PREPARE_CANCELLED|PREPARING_CONNECTED), CLEAR);

    delete mAsyncPrepareEvent;
    mAsyncPrepareEvent = NULL;
    mPreparedCondition.broadcast();
}

void NBMediaPlayer::onVideoEvent() {
    NBAutoMutex autoLock(mLock);
    if (!mVideoEventPending) {
        // The event has been cancelled in reset_l() but had already
        // been scheduled for execution at that time.
        return;
    }
    mVideoEventPending = false;
    
    if (mSeeking != NO_SEEK) {
        //Free current data
        if (mVideoBuffer) {
            delete mVideoBuffer;
            mVideoBuffer = NULL;
        }
    }

    if (!mVideoBuffer) {

        NBMediaSource::ReadOptions options;

        if (mSeeking != NO_SEEK) {
            options.setSeekTo(
                                mSeekTimeUs,
                                mSeeking == SEEK_VIDEO_ONLY
                                    ? NBMediaSource::ReadOptions::SEEK_NEXT_SYNC
                                    : NBMediaSource::ReadOptions::SEEK_CLOSEST_SYNC);
        }
        
//        NBLOG_INFO(LOG_TAG, "Read data begin");
        for (;;) {
            nb_status_t err = mVideoSource->read(&mVideoBuffer, &options);
            
//            NBLOG_INFO(LOG_TAG, "video stream read result : %d", err);

            if (err != OK) {
                if (err == INFO_FORMAT_CHANGED) {
//                    notifyVideoSize_l();
//
//                        if (mVideoRenderer != NULL) {
//                            mVideoRendererIsPreview = false;
//                            initRenderer_l();
//                        }
                    continue;
                }

                // So video playback is complete, but we may still have
                // a seek request pending that needs to be applied
                // to the audio track.
//                if (mSeeking != NO_SEEK  && mSeeking != SEEK_ACTIONED) {
//                    if (mAudioPlayer != NULL) {
//                        mAudioPlayer->flushContext();
//                    }
//                }
                if (err != ERROR_IO) {
//                    finishSeekIfNecessary(-1);
//
//                    if (mAudioPlayer != NULL
//                        && !(mFlags & (AUDIO_RUNNING | SEEK_PREVIEW))) {
//                        startAudioPlayer_l();
//                    }
                }

                if (err != ERROR_END_OF_STREAM) {
                    //Fix me , decode error
//                        INFO("postVideoEvent_l ERROR_END_OF_STREAM");
                    postVideoEvent_l(5000);
                }else {
                    // So video playback is complete, but we may still have
                    // a seek request pending that needs to be applied
                    // to the audio track.
                    finishSeekIfNecessary(-1);
                    
                    if (mAudioPlayer != NULL
                        && !(mFlags & (AUDIO_RUNNING | SEEK_PREVIEW))) {
                        startAudioPlayer_l();
                    }
                    
                    modifyFlags(VIDEO_AT_EOS, SET);
                    NBLOG_INFO(LOG_TAG, "postVideoEvent_l modifyFlags(VIDEO_AT_EOS, SET)\n");
                    postStreamDoneEvent_l(err);
                }
                return;
            }

            if (mVideoBuffer == NULL) {
                postVideoEvent_l();
                return ;
            }

//                if (mVideoBuffer->range_length() == 0) {
//                    // Some decoders, notably the PV AVC software decoder
//                    // return spurious empty buffers that we just want to ignore.
//
//                    mVideoBuffer->release();
//                    mVideoBuffer = NULL;
//                    continue;
//                }

            break;
        }
    }

    AVFrame* videoFrame = (AVFrame*)mVideoBuffer->data();
    int64_t timeUs = videoFrame->pts;
    
//    NBLOG_INFO(LOG_TAG, "Read data end pts : %lld", timeUs);

    if (timeUs == AV_NOPTS_VALUE) {
        delete mVideoBuffer;
        mVideoBuffer = NULL;
        
        mLastVideoTimeUs = timeUs;
        
        postVideoEvent_l(10000);
        return ;
    }

    if (mAudioPlayer != NULL && !(mFlags & (AUDIO_RUNNING | SEEK_PREVIEW))) {
        nb_status_t err = startAudioPlayer_l();
        if (err != OK) {
            return ;
        }
    }

    {
        NBAutoMutex autoLock(mMiscStateLock);
        mVideoTimeUs = timeUs;
//        NBLOG_INFO(LOG_TAG, "The video current timeUs is : %lld", mVideoTimeUs);
    }

    SeekType wasSeeking = mSeeking;
    finishSeekIfNecessary(timeUs);

    if (wasSeeking == NO_SEEK) {
        if (mAudioPlayer != NULL) {
            int64_t nowUs = mClockProxy->getClockManager()->getMasterClock();

            int64_t latenessUs = nowUs - timeUs;

            if (latenessUs > 40) {
                mVideoSource->discardNonRef();
            }
            
            // We're more than 125ms late.
            if (latenessUs > 125) {
                delete mVideoBuffer;
                mVideoBuffer = NULL;

                //We need decode right now
                postVideoEvent_l(0);
                return;
            }

            if (latenessUs < -40) {
                if (latenessUs > 4) {
                    mVideoSource->restoreDiscard();
                }
                
                // We're more than 10ms early.
                postVideoEvent_l(latenessUs* 1000);
                return;
            }
        }
    }

    if ((mVideoOutput != NULL)
        && (mVideoRendererIsPreview || mVideoRenderer == NULL)) {
        mVideoRendererIsPreview = false;
        initVideoRenderer_l();
    }

    if (mVideoRenderer != NULL) {
        if (mVideoRenderer != NULL) {
            mVideoRenderer->displayFrame(mVideoBuffer);
        }
    }

    delete mVideoBuffer;
    mVideoBuffer = NULL;

    mLastVideoTimeUs = timeUs;

    postVideoEvent_l(0);
}

void NBMediaPlayer::postVideoEvent_l(int64_t delayUs) {
    if (mVideoEventPending) {
        return;
    }

    mVideoEventPending = true;
    mQueue.postEventWithDelay(mVideoEvent, delayUs < 0 ? 10000 : delayUs);
}

void NBMediaPlayer::onStreamDone() {
    // Posted whenever any stream finishes playing.

    NBAutoMutex autoLock(mLock);
    if (!mStreamDoneEventPending) {
        return;
    }
    mStreamDoneEventPending = false;

    NBLOG_DEBUG(LOG_TAG, "The video stream done\n");

    if (mStreamDoneStatus != ERROR_END_OF_STREAM) {
        notifyListener_l(
                MEDIA_ERROR, MEDIA_ERROR_UNKNOWN, mStreamDoneStatus);

        pause_l(true /* at eos */);

        modifyFlags(AT_EOS, SET);
        return;
    }

    const bool allDone =
            (mVideoSource == NULL || (mFlags & VIDEO_AT_EOS))
            && (mAudioSource == NULL || (mFlags & AUDIO_AT_EOS));

    NBLOG_DEBUG(LOG_TAG, "video stream done\n");

    if (!allDone) {
        NBLOG_INFO(LOG_TAG, "not all stream is done\n");
        return;
    }

    if ((mFlags & LOOPING)
        /*|| ((mFlags & AUTO_LOOPING)
         && (mAudioSink == NULL || mAudioSink->realtime()))*/) {
        // Don't AUTO_LOOP if we're being recorded, since that cannot be
        // turned off and recording would go on indefinitely.
        seekTo_l(0);

        if (mVideoSource != NULL) {
            postVideoEvent_l();
        }
    } else {
        notifyListener_l(MEDIA_PLAYBACK_COMPLETE);

        pause_l(true /* at eos */);

        modifyFlags(AT_EOS, SET);
    }
}

void NBMediaPlayer::postStreamDoneEvent_l(nb_status_t status) {
    if (mStreamDoneEventPending) {
        return;
    }

    mStreamDoneEventPending = true;

    mStreamDoneStatus = status;
    mQueue.postEvent(mStreamDoneEvent);
}

// audio status
void NBMediaPlayer::onCheckAudioStatus() {
    {
        NBAutoMutex autoLock(mAudioLock);
        if (!mAudioStatusEventPending) {
            // Event was dispatched and while we were blocking on the mutex,
            // has already been cancelled.
            return;
        }

        mAudioStatusEventPending = false;
    }

    NBAutoMutex autoLock(mLock);

//    if (mWatchForAudioSeekComplete && !mAudioPlayer->isSeeking()) {
//        mWatchForAudioSeekComplete = false;
//
//        if (!mSeekNotificationSent) {
//            notifyListener_l(MEDIA_SEEK_COMPLETE);
//            mSeekNotificationSent = true;
//        }
//
//        mSeeking = NO_SEEK;
//        notifyIfMediaStarted_l();
//    }

    nb_status_t finalStatus;
    if (mWatchForAudioEOS && mAudioPlayer->reachedEOS(&finalStatus)) {
        mWatchForAudioEOS = false;
        modifyFlags(AUDIO_AT_EOS, SET);
        modifyFlags(FIRST_FRAME, SET);
        
        NBLOG_INFO(LOG_TAG, "The audio stream is done");
        
        postStreamDoneEvent_l(finalStatus);
    }
}

void NBMediaPlayer::postCheckAudioStatusEvent(int64_t delayUs) {
    NBAutoMutex autoLock(mAudioLock);
    if (mAudioStatusEventPending) {
        return;
    }
    mAudioStatusEventPending = true;
    mQueue.postEventWithDelay(mCheckAudioStatusEvent, delayUs);
}

void NBMediaPlayer::postAudioEOS(int64_t delayUs) {
    postCheckAudioStatusEvent(delayUs);
}

nb_status_t NBMediaPlayer::setDataSource(const char* uri, const NBMap<NBString, NBString>* params) {
    NBAutoMutex _lock(mLock);
    return setDataSource_l(uri, params);
}

nb_status_t NBMediaPlayer::setDataSource_l(const char *uri, const NBMap<NBString, NBString>* params) {
    reset_l();
    
    mUri = uri;

    if (params) {
        mParams = *params;
        
//        ssize_t index = mUriHeaders.indexOfKey(String8("x-hide-urls-from-log"));
//        if (index >= 0) {
//            // Browser is in "incognito" mode, suppress logging URLs.
//
//            // This isn't something that should be passed to the server.
//            mUriHeaders.removeItemsAt(index);
//
//            modifyFlags(INCOGNITO, SET);
//        }
    }
    
//    ALOGI("setDataSource_l(URL suppressed)");
//
//    // The actual work will be done during preparation in the call to
//    // ::finishSetDataSource_l to avoid blocking the calling thread in
//    // setDataSource for any significant time.
//
//    {
//        Mutex::Autolock autoLock(mStatsLock);
//        mStats.mFd = -1;
//        mStats.mURI = mUri;
//    }

    return OK;
}

nb_status_t NBMediaPlayer::play() {
    NBAutoMutex autoLock(mLock);

    modifyFlags(CACHE_UNDERRUN, CLEAR);

    return play_l();
}

nb_status_t NBMediaPlayer::play_l() {
    nb_status_t err = OK;

    modifyFlags(SEEK_PREVIEW, CLEAR);

    if (mFlags & PLAYING) {
        return OK;
    }

    modifyFlags(PLAYING, SET);
    modifyFlags(FIRST_FRAME, SET);

    if (mAudioSource != NULL) {
        if (mAudioPlayer == NULL) {
            err = createAudioPlayer_l();
        }

        // start audio play if video source
        if (mVideoSource == NULL) {
            err = startAudioPlayer_l();
        }

        //return if play audio error
        if (err != OK) {
            delete mAudioPlayer;
            mAudioPlayer = NULL;

            modifyFlags((PLAYING | FIRST_FRAME), CLEAR);
            return err;
        }
    }

    if (mVideoSource != NULL) {
        // Kick off video playback
        postVideoEvent_l();

//        if (mAudioSource != NULL && mVideoSource != NULL) {
//            postVideoLagEvent_l();
//        }
    }

    if (mFlags & AT_EOS) {
        // Legacy behaviour, if a stream finishes playing and then
        // is started again, we play from the start...
        seekTo_l(0);
    }

    return OK;
}

nb_status_t NBMediaPlayer::seekTo(int64_t millisec) {
    if (mExtractorFlags & NBMediaExtractor::CAN_SEEK) {
        NBAutoMutex autoMutex(mLock);
        
        NBLOG_DEBUG(LOG_TAG, "We seek to target : %lld", millisec);
        
        return seekTo_l(millisec);
    }
    return OK;
}

nb_status_t NBMediaPlayer::seekTo_l(int64_t millisec) {
    if (mFlags & CACHE_UNDERRUN) {
        modifyFlags(CACHE_UNDERRUN, CLEAR);
        modifyFlags(PLAYING, SET);
    }

    cancelPlayerEvents(true);

    if (mAudioPlayer != NULL && (mFlags & AUDIO_RUNNING)) {

        NBLOG_DEBUG(LOG_TAG, "We audio player is playing");

        // If we played the audio stream to completion we
        // want to make sure that all samples remaining in the audio
        // track's queue are played out.
        mAudioPlayer->pause(false /* playPendingSamples */);
        // send us a reminder to tear down the AudioPlayer if paused for too long.
//        if (mOffloadAudio) {
//            postAudioTearDownEvent(kOffloadPauseMaxUs);
//        }
        modifyFlags(AUDIO_RUNNING, CLEAR);
    }

    mSeeking = SEEK;
    mSeekNotificationSent = false;

    //mutiple the timescale
//    millisec *= 1000;

    return mMediaExtractor->seekTo(millisec);
}

nb_status_t NBMediaPlayer::pause() {
    NBAutoMutex autoLock(mLock);

    modifyFlags(CACHE_UNDERRUN, CLEAR);

    return pause_l();
}

nb_status_t NBMediaPlayer::pause_l(bool at_eos) {
//    if (!(mFlags & PLAYING)) {
//        if (mAudioTearDown && mAudioTearDownWasPlaying) {
//            mAudioTearDownWasPlaying = false;
//            notifyListener_l(MEDIA_PAUSED);
//            mMediaRenderingStartGeneration = ++mStartGeneration;
//            }
//        return OK;
//    }
//
//    notifyListener_l(MEDIA_PAUSED);
//    mMediaRenderingStartGeneration = ++mStartGeneration;

    cancelPlayerEvents(true /* keepNotifications */);

    if (mAudioPlayer != NULL && (mFlags & AUDIO_RUNNING)) {
        // If we played the audio stream to completion we
        // want to make sure that all samples remaining in the audio
        // track's queue are played out.
        mAudioPlayer->pause(at_eos /* playPendingSamples */);
        // send us a reminder to tear down the AudioPlayer if paused for too long.
//        if (mOffloadAudio) {
//            postAudioTearDownEvent(kOffloadPauseMaxUs);
//        }
        modifyFlags(AUDIO_RUNNING, CLEAR);
    }

//    if (mFlags & TEXTPLAYER_INITIALIZED) {
//        mTextDriver->pause();
//        modifyFlags(TEXT_RUNNING, CLEAR);
//    }

    modifyFlags(PLAYING, CLEAR);

//    if (mDecryptHandle != NULL) {
//        mDrmManagerClient->setPlaybackStatus(mDecryptHandle,
//                                      Playback::PAUSE, 0);
//    }
//
//    uint32_t params = IMediaPlayerService::kBatteryDataTrackDecoder;
//    if ((mAudioSource != NULL) && (mAudioSource != mAudioTrack)) {
//        params |= IMediaPlayerService::kBatteryDataTrackAudio;
//    }
//
//    if (mVideoSource != NULL) {
//        params |= IMediaPlayerService::kBatteryDataTrackVideo;
//    }
//
//    addBatteryData(params);

    return OK;
}

void NBMediaPlayer::cancelPlayerEvents(bool keepNotifications) {
    mQueue.cancelEvent(mVideoEvent->eventID());
    mVideoEventPending = false;
//    mQueue.cancelEvent(mVideoLagEvent->eventID());
//    mVideoLagEventPending = false;
//
//    if (mOffloadAudio) {
//        mQueue.cancelEvent(mAudioTearDownEvent->eventID());
//        mAudioTearDownEventPending = false;
//    }

    if (!keepNotifications) {
        mQueue.cancelEvent(mStreamDoneEvent->eventID());
        mStreamDoneEventPending = false;
        mQueue.cancelEvent(mCheckAudioStatusEvent->eventID());
        mAudioStatusEventPending = false;

//        mQueue.cancelEvent(mBufferingEvent->eventID());
//        mBufferingEventPending = false;
//        mAudioTearDown = false;
    }
}

void NBMediaPlayer::onMediaSeeked(int64_t pos, nb_status_t status) {
    NBAutoMutex autoMutex(mLock);
    
    if ((mFlags & PLAYING) && mVideoSource != NULL) {
        // Video playback completed before, there's no pending
        // video event right now. In order for this new seek
        // to be honored, we need to post one.

        postVideoEvent_l();
    }

    modifyFlags((AT_EOS | AUDIO_AT_EOS | VIDEO_AT_EOS), CLEAR);
    
    NBLOG_INFO(LOG_TAG, "The audio stream is cleared");

//    if (mFlags & PLAYING) {
//        notifyListener_l(MEDIA_PAUSED);
//        mMediaRenderingStartGeneration = ++mStartGeneration;
//    }
//
//    if (mFlags & TEXTPLAYER_INITIALIZED) {
//        mTextDriver->seekToAsync(mSeekTimeUs);
//    }

    if (!(mFlags & PLAYING)) {
        NBLOG_DEBUG(LOG_TAG, "seeking while paused, sending SEEK_COMPLETE notification"
                   " immediately.");

        notifyListener_l(MEDIA_SEEK_COMPLETE);
        mSeekNotificationSent = true;

        if ((mFlags & PREPARED) && mVideoSource != NULL) {
            modifyFlags(SEEK_PREVIEW, SET);
            postVideoEvent_l();
        }
    }

}

void NBMediaPlayer::reset() {
    NBAutoMutex autoLock(mLock);
    reset_l();
}

void NBMediaPlayer::stop() {
    NBAutoMutex autoLock(mLock);
    reset_l();
//    if (mQueueStarted) {
//        mQueue.stop();
//        mQueueStarted = false;
//    }
}

nb_status_t NBMediaPlayer::getVideoWidth(int32_t *vWidth) {
    NBAutoMutex autoLock(mStatsLock);
    if (vWidth == NULL) {
        return UNKNOWN_ERROR;
    }
    *vWidth = mStats.mVideoWidth;
    return OK;
}

nb_status_t NBMediaPlayer::getVideoHeight(int32_t *vHeight) {
    NBAutoMutex autoLock(mStatsLock);
    if (vHeight == NULL) {
        return UNKNOWN_ERROR;
    }
    *vHeight = mStats.mVideoHeight;
    return OK;
}

void NBMediaPlayer::reset_l() {
    mVideoRenderingStarted = false;
//    mActiveAudioTrackIndex = 0;
    mAudioTrackIndex = 0;
    mDisplayWidth = 0;
    mDisplayHeight = 0;

//    notifyListener_l(MEDIA_STOPPED);
//
//    if (mDecryptHandle != NULL) {
//        mDrmManagerClient->setPlaybackStatus(mDecryptHandle,
//                            Playback::STOP, 0);
//        mDecryptHandle = NULL;
//        mDrmManagerClient = NULL;
//    }

//    if (mFlags & PLAYING) {
//        uint32_t params = IMediaPlayerService::kBatteryDataTrackDecoder;
//        if ((mAudioSource != NULL) && (mAudioSource != mAudioTrack)) {
//            params |= IMediaPlayerService::kBatteryDataTrackAudio;
//        }
//        if (mVideoSource != NULL) {
//            params |= IMediaPlayerService::kBatteryDataTrackVideo;
//        }
//        addBatteryData(params);
//    }

    if (mFlags & PREPARING) {
        modifyFlags(PREPARE_CANCELLED, SET);
        if (mDataSource != NULL) {
            NBLOG_DEBUG(LOG_TAG, "interrupting the connection data source");
            mDataSource->setUserInterrupted(true);
        }

        if (mFlags & PREPARING_CONNECTED) {
            // We are basically done preparing, we're just buffering
            // enough data to start playback, we can safely interrupt that.
            finishAsyncPrepare_l();
        }
    }

    while (mFlags & PREPARING) {
        mPreparedCondition.wait(mLock);
    }

    cancelPlayerEvents();
    
    if (mAudioPlayer != NULL) {
        mAudioPlayer->stop();
        delete mAudioPlayer;
        mAudioPlayer = NULL;
    }

    // Shutdown audio first, so that the response to the reset request
    // appears to happen instantaneously as far as the user is concerned
    // If we did this later, audio would continue playing while we
    // shutdown the video-related resources and the player appear to
    // not be as responsive to a reset request.
    if ((mAudioPlayer == NULL || !(mFlags & AUDIOPLAYER_STARTED))
        && mAudioSource != NULL) {
        // If we had an audio player, it would have effectively
        // taken possession of the audio source and stopped it when
        // _it_ is stopped. Otherwise this is still our responsibility.
        mAudioSource->stop();
    }
    
    if (mAudioSource != NULL) {
        delete mAudioSource;
        mAudioSource = NULL;
    }
    
    if (mActiveAudioTrackIndex >= 0) {
        delete mAudioTracks[mActiveAudioTrackIndex];
        mAudioTracks[mActiveAudioTrackIndex] = NULL;
        mActiveAudioTrackIndex = -1;
    }

//    if (mTextDriver != NULL) {
//        delete mTextDriver;
//        mTextDriver = NULL;
//    }

    if (mVideoRenderer != NULL) {
        mVideoRenderer->stop();
        delete mVideoRenderer;
        mVideoRenderer = NULL;
    }


    if (mVideoSource != NULL) {
        shutdownVideoDecoder_l();
    }
    
    if (mVideoTrack != NULL) {
        delete mVideoTrack;
        mVideoTrack = NULL;
    }
    
    if (mMediaExtractor != NULL) {
        // interrupt current read loop
        mDataSource->setUserInterrupted(true);
        
         mMediaExtractor->stop();
        delete mMediaExtractor;
        mMediaExtractor = NULL;
    }
    
    if (mDataSource != NULL) {
        delete mDataSource;
        mDataSource = NULL;
    }

    mDurationUs = -1;
    modifyFlags(0, ASSIGN);
    mExtractorFlags = 0;
//    mTimeSourceDeltaUs = 0;
    mVideoTimeUs = 0;

    mSeeking = NO_SEEK;
    mSeekNotificationSent = true;
    mSeekTimeUs = 0;

    mUri.setTo("");

//    mUriHeaders.clear();

//    mFileSource.clear();

    mBitrate = -1;
    mLastVideoTimeUs = -1;

    {
        NBAutoMutex autoLock(mStatsLock);
        mStats.mFd = -1;
        mStats.mURI = NBString();
        mStats.mBitrate = -1;
        mStats.mAudioTrackIndex = -1;
        mStats.mVideoTrackIndex = -1;
        mStats.mNumVideoFramesDecoded = 0;
        mStats.mNumVideoFramesDropped = 0;
        mStats.mVideoWidth = -1;
        mStats.mVideoHeight = -1;
        mStats.mFlags = 0;
//        mStats.mTracks.clear();
    }

    mWatchForAudioSeekComplete = false;
    mWatchForAudioEOS = false;
}

void NBMediaPlayer::shutdownVideoDecoder_l() {
    if (mVideoBuffer) {
        delete mVideoBuffer;
        mVideoBuffer = NULL;
    }

    if (mVideoSource != NULL) {
        mVideoSource->stop();
        delete mVideoSource;
        mVideoSource = NULL;
    }

    // The following hack is necessary to ensure that the OMX
    // component is completely released by the time we may try
    // to instantiate it again.
//    wp<MediaSource> tmp = mVideoSource;
//    mVideoSource.clear();
//    while (tmp.promote() != NULL) {
//        usleep(1000);
//    }
//    IPCThreadState::self()->flushCommands();
    NBLOG_INFO(LOG_TAG, "video decoder shutdown completed\n");
}

nb_status_t NBMediaPlayer::setListener(INBMediaPlayerLister* listener) {
    NBAutoMutex _lock(mLock);
    mListener = listener;
    return OK;
}

nb_status_t NBMediaPlayer::setVideoOutput(NBRendererTarget* vo) {
    NBAutoMutex _lock(mLock);
    mVideoOutput = vo;
    return OK;
}

bool NBMediaPlayer::isPlaying() const {
    return (mFlags & PLAYING) || (mFlags & CACHE_UNDERRUN);
}

nb_status_t NBMediaPlayer::getDuration(int64_t *durationUs) {
    NBAutoMutex autoLock(mMiscStateLock);

    if (mDurationUs < 0) {
        return UNKNOWN_ERROR;
    }

    *durationUs = mDurationUs;

    return OK;
}

nb_status_t NBMediaPlayer::getCurrentPosition(int64_t *positionUs) {
    if (mSeeking != NO_SEEK) {
        *positionUs = mMediaExtractor->getSeekTimeUs();
    } else if (mVideoSource != NULL
                && (mAudioPlayer == NULL || !(mFlags & VIDEO_AT_EOS))) {
        NBAutoMutex autoLock(mMiscStateLock);
        *positionUs = mVideoTimeUs;
    } else if (mAudioPlayer != NULL) {
        *positionUs = mAudioPlayer->getMediaTimeUs();
    } else {
        *positionUs = 0;
    }
    return OK;
}

void NBMediaPlayer::modifyFlags(unsigned value, FlagMode mode) {
    switch (mode) {
        case SET:
            mFlags |= value;
            break;
        case CLEAR:
            mFlags &= ~value;
            break;
        case ASSIGN:
            mFlags = value;
            break;
        default:
            break;
    }

    {
        NBAutoMutex autoLock(mStatsLock);
        mStats.mFlags = mFlags;
    }
}

void NBMediaPlayer::notifyListener_l(int msg, int ext1, int ext2) {
    if (mListener != NULL) {
        mListener->sendEvent(msg, ext1, ext2);
    }
}

void NBMediaPlayer::notifyVideoSize_l() {
    NBMetaData *meta = mVideoSource->getFormat();

    int32_t cropLeft, cropTop, cropRight, cropBottom;
    if (!meta->findRect(
            kKeyCropRect, &cropLeft, &cropTop, &cropRight, &cropBottom)) {
        int32_t width = 0;
        int32_t height = 0;
        meta->findInt32(kKeyWidth, &width);
        meta->findInt32(kKeyHeight, &height);

        cropLeft = cropTop = 0;
        cropRight = width - 1;
        cropBottom = height - 1;

        NBLOG_DEBUG(LOG_TAG, "got dimensions only %d x %d", width, height);
    } else {
        NBLOG_DEBUG(LOG_TAG, "got crop rect %d, %d, %d, %d",
               cropLeft, cropTop, cropRight, cropBottom);
    }

    int32_t displayWidth;
    if (meta->findInt32(kKeyDisplayWidth, &displayWidth)) {
        NBLOG_DEBUG(LOG_TAG, "Display width changed (%d=>%d)", mDisplayWidth, displayWidth);
        mDisplayWidth = displayWidth;
    }

    int32_t displayHeight;
    if (meta->findInt32(kKeyDisplayHeight, &displayHeight)) {
        NBLOG_DEBUG(LOG_TAG, "Display height changed (%d=>%d)", mDisplayHeight, displayHeight);
        mDisplayHeight = displayHeight;
    }

    int32_t usableWidth = cropRight - cropLeft + 1;
    int32_t usableHeight = cropBottom - cropTop + 1;
    if (mDisplayWidth != 0) {
        usableWidth = mDisplayWidth;
    }

    if (mDisplayHeight != 0) {
        usableHeight = mDisplayHeight;
    }

    int32_t rotationDegrees;
    if (!mVideoTrack->getFormat()->findInt32(
            kKeyRotation, &rotationDegrees)) {
        rotationDegrees = 0;
    }

    {
        NBAutoMutex autoLock(mStatsLock);
        mStats.mVideoWidth = usableWidth;
        mStats.mVideoHeight = usableHeight;
    }
    
    if (rotationDegrees == 90 || rotationDegrees == 270) {
        notifyListener_l(
                MEDIA_SET_VIDEO_SIZE, usableHeight, usableWidth);
    } else {
        notifyListener_l(
                MEDIA_SET_VIDEO_SIZE, usableWidth, usableHeight);
    }
}

int64_t NBMediaPlayer::getCurrentClock() {
    return mVideoTimeUs;
}
