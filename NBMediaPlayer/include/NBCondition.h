//
// Created by parallels on 9/8/18.
//

#ifndef NBCONDITION_H
#define NBCONDITION_H

#include <NBMacros.h>
#include <pthread.h>
#include <sys/time.h>
#include "NBMutex.h"

/*
 * Condition variable class.  The implementation is system-dependent.
 *
 * Condition variables are paired up with mutexes.  Lock the mutex,
 * call wait(), then either re-wait() if things aren't quite what you want,
 * or unlock the mutex and continue.  All threads calling wait() must
 * use the same mutex for a given Condition.
 */
class NBCondition {
public:
    enum {
        PRIVATE = 0,
        SHARED = 1
    };

    NBCondition();
    NBCondition(int type);
    ~NBCondition();
    // Wait on the condition variable.  Lock the mutex before calling.
    nb_status_t wait(NBMutex& mutex);
    // same with relative timeout
    nb_status_t waitRelative(NBMutex& mutex, time_t reltime);
    // Signal the condition variable, allowing one thread to continue.
    void signal();
    // Signal the condition variable, allowing all threads to continue.
    void broadcast();

private:
    pthread_cond_t mCond;
};

// ---------------------------------------------------------------------------

inline NBCondition::NBCondition() {
    pthread_cond_init(&mCond, NULL);
}
inline NBCondition::NBCondition(int type) {
    if (type == SHARED) {
        pthread_condattr_t attr;
        pthread_condattr_init(&attr);
        pthread_condattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
        pthread_cond_init(&mCond, &attr);
        pthread_condattr_destroy(&attr);
    } else {
        pthread_cond_init(&mCond, NULL);
    }
}
inline NBCondition::~NBCondition() {
    pthread_cond_destroy(&mCond);
}
inline nb_status_t NBCondition::wait(NBMutex& mutex) {
    return -pthread_cond_wait(&mCond, &mutex.mMutex);
}
inline nb_status_t NBCondition::waitRelative(NBMutex& mutex, time_t reltime) {
    struct timespec ts;
    // we don't support the clocks here.
    struct timeval t;
    gettimeofday(&t, NULL);
    ts.tv_sec = t.tv_sec;
    ts.tv_nsec= t.tv_usec*1000;
    ts.tv_sec += reltime/1000000000;
    ts.tv_nsec+= reltime%1000000000;
    if (ts.tv_nsec >= 1000000000) {
        ts.tv_nsec -= 1000000000;
        ts.tv_sec  += 1;
    }
    return -pthread_cond_timedwait(&mCond, &mutex.mMutex, &ts);
}
inline void NBCondition::signal() {
    pthread_cond_signal(&mCond);
}
inline void NBCondition::broadcast() {
    pthread_cond_broadcast(&mCond);
}

#endif //NBCONDITION_H
