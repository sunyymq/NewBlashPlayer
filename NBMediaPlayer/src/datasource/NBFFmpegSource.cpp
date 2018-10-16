//
// Created by parallels on 9/9/18.
//

#include "NBFFmpegSource.h"

#include <NBLog.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <libavformat/avio.h>
#include <libavformat/avformat.h>
#include <libavformat/rtpdec.h>
#include <libavformat/rdt.h>

extern AVInputFormat ff_rtsp_demuxer;
    
extern AVHWAccel ff_h263_videotoolbox_hwaccel;
extern AVHWAccel ff_hevc_videotoolbox_hwaccel;
extern AVHWAccel ff_h264_videotoolbox_hwaccel;
extern AVHWAccel ff_mpeg1_videotoolbox_hwaccel;
extern AVHWAccel ff_mpeg2_videotoolbox_hwaccel;
extern AVHWAccel ff_mpeg4_videotoolbox_hwaccel;
    
#ifdef __cplusplus
}
#endif

#define LOG_TAG "NBFFmpegSource"

NBFFmpegSource::NBFFmpegSource(const NBString& uri)
    :mUri(uri)
    ,mFormatCtx(NULL) {

}

NBFFmpegSource::~NBFFmpegSource() {
    NBLOG_INFO(LOG_TAG, "FFmpeg data source closing : %p", mFormatCtx);
    if (mFormatCtx != NULL) {
        avformat_close_input(&mFormatCtx);
    }
}

void NBFFmpegSource::RegisterSource() {
    av_register_all();
    
    avformat_network_init();

    // add rtsp support
    av_register_input_format(&ff_rtsp_demuxer);
    
    // register the rtp payload
    ff_register_rtp_dynamic_payload_handlers();
    
    ff_register_rdt_dynamic_payload_handlers();
    
//    extern AVHWAccel ff_h263_videotoolbox_hwaccel;
//    extern AVHWAccel ff_hevc_videotoolbox_hwaccel;
//    extern AVHWAccel ff_h264_videotoolbox_hwaccel;
//    extern AVHWAccel ff_mpeg1_videotoolbox_hwaccel;
//    extern AVHWAccel ff_mpeg2_videotoolbox_hwaccel;
//    extern AVHWAccel ff_mpeg4_videotoolbox_hwaccel;
//    //register all hardware decoder
//    av_register_hwaccel(&ff_h263_videotoolbox_hwaccel);
//    av_register_hwaccel(&ff_hevc_videotoolbox_hwaccel);
//    av_register_hwaccel(&ff_h264_videotoolbox_hwaccel);
//    av_register_hwaccel(&ff_mpeg1_videotoolbox_hwaccel);
//    av_register_hwaccel(&ff_mpeg2_videotoolbox_hwaccel);
//    av_register_hwaccel(&ff_mpeg4_videotoolbox_hwaccel);
}

void NBFFmpegSource::UnRegisterSource() {
    avformat_network_deinit();
}

int NBFFmpegSource::CheckUserInterupted(void* opaque) {
    NBFFmpegSource* dataSource = (NBFFmpegSource*)opaque;
    if (dataSource->mUserInterrupted) {
        return 1;
    }
    return 0;
}

nb_status_t NBFFmpegSource::initCheck(const NBMap<NBString, NBString>* params) {
    nb_status_t rc = OK;
    AVDictionary* openDict = NULL;
    if (params != NULL) {
        //do something with params
    }
    
    AVIOContext* ioCtx = NULL;
    AVIOInterruptCB interrupt_callback = {
        .callback = CheckUserInterupted,
        .opaque = (void*)this,
    };
    
    if (strncmp(mUri.string(), "rtsp://", 7) != 0) {
        if ((rc = avio_open2(&ioCtx, mUri.string(), AVIO_FLAG_READ, &interrupt_callback, &openDict))) {
            char errbuf[AV_ERROR_MAX_STRING_SIZE] = { 0 };
            NBLOG_ERROR(LOG_TAG, "The error string is is : %s", av_make_error_string(errbuf, AV_ERROR_MAX_STRING_SIZE, rc));
            return UNKNOWN_ERROR;
        }
    }
    
    mFormatCtx = avformat_alloc_context();
    mFormatCtx->pb = ioCtx;
    mFormatCtx->interrupt_callback = interrupt_callback;
    
    return rc;
}
