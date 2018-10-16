//
// Created by parallels on 9/9/18.
//

#ifndef NBFFMPEGVDECODER_H
#define NBFFMPEGVDECODER_H

#include "NBMediaSource.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "libavutil/rational.h"
#include "libavutil/pixfmt.h"

#ifdef __cplusplus
}
#endif

struct AVStream;
struct AVFrame;
struct SwsContext;
struct AVCodecContext;
struct AVCodecParameters;

class NBFFmpegVDecoder : public NBMediaSource {
public:
    NBFFmpegVDecoder(NBMediaSource* mediaTrack);
    virtual ~NBFFmpegVDecoder();

    enum DecoderRecorderPts {
        DRP_AUTO = -1,
        DRP_OFF,
        DRP_ON,
    };

public:
    // To be called before any other methods on this object, except
    // getFormat().
    virtual nb_status_t start(NBMetaData *params = NULL);

    // Any blocking read call returns immediately with a result of NO_INIT.
    // It is an error to call any methods other than start after this call
    // returns. Any buffers the object may be holding onto at the time of
    // the stop() call are released.
    // Also, it is imperative that any buffers output by this object and
    // held onto by callers be released before a call to stop() !!!
    virtual nb_status_t stop();

    // Returns the format of the data output by this media source.
    virtual NBMetaData* getFormat();

    // Returns a new buffer of data. Call blocks until a
    // buffer is available, an error is encountered of the end of the stream
    // is reached.
    // End of stream is signalled by a result of ERROR_END_OF_STREAM.
    // A result of INFO_FORMAT_CHANGED indicates that the format of this
    // MediaSource has changed mid-stream, the client can continue reading
    // but should be prepared for buffers of the new configuration.
    virtual nb_status_t read(
            NBMediaBuffer **buffer, ReadOptions *options = NULL);
    
    // Discard nonref ref and looperfilter if video is far delay audio
    virtual void discardNonRef();
    
    // restore the default discard state
    virtual void restoreDiscard();

private:
    bool openVideoCodecContext(AVCodecParameters* codecpar);

    static enum AVPixelFormat get_format(AVCodecContext *s, const enum AVPixelFormat *pix_fmts);
    
private:
    NBMediaSource* mMediaTrack;

    AVStream* mVStream;
    AVCodecContext* mVCodecCxt;
    SwsContext* mSwsContext;

    AVFrame* mDecodedFrame;
    AVFrame* mScaledFrame;

    int64_t mStartTime;

    int64_t next_pts;
    AVRational next_pts_tb;

    NBMediaBuffer* mCachedBuffer;

    //let decoder recorder the pts
    DecoderRecorderPts mDecoderReorderPts;

private:
    NBMetaData mMetaData;
};

#endif //NBFFMPEGVDECODER_H
