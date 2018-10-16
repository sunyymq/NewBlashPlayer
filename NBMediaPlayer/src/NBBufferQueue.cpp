//
// Created by parallels on 9/9/18.
//

#include "NBBufferQueue.h"

NBBufferQueue::NBBufferQueue() {
    pthread_mutex_init(&mMutex, NULL);

    INIT_LIST_HEAD(&mList);

    mBufferCount = 0;
    mDuration = 0;
}

NBBufferQueue::~NBBufferQueue() {
    pthread_mutex_destroy(&mMutex);
}

size_t NBBufferQueue::getCount() {
    // size_t bufferCount = 0;

    // pthread_mutex_lock(&mMutex);
    // bufferCount = mList.size();
    // pthread_mutex_unlock(&mMutex);

    // return bufferCount;

    return mBufferCount;
}

/**
 Get packet from queue thread-safely
 outPacket: output packet
 block    : should block the process to wait until there is packet available.
 */
int NBBufferQueue::peekPacket(NBMediaBuffer** outPacket) {
    int ret = 0;

    pthread_mutex_lock(&mMutex);
    if (!list_empty(&mList)) {
        *outPacket = list_first_entry(&mList, NBMediaBuffer, list);
        ret = 1;
    }else {
        ret = 0;
    }
    pthread_mutex_unlock(&mMutex);

    return  ret;
}

/**
 Get packet from queue thread-safely
 outPacket: output packet
 block    : should block the process to wait until there is packet available.
 */
int NBBufferQueue::popPacket(NBMediaBuffer** outPacket) {
    int ret = 0;

    pthread_mutex_lock(&mMutex);
    if (!list_empty(&mList)) {


        *outPacket = list_first_entry(&mList, NBMediaBuffer, list);

        list_del(&(*outPacket)->list);

        -- mBufferCount;
        mDuration -= (*outPacket)->getDuration();

        ret = 1;
    }else {
        ret = 0;
    }
    pthread_mutex_unlock(&mMutex);

    return  ret;
}

int NBBufferQueue::popPacketPredicate(bool (*predicate)(int64_t pts, NBMediaBuffer *buffer), int64_t pts) {
    int ret = 0;
    pthread_mutex_lock(&mMutex);

    struct list_head *pos, *q;

    list_for_each_safe(pos, q, &mList){
        NBMediaBuffer* dummyBuffer = list_entry(pos, NBMediaBuffer, list);
        if (predicate(pts, dummyBuffer)) {
            list_del(pos);
            --mBufferCount;
            delete dummyBuffer;
        } else
            break;
    }

    pthread_mutex_unlock(&mMutex);
    return ret;
}

/**
 Get packet from queue thread-safely
 inPacket : input packet
 block    : should block the process to wait until there is space for new packet.
 */
int NBBufferQueue::pushPacket(NBMediaBuffer* inPacket) {
    int ret = 0;

    pthread_mutex_lock(&mMutex);
    list_add_tail(&inPacket->list, &mList);

    ret = 1;

    ++ mBufferCount;
    mDuration += inPacket->getDuration();

    pthread_mutex_unlock(&mMutex);

    return  ret;
}

/**
 Flush all the packets in the queue.
 */
void NBBufferQueue::flush() {
    pthread_mutex_lock(&mMutex);

    struct list_head *pos, *q;

    list_for_each_safe(pos, q, &mList){
        NBMediaBuffer* dummyBuffer = list_entry(pos, NBMediaBuffer, list);
        list_del(pos);
        --mBufferCount;
        delete dummyBuffer;
    }

    mBufferCount = 0;
    mDuration = 0;

    pthread_mutex_unlock(&mMutex);
}

// void MediaBufferQueue::clearDuration() {
//     mDuration = 0;
// }

// void MediaBufferQueue::incrimentDuration(int64_t duration) {
//     mDuration += duration;
// }

// void MediaBufferQueue::decrimentDuration(int64_t duration) {
//     mDuration -= duration;
// }

int64_t NBBufferQueue::getDuration() {
    return mDuration;
}
