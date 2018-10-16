//
// Created by parallels on 9/7/18.
//

#ifndef NBMEDIAPLAYER_H
#define NBMEDIAPLAYER_H

#include <NBMacros.h>
#include <NBString.h>
#include <NBMap.h>
#include <NBMutex.h>
#include <NBTimedEventQueue.h>
#include <NBRendererInfo.h>

class NBDataSource;
class NBMediaSource;
class NBAudioPlayer;
class NBVideoRenderer;
class NBMediaExtractor;
class NBMediaBuffer;

#define MAX_AUDIO_TRACK_COUNT   16

class NBClockProxy;

enum media_event_type {
    MEDIA_NOP               = 0, // interface test message
    MEDIA_PREPARED          = 1,
    MEDIA_PLAYBACK_COMPLETE = 2,
    MEDIA_BUFFERING_UPDATE  = 3,
    MEDIA_SEEK_COMPLETE     = 4,
    MEDIA_SET_VIDEO_SIZE    = 5,
    MEDIA_TIMED_TEXT        = 99,
    MEDIA_ERROR             = 100,
    MEDIA_INFO              = 200,
};

// Generic error codes for the media player framework.  Errors are fatal, the
// playback must abort.
//
// Errors are communicated back to the client using the
// MediaPlayerListener::notify method defined below.
// In this situation, 'notify' is invoked with the following:
//   'msg' is set to MEDIA_ERROR.
//   'ext1' should be a value from the enum media_error_type.
//   'ext2' contains an implementation dependant error code to provide
//          more details. Should default to 0 when not used.
//
// The codes are distributed as follow:
//   0xx: Reserved
//   1xx: Android Player errors. Something went wrong inside the MediaPlayer.
//   2xx: Media errors (e.g Codec not supported). There is a problem with the
//        media itself.
//   3xx: Runtime errors. Some extraordinary condition arose making the playback
//        impossible.
//
enum media_error_type {
    // 0xx
    MEDIA_ERROR_UNKNOWN = 1,

    MEDIA_ERROR_3D_RESOUCE_OCCUPIED = 2,

    // 1xx
    MEDIA_ERROR_SERVER_DIED = 100,

    // 2xx
    MEDIA_ERROR_NOT_VALID_FOR_PROGRESSIVE_PLAYBACK = 200,

    // 3xx
};

// Info and warning codes for the media player framework.  These are non fatal,
// the playback is going on but there might be some user visible issues.
//
// Info and warning messages are communicated back to the client using the
// MediaPlayerListener::notify method defined below.  In this situation,
// 'notify' is invoked with the following:
//   'msg' is set to MEDIA_INFO.
//   'ext1' should be a value from the enum media_info_type.
//   'ext2' contains an implementation dependant info code to provide
//          more details. Should default to 0 when not used.
//
// The codes are distributed as follow:
//   0xx: Reserved
//   7xx: Android Player info/warning (e.g player lagging behind.)
//   8xx: Media info/warning (e.g media badly interleaved.)
//
enum media_info_type {
    // 0xx
    MEDIA_INFO_UNKNOWN = 1,
    //The player just pushed the very first video frame for rendering.
    MEDIA_INFO_VIDEO_RENDERING_START = 3,

    // 7xx
    // The video is too complex for the decoder: it can't decode frames fast
    // enough. Possibly only the audio plays fine at this stage.
    MEDIA_INFO_VIDEO_TRACK_LAGGING = 700,
    // MediaPlayer is temporarily pausing playback internally in order to
    // buffer more data.
    MEDIA_INFO_BUFFERING_START = 701,
    // MediaPlayer is resuming playback after filling buffers.
    MEDIA_INFO_BUFFERING_END = 702,
    // Bandwidth in recent past
    MEDIA_INFO_NETWORK_BANDWIDTH = 703,

    MEDIA_INFO_DOWNLOAD_SPEED = 704,

    MEDIA_INFO_DOWNLOAD_PERCENT = 705,

    MEDIA_INFO_TEXTURE_VAILABLE = 706,

    MEDIA_INFO_DECODE_MODE_NOT_SUPPORT = 707,

    // 8xx
    // Bad interleaving means that a media has been improperly interleaved or not
    // interleaved at all, e.g has all the video samples first then all the audio
    // ones. Video is playing but a lot of disk seek may be happening.
    MEDIA_INFO_BAD_INTERLEAVING = 800,
    // The media is not seekable (e.g live stream).
    MEDIA_INFO_NOT_SEEKABLE = 801,
    // New media metadata is available.
    MEDIA_INFO_METADATA_UPDATE = 802,
};

