//
// Created by parallels on 9/9/18.
//

#include "NBTimedEventQueue.h"

#ifdef BUILD_TARGET_ANDROID
#include <jni.h>
#endif

NBTimedEventQueue::NBTimedEventQueue()
        : mNextEventID(1)
        ,mRunning(false)
        ,mStopped(false)
{
    INIT_LIST_HEAD(&mQueue);
}

NBTimedEventQueue::~NBTimedEventQueue() {
    stop();
}

void NBTimedEventQueue::start() {
    if (mRunning) {
        return;
    }

    mStopped = false;

//    INIT_LIST_HEAD(&mQueue.list);

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    pthread_create(&mThread, &attr, ThreadWrapper, (void*)this);

    pthread_attr_destroy(&attr);

    mRunning = true;
}

void NBTimedEventQueue::stop(bool flush) {
    if (!mRunning) {
        return;
    }

    NBTimedEventQueue::Event* stopEvent = new StopEvent();

    if (flush) {
        postEventToBack(stopEvent);
    } else {
        postTimedEvent(stopEvent, INT64_MIN);
    }

    void *dummy;
    pthread_join(mThread, &dummy);

    struct list_head *pos, *q;
    list_for_each_safe(pos, q, &mQueue) {
        QueueItem* dummyItem = list_entry(pos, struct QueueItem, list);
        list_del(pos);
        delete dummyItem;
    }

    delete stopEvent;

    mRunning = false;
}

NBTimedEventQueue::event_id NBTimedEventQueue::postEvent(const Event *event) {
    // Reserve an earlier timeslot an INT64_MIN to be able to post
    // the StopEvent to the absolute head of the queue.
    return postTimedEvent(event, INT64_MIN + 1);
}

NBTimedEventQueue::event_id NBTimedEventQueue::postEventToBack(
        const Event *event) {
    return postTimedEvent(event, INT64_MAX);
}

NBTimedEventQueue::event_id NBTimedEventQueue::postEventWithDelay(
        const Event *event, int64_t delay_us) {
    return postTimedEvent(event, getRealTimeUs() + delay_us);
}

NBTimedEventQueue::event_id NBTimedEventQueue::postTimedEvent(
        const Event *event, int64_t realtime_us) {
    NBAutoMutex autoLock(mLock);

    event->setEventID(mNextEventID++);

    struct list_head *pos;
    list_for_each(pos, &mQueue) {
        QueueItem* tmpItem = list_entry(pos, struct QueueItem, list);
        if (tmpItem->realtime_us > realtime_us)
            break;
    }

    QueueItem *item = new QueueItem;
    item->event = event;
    item->realtime_us = realtime_us;

    if (list_empty(&mQueue)) {
        mQueueHeadChangedCondition.signal();
    }

    list_add(&item->list, pos);

    mQueueNotEmptyCondition.signal();

    return event->eventID();
}

static bool MatchesEventID(
        void *cookie, const NBTimedEventQueue::Event *event) {
    NBTimedEventQueue::event_id *id =
            static_cast<NBTimedEventQueue::event_id *>(cookie);

    if (event->eventID() != *id) {
        return false;
    }

    *id = 0;

    return true;
}

bool NBTimedEventQueue::cancelEvent(event_id id, bool stopAtFirstMatch) {
    if (id == 0) {
        return false;
    }

    cancelEvents(&MatchesEventID, &id, stopAtFirstMatch /* stopAfterFirstMatch */);

    // if MatchesEventID found a match, it will have set id to 0
    // (which is not a valid event_id).

    return id == 0;
}

void NBTimedEventQueue::cancelEvents(
        bool (*predicate)(void *cookie, const Event *event),
        void *cookie,
        bool stopAfterFirstMatch) {
    NBAutoMutex autoLock(mLock);

    struct list_head *pos, *q;
    bool cancelFirst = true;
    list_for_each_safe(pos, q, &mQueue) {
        QueueItem* dummyItem = list_entry(pos, struct QueueItem, list);

        if (!(*predicate)(cookie, dummyItem->event)) {
            cancelFirst = false;
            continue;
        }

        if (cancelFirst) {
            mQueueHeadChangedCondition.signal();
        }

        dummyItem->event->setEventID(0);
        list_del(&dummyItem->list);
        
        delete dummyItem;
        dummyItem = NULL;

        if (stopAfterFirstMatch) {
            return;
        }
    }
}

