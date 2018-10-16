//
// Created by parallels on 9/9/18.
//

#ifndef NBFFMPEGADECODER_H
#define NBFFMPEGADECODER_H

#include "NBMediaSource.h"

struct AVCodecContext;
struct AVStream;
struct AVFrame;
struct AVCodecParameters;

#ifdef __cplusplus
extern "C" {
#endif

#include "libavutil/rational.h"

#ifdef __cplusplus
}
#endif

class NBFFmpegADecoder : public NBMediaSource {
public:
    NBFFmpegADecoder(NBMediaSource* mediaTrack);
    virtual ~NBFFmpegADecoder();

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

private:
    bool openAudioCodecContext(AVCodecParameters* codecpar);

private:
    NBMediaSource* mMediaTrack;
    AVCodecContext *mACodecCtx;
    AVStream* mAStream;

    AVFrame* mDecodedFrame;

    int64_t mStartTime;

    int64_t mNextPts;
    AVRational mNextPtsTb;

    int64_t mStartPts;
    AVRational mStartPtsTb;

    NBMediaBuffer* mCachedBuffer;

private:
    NBMetaData mMetaData;
};

#endif //NBFFMPEGADECODER_H
