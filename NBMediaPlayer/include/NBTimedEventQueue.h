//
// Created by parallels on 9/9/18.
//

#ifndef NBTIMEDEVENTQUEUE_H
#define NBTIMEDEVENTQUEUE_H

#include <pthread.h>
#include <stdint.h>
#include <NBString.h>
#include <NBList.h>

#include <NBCondition.h>

struct NBTimedEventQueue {

    typedef int32_t event_id;

    struct Event {
        Event()
                : mEventID(0) {
        }

        virtual ~Event() {}

        event_id eventID() {
            return mEventID;
        }

        event_id eventID() const {
            return mEventID;
        }

    protected:
        virtual void fire(NBTimedEventQueue *queue, int64_t now_us) const = 0;

    private:
        friend class NBTimedEventQueue;

        mutable event_id mEventID;

        void setEventID(event_id id) {
            mEventID = id;
        }

        void setEventID(event_id id) const {
            mEventID = id;
        }

        Event(const Event &);
        Event &operator=(const Event &);
    };

    NBTimedEventQueue();
    ~NBTimedEventQueue();

    // Start executing the event loop.
    void start();

    // Stop executing the event loop, if flush is false, any pending
    // events are discarded, otherwise the queue will stop (and this call
    // return) once all pending events have been handled.
    void stop(bool flush = false);

    // Posts an event to the front of the queue (after all events that
    // have previously been posted to the front but before timed events).
    event_id postEvent(const Event *event);

    event_id postEventToBack(const Event *event);

    // It is an error to post an event with a negative delay.
    event_id postEventWithDelay(const Event *event, int64_t delay_us);

    // If the event is to be posted at a time that has already passed,
    // it will fire as soon as possible.
    event_id postTimedEvent(const Event *event, int64_t realtime_us);

    // Returns true iff event is currently in the queue and has been
    // successfully cancelled. In this case the event will have been
    // removed from the queue and won't fire.
    bool cancelEvent(event_id id, bool stopAtFirstMatch = true);

    // Cancel any pending event that satisfies the predicate.
    // If stopAfterFirstMatch is true, only cancels the first event
    // satisfying the predicate (if any).
    void cancelEvents(
            bool (*predicate)(void *cookie, const Event *event),
            void *cookie,
            bool e = false);

    static int64_t getRealTimeUs();
    void runEvent(event_id eventID, int64_t timeUs);

    inline void setQueueName(NBString name) {
        mThreadName = name;
    }

private:
    struct QueueItem {
        list_head list;
        const Event* event;
        int64_t realtime_us;

        QueueItem():event(NULL),
                    realtime_us(0)
        {
            INIT_LIST_HEAD(&list);
        }

        ~QueueItem() {
        }
    };

    struct StopEvent : public NBTimedEventQueue::Event {
        virtual void fire(NBTimedEventQueue *queue, int64_t now_us) const {
            queue->mStopped = true;
        }
    };

    NBString mThreadName;

    pthread_t mThread;
    struct list_head mQueue;
    NBMutex mLock;
    NBCondition mQueueNotEmptyCondition;
    NBCondition mQueueHeadChangedCondition;
    NBCondition mEventThreadStartedCondition;
    event_id mNextEventID;

    bool mRunning;
    bool mStopped;

    static void *ThreadWrapper(void *me);
    void threadEntry();

    const Event* removeEventFromQueue_l(event_id id);

    NBTimedEventQueue(const NBTimedEventQueue &);
    NBTimedEventQueue &operator=(const NBTimedEventQueue &);
};

#endif //NBTIMEDEVENTQUEUE_H
