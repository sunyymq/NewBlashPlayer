//
// Created by parallels on 9/9/18.
//

#ifndef NBMEDIABUFFER_H
#define NBMEDIABUFFER_H

#include <NBList.h>
#include <stdlib.h>
#include <stdint.h>

#define VIDEO_PICTURE_QUEUE_SIZE 3
#define SUBPICTURE_QUEUE_SIZE 16
#define SAMPLE_QUEUE_SIZE 9

class NBMediaBuffer {
public:
    struct list_head list;
public:
    NBMediaBuffer(int64_t duration = 0);
    virtual ~NBMediaBuffer();
    
public:
    virtual void* data() {
        return NULL;
    }
    int64_t getDuration();

protected:
    int64_t mDuration;
};

#endif //NBMEDIABUFFER_H
