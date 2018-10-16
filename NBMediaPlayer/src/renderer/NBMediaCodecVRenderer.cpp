//
// Created by liu enbao on 24/09/2018.
//

#include "NBMediaCodecVRenderer.h"
#include <NBLog.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <libavformat/avformat.h>

#ifdef __cplusplus
}
#endif

#define LOG_TAG "NBMediaCodecVRenderer"

NBMediaCodecVRenderer::NBMediaCodecVRenderer() {

}

NBMediaCodecVRenderer::~NBMediaCodecVRenderer() {

}

nb_status_t NBMediaCodecVRenderer::displayFrame(NBMediaBuffer* mediaBuffer) {
//    NBLOG_INFO(LOG_TAG, "display frame everyframe");
    AVFrame* frame = (AVFrame*)mediaBuffer->data();

    //set flags to 1 show this frame
    frame->flags = 1;
    return OK;
}

nb_status_t NBMediaCodecVRenderer::start(NBMetaData* metaData) {
    //do nothing at all
    return OK;
}

nb_status_t NBMediaCodecVRenderer::stop() {
    //do nothing at all
    return OK;
}