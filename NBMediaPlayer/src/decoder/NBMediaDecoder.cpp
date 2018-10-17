//
// Created by parallels on 9/10/18.
//

#include "NBMediaDecoder.h"
#include "foundation/NBFoundation.h"
#include "NBFFmpegADecoder.h"
#include "NBFFmpegVDecoder.h"

#ifdef BUILD_TARGET_IOS
#include "NBVideoToolboxDecoder.h"
#elif BUILD_TARGET_ANDROID

#include "NBMediaCodecVDecoder.h"
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"

#ifdef __cplusplus
}
#endif

NBMediaSource* NBMediaDecoder::Create(NBMetaData* metaData, NBMediaSource* mediaTrack, void* window, uint32_t flags) {
    const char *_mime;
    metaData->findCString(kKeyMIMEType, &_mime);

    NBMediaSource* ret = NULL;
    if (!strcasecmp(_mime, MEDIA_MIMETYPE_AUDIO_FFMPEG)) {
        ret = new NBFFmpegADecoder(mediaTrack);
    } else if(!strcasecmp(_mime, MEDIA_MIMETYPE_VIDEO_FFMPEG)) {

        AVStream* videoStream = NULL;
        AVCodecParameters* videoCodecPar = NULL;

        NBMetaData* videoMeta = mediaTrack->getFormat();
        if(!videoMeta->findPointer(kKeyFFmpegStream, (void**)&videoStream)) {
            return NULL;
        }

        if (videoStream == NULL) {
            return NULL;
        }

        videoCodecPar = videoStream->codecpar;

        if (flags & NB_DECODER_FLAG_AUTO_SELECT) {
        // autoselect decoder mode
#if BUILD_TARGET_ANDROID
            // current only support h264
            if (videoCodecPar->codec_id == AV_CODEC_ID_H264) {
                ret = new NBMediaCodecVDecoder(mediaTrack, window);
            } else {
                ret = new NBFFmpegVDecoder(mediaTrack);
            }
#elif BUILD_TARGET_IOS
            if (videoCodecPar->codec_id == AV_CODEC_ID_H264
                || videoCodecPar->codec_id == AV_CODEC_ID_MPEG4) {
                ret = new NBVideoToolboxDecoder(mediaTrack);
            } else {
                ret = new NBFFmpegVDecoder(mediaTrack);
            }
#endif
        } else {
            ret = new NBFFmpegVDecoder(mediaTrack);
        }
    }
    return ret;
}

void NBMediaDecoder::Destroy(NBMediaSource* mediaSource) {
    if (mediaSource == NULL) {
        return ;
    }
    delete mediaSource;
}
