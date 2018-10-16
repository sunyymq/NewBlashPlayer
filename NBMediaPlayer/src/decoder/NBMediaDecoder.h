//
// Created by parallels on 9/10/18.
//

#ifndef NBMEDIADECODER_H
#define NBMEDIADECODER_H

#include "NBMediaSource.h"
#include "foundation/NBMetaData.h"

class NBMediaDecoder {
public:
    /** the audio decoder only support software
      * the video decoder select hardware in auto mode
      */
    enum {
        NB_DECODER_FLAG_AUTO_SELECT = 0x01,
        NB_DECODER_FLAG_FORCE_SOFTWARE = 0x02
    };
    
public:
    static NBMediaSource* Create(NBMetaData* metaData, NBMediaSource* mediaTrack, void* window, uint32_t flags = NB_DECODER_FLAG_AUTO_SELECT);
    static void Destroy(NBMediaSource* mediaSource);

private:
    NBMediaDecoder() {
    }
    ~NBMediaDecoder() {
    }
};

#endif //NBMEDIADECODER_H
