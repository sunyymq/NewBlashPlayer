//
// Created by parallels on 9/9/18.
//

#ifndef NBMEDIABUFFERQUEUE_H
#define NBMEDIABUFFERQUEUE_H

#include <stdint.h>
#include <pthread.h>

#include "NBMediaBuffer.h"

class NBBufferQueue {
public:
    NBBufferQueue();
    ~NBBufferQueue();

    size_t getCount();

    int peekPacket(NBMediaBuffer** outPacket);

    /**
     Get packet from queue thread-safely
     */
    int popPacket(NBMediaBuffer** outPacket);

    /**
     Pop packet from queue thread-safely
     Util the predicate is false
     */
    int popPacketPredicate(bool (*predicate)(int64_t pts, NBMediaBuffer *buffer), int64_t pts);

    /**
     Get packet from queue thread-safely
     */
    int pushPacket(NBMediaBuffer* inPacket);

    // void clearDuration();
    // void incrimentDuration(int64_t duration);
    // void decrimentDuration(int64_t duration);
    int64_t getDuration();

    /**
     Flush all the packets in the queue.
     */
    void flush();
public:
    pthread_mutex_t mMutex;
    int64_t mDuration;
    struct list_head mList;
    int mBufferCount;
};

#endif //NBMEDIABUFFERQUEUE_H