enum media_player_states {
    MEDIA_PLAYER_STATE_ERROR        = 0,
    MEDIA_PLAYER_IDLE               = 1 << 0,
    MEDIA_PLAYER_INITIALIZED        = 1 << 1,
    MEDIA_PLAYER_PREPARING          = 1 << 2,
    MEDIA_PLAYER_PREPARED           = 1 << 3,
    MEDIA_PLAYER_STARTED            = 1 << 4,
    MEDIA_PLAYER_PAUSED             = 1 << 5,
    MEDIA_PLAYER_STOPPED            = 1 << 6,
    MEDIA_PLAYER_PLAYBACK_COMPLETE  = 1 << 7
};

// Keep KEY_PARAMETER_* in sync with MediaPlayer.java.
// The same enum space is used for both set and get, in case there are future keys that
// can be both set and get.  But as of now, all parameters are either set only or get only.
enum media_parameter_keys {
    KEY_PARAMETER_TIMED_TEXT_TRACK_INDEX = 1000,                // set only
    KEY_PARAMETER_TIMED_TEXT_ADD_OUT_OF_BAND_SOURCE = 1001,     // set only

    // Streaming/buffering parameters
    KEY_PARAMETER_CACHE_STAT_COLLECT_FREQ_MS = 1100,            // set only

    // Return a Parcel containing a single int, which is the channel count of the
    // audio track, or zero for error (e.g. no audio track) or unknown.
    KEY_PARAMETER_AUDIO_CHANNEL_COUNT = 1200,                   // get only
};

class INBMediaPlayerLister {
public:
    INBMediaPlayerLister() {
    }
    virtual ~INBMediaPlayerLister() {
    }

public:
    virtual void sendEvent(int msg, int ext1=0, int ext2=0) = 0;
};

class INBMediaSeekListener {
public:
    INBMediaSeekListener() {}
    virtual ~INBMediaSeekListener() {}

public:
    virtual void onMediaSeeked(int64_t pos, nb_status_t status) = 0;
};

class NBMediaPlayer : public INBMediaSeekListener {
public:
    NBMediaPlayer();
    ~NBMediaPlayer();

public:
    nb_status_t prepareAsync();
    nb_status_t setDataSource(const char* uri, const NBMap<NBString, NBString>* params = NULL);
    nb_status_t play();
    nb_status_t seekTo(int64_t millisec);
    nb_status_t pause();

    bool isPlaying() const;
    nb_status_t getDuration(int64_t *durationUs);
    nb_status_t getCurrentPosition(int64_t *positionUs);

    void reset();
    void stop();
    
    nb_status_t getVideoWidth(int32_t *vWidth);
    nb_status_t getVideoHeight(int32_t *vHeight);

    //set the input value
    nb_status_t setListener(INBMediaPlayerLister* listener);
    nb_status_t setVideoOutput(NBRendererTarget* vo);

private:
    //help functions
    nb_status_t prepareAsync_l();
    nb_status_t finishSetDataSource_l();
    nb_status_t setDataSource_l(const char *uri, const NBMap<NBString, NBString>* params);
    nb_status_t setDataSource_l(NBDataSource *dataSource);
    nb_status_t setDataSource_l(NBMediaExtractor *extractor);
    nb_status_t play_l();
    nb_status_t pause_l(bool at_eos = false);
    nb_status_t seekTo_l(int64_t millisec);
    void reset_l();

    void finishAsyncPrepare_l();

private:
    //process actually things
    void onPrepareAsyncEvent();
    void cancelPlayerEvents(bool keepNotifications = false);

private:
    void addAudioSource(NBMediaSource *source);
    void setVideoSource(NBMediaSource *source);

    nb_status_t initAudioDecoder();
    nb_status_t initVideoDecoder(uint32_t flags = 0);

    void finishSeekIfNecessary(int64_t videoTimeUs);

    nb_status_t createAudioPlayer_l();
    nb_status_t startAudioPlayer_l();
    nb_status_t initVideoRenderer_l();
    void shutdownVideoDecoder_l();

    //notify the user video size
    void notifyVideoSize_l();

private:
    enum {
        PLAYING             = 0x01,
        LOOPING             = 0x02,
        FIRST_FRAME         = 0x04,
        PREPARING           = 0x08,
        PREPARED            = 0x10,
        AT_EOS              = 0x20,
        PREPARE_CANCELLED   = 0x40,
        CACHE_UNDERRUN      = 0x80,
        AUDIO_AT_EOS        = 0x0100,
        VIDEO_AT_EOS        = 0x0200,
        AUTO_LOOPING        = 0x0400,

        // We are basically done preparing but are currently buffering
        // sufficient data to begin playback and finish the preparation phase
        // for good.
        PREPARING_CONNECTED = 0x0800,

