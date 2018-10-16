//
// Created by parallels on 9/9/18.
//

#include "NBFFmpegMediaTrack.h"
#include "NBBufferQueue.h"
#include "NBAVPacket.h"
#include "extractor/NBMediaExtractor.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <libavformat/avformat.h>

#ifdef __cplusplus
}
#endif

NBFFmpegMediaTrack::NBFFmpegMediaTrack(NBMetaData *meta, NBMediaExtractor* extractor)
    : mExtractor(extractor)
    ,mMetaData(meta)
    ,mMediaBufferQueue(NULL) {

}

NBFFmpegMediaTrack::~NBFFmpegMediaTrack() {

}

nb_status_t NBFFmpegMediaTrack::start(NBMetaData *params) {
    if (mMetaData != NULL) {
        mMetaData->setInt32(kKeyTrackActive, 1);

        AVStream *stream = NULL;
        mMetaData->findPointer(kKeyFFmpegStream, (void**)&stream);

        if (stream != NULL) {
            stream->discard = AVDISCARD_DEFAULT;
        }

        mMetaData->findPointer(kKeyMediaBufferQueue, (void**)&mMediaBufferQueue);
    }
    return OK;
}

nb_status_t NBFFmpegMediaTrack::stop() {
    if (mMetaData != NULL) {
        mMetaData->setInt32(kKeyTrackActive, 0);

        AVStream *stream = NULL;
        mMetaData->findPointer(kKeyFFmpegStream, (void**)&stream);

        if (stream != NULL) {
            stream->discard = AVDISCARD_ALL;
        }
    }
    return OK;
}

NBMetaData* NBFFmpegMediaTrack::getFormat() {
    return mMetaData;
}

// Returns a new buffer of data. Call blocks until a
// buffer is available, an error is encountered of the end of the stream
// is reached.
// End of stream is signalled by a result of ERROR_END_OF_STREAM.
// A result of INFO_FORMAT_CHANGED indicates that the format of this
// MediaSource has changed mid-stream, the client can continue reading
// but should be prepared for buffers of the new configuration.
nb_status_t NBFFmpegMediaTrack::read(
        NBMediaBuffer **buffer, ReadOptions *options) {
//    if (options != NULL && options->getPeekSeek()) {
//        NBMediaBuffer* peekBuffer = NULL;
//        //check if seek request
//        if(mMediaBufferQueue->peekPacket(&peekBuffer) <= 0) {
//            return OK;
//        }
//
//        AVPacket* packet = (AVPacket*)peekBuffer->data();
//        if (packet->data != NBFFmpegAVPacket::FlashPacket.data) {
//            return OK;
//        }
//
//        mMediaBufferQueue->popPacket(&peekBuffer);
//        delete peekBuffer;
//
//        return INFO_DISCONTINUITY;
//    }

    if (mMediaBufferQueue->popPacket(buffer) == 1) {
        AVPacket* packet = (AVPacket*)(*buffer)->data();
        if (packet->data == NBFFmpegAVPacket::FlashPacket.data) {
            delete *buffer;
            *buffer = NULL;
            return INFO_DISCONTINUITY;
        }

//        printf("Read packet index : %d packet count : %d\n", packet->stream_index, mMediaBufferQueue->getCount());

        return OK;
    }

    if (mExtractor->getReachedEnd()) {
        return ERROR_END_OF_STREAM;
    }

    //notify start read the data
    mExtractor->notifyContinueRead();

    return ERROR_BUFFER_TOO_SMALL;
}
