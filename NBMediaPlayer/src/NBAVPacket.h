//
// Created by parallels on 9/9/18.
//

#ifndef NBAVPACKET_H
#define NBAVPACKET_H

#include "NBMediaBuffer.h"

#ifdef __cplusplus
extern "C"
{
#endif
#include "libavcodec/avcodec.h"
#ifdef __cplusplus
}
#endif

class NBFFmpegAVPacket: public NBMediaBuffer {
public:
    NBFFmpegAVPacket(AVPacket pkt, int64_t duration);
    virtual ~NBFFmpegAVPacket();

    virtual void* data();

public:
    static AVPacket FlashPacket;

private:
    AVPacket mPacket;
};

#endif //NBAVPACKET_H