        // We're triggering a single video event to display the first frame
        // after the seekpoint.
        SEEK_PREVIEW        = 0x1000,

        AUDIO_RUNNING       = 0x2000,
        AUDIOPLAYER_STARTED = 0x4000,

        INCOGNITO           = 0x8000,

        TEXT_RUNNING        = 0x10000,
        TEXTPLAYER_STARTED  = 0x20000,

        SLOW_DECODER_HACK   = 0x40000,

        USER_PASUED         = 0x80000,

        STOP                = 0x100000,

    };


    NBMutex mLock;
    uint32_t mFlags;

    NBMutex mMiscStateLock;
    NBMutex mStatsLock;
    NBMutex mAudioLock;

    enum FlagMode {
        SET,
        CLEAR,
        ASSIGN
    };
    void modifyFlags(unsigned value, FlagMode mode);

    NBTimedEventQueue mQueue;
    bool mQueueStarted;

    NBString mUri;
    NBMap<NBString, NBString> mParams;

    INBMediaPlayerLister* mListener;
    void notifyListener_l(int msg, int ext1 = 0, int ext2 = 0);
    void abortPrepare(nb_status_t err);

    //prepare async event
    NBTimedEventQueue::Event* mAsyncPrepareEvent;
    NBCondition mPreparedCondition;
    bool mIsAsyncPrepare;
    nb_status_t mPrepareResult;

private:
    NBTimedEventQueue::Event* mVideoEvent;
    bool mVideoEventPending;
    NBTimedEventQueue::Event* mStreamDoneEvent;
    bool mStreamDoneEventPending;
    NBTimedEventQueue::Event* mCheckAudioStatusEvent;
    bool mAudioStatusEventPending;

    void postVideoEvent_l(int64_t delayUs = -1);

    void onVideoEvent();
    void onCheckAudioStatus();
    void onStreamDone();

    void postStreamDoneEvent_l(nb_status_t status);
    void postAudioEOS(int64_t delayUs = 0ll);
    void postCheckAudioStatusEvent(int64_t delayUs);

    virtual void onMediaSeeked(int64_t pos, nb_status_t status);

private:
    //The datasource object
    NBDataSource* mDataSource;

    //The extractor object
    friend class NBMediaExtractor;
    NBMediaExtractor* mMediaExtractor;

    NBMediaSource* mAudioSource;
    NBMediaSource* mVideoSource;

    //The media tract object
    NBMediaSource* mVideoTrack;
    NBMediaBuffer* mVideoBuffer;

    enum SeekType {
        NO_SEEK,
        SEEK,
        SEEK_VIDEO_ONLY,
        SEEK_ACTIONED,
    };
    SeekType mSeeking;

    NBRendererTarget* mVideoOutput;

    int mDisplayWidth;
    int mDisplayHeight;
    int mVideoFrameRate;
    
    // protected by mStatsLock
    struct Stats {
        int mFd;
        NBString mURI;
        int64_t mBitrate;
        
        // FIXME:
        // These two indices are just 0 or 1 for now
        // They are not representing the actual track
        // indices in the stream.
        ssize_t mAudioTrackIndex;
        ssize_t mVideoTrackIndex;
        
        int64_t mNumVideoFramesDecoded;
        int64_t mNumVideoFramesDropped;
        int32_t mVideoWidth;
        int32_t mVideoHeight;
        uint32_t mFlags;
//        Vector<TrackStat> mTracks;
    } mStats;

    //The audio track context
    NBMediaSource* mAudioTracks[MAX_AUDIO_TRACK_COUNT];
    int mAudioTrackIndex;
    int mActiveAudioTrackIndex;

    friend class NBAudioPlayer;
    NBAudioPlayer* mAudioPlayer;
    NBVideoRenderer* mVideoRenderer;
    // NBGLVideoRenderer* mVideoRenderer;

    //The clock manager for sync
    //NBClockManager* mClockManager;
    int64_t mLastVideoTimeUs;
    int64_t mVideoTimeUs;
    int64_t mDurationUs;
    int64_t mSeekTimeUs;
    int32_t mBitrate;

    uint32_t mExtractorFlags;

    bool mWatchForAudioEOS;
    bool mSeekNotificationSent;
    bool mVideoRendererIsPreview;
    bool mWatchForAudioSeekComplete;
    bool mVideoRenderingStarted;

    nb_status_t mStreamDoneStatus;

    //Use clock proxy
    friend NBClockProxy;
    NBClockProxy* mClockProxy;

    int64_t getCurrentClock();
};

#endif //NBMEDIAPLAYER_H