// static
int64_t NBTimedEventQueue::getRealTimeUs() {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return (int64_t)tv.tv_sec * 1000000ll + tv.tv_usec;
}

void NBTimedEventQueue::runEvent(event_id eventID, int64_t timeUs) {
    const Event* event = removeEventFromQueue_l(eventID);
    if (event != NULL) {
        event->fire(this, timeUs);
    }
}

// static
void *NBTimedEventQueue::ThreadWrapper(void *me) {

// #ifdef ANDROID_SIMULATOR
//         // The simulator runs everything as one process, so any
//         // Binder calls happen on this thread instead of a thread
//         // in another process. We therefore need to make sure that
//         // this thread can do calls into interpreted code.
//         // On the device this is not an issue because the remote
//         // thread will already be set up correctly for this.
//         JavaVM *vm;
//         int numvms;
//         JNI_GetCreatedJavaVMs(&vm, 1, &numvms);
//         JNIEnv *env;
//         vm->AttachCurrentThread(&env, NULL);
// #endif

#ifdef BUILD_TARGET_ANDROID
    extern JavaVM* getJavaVM();
    JavaVM *javaVM = getJavaVM();
    JNIEnv *env;
    javaVM->AttachCurrentThread(&env, NULL);
#endif

    static_cast<NBTimedEventQueue *>(me)->threadEntry();

#ifdef BUILD_TARGET_ANDROID
    javaVM->DetachCurrentThread();
#endif

// #ifdef ANDROID_SIMULATOR
//         vm->DetachCurrentThread();
// #endif

    return NULL;
}

void NBTimedEventQueue::threadEntry() {

#ifdef __linux__
    pthread_setname_np(mThread, mThreadName.string());
#else
    pthread_setname_np(mThreadName.string());
#endif

    for (;;) {
        int64_t now_us = 0;
        const Event* event = NULL;

        {
            NBAutoMutex autoLock(mLock);

            if (mStopped) {
                break;
            }

            while (list_empty(&mQueue)) {
                mQueueNotEmptyCondition.wait(mLock);
            }

            event_id eventID = 0;
            for (;;) {
                if (list_empty(&mQueue)) {
                    // The only event in the queue could have been cancelled
                    // while we were waiting for its scheduled time.
                    break;
                }

                QueueItem* item = list_first_entry(&mQueue, QueueItem, list);
                eventID = item->event->eventID();

                now_us = getRealTimeUs();
                int64_t when_us = item->realtime_us;

                int64_t delay_us;
                if (when_us < 0 || when_us == INT64_MAX) {
                    delay_us = 0;
                } else {
                    delay_us = when_us - now_us;
                }

                if (delay_us <= 0) {
                    break;
                }

                static int64_t kMaxTimeoutUs = 10000000ll;  // 10 secs
                bool timeoutCapped = false;
                if (delay_us > kMaxTimeoutUs) {
                    // We'll never block for more than 10 secs, instead
                    // we will split up the full timeout into chunks of
                    // 10 secs at a time. This will also avoid overflow
                    // when converting from us to ns.
                    delay_us = kMaxTimeoutUs;
                    timeoutCapped = true;
                }

                nb_status_t err = mQueueHeadChangedCondition.waitRelative(
                        mLock, delay_us * 1000ll);

                if (!timeoutCapped && err == -ETIMEDOUT) {
                    // We finally hit the time this event is supposed to
                    // trigger.
                    now_us = getRealTimeUs();
                    break;
                }
            }

            // The event w/ this id may have been cancelled while we're
            // waiting for its trigger-time, in that case
            // removeEventFromQueue_l will return NULL.
            // Otherwise, the QueueItem will be removed
            // from the queue and the referenced event returned.
            event = removeEventFromQueue_l(eventID);
        }

        if (event != NULL) {
            // Fire event with the lock NOT held.
            event->fire(this, now_us);
        }
    }

}

const NBTimedEventQueue::Event* NBTimedEventQueue::removeEventFromQueue_l(
        event_id id) {

    struct list_head *pos, *q;
    list_for_each_safe(pos, q, &mQueue) {
        QueueItem* dummyItem = list_entry(pos, struct QueueItem, list);
        if (dummyItem->event->eventID() == id) {
            const Event* event = dummyItem->event;
            event->setEventID(0);
            list_del(pos);
            delete dummyItem;
            return event;
        }
    }

    return NULL;
}
