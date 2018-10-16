//
// Created by parallels on 9/9/18.
//

#include "NBAVPacket.h"

AVPacket NBFFmpegAVPacket::FlashPacket;

NBFFmpegAVPacket::NBFFmpegAVPacket(AVPacket pkt, int64_t duration)
        : NBMediaBuffer(duration) {
    if (duration == 0) {
//            DEBUG("The push packet data is : %p", mPacket.data);
    }
    mPacket = pkt;
}

NBFFmpegAVPacket::~NBFFmpegAVPacket() {
    av_packet_unref(&mPacket);
}

void* NBFFmpegAVPacket::data() {
    if (mDuration == 0) {
//            DEBUG("The pop packet data is : %p", mPacket.data);
    }
    return &mPacket;
}