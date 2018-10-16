//
// Created by parallels on 9/8/18.
//

#ifndef NBMUTEX_H
#define NBMUTEX_H

#include <stdlib.h>
#include <pthread.h>
#include <NBMacros.h>

class NBCondition;

/*
 * Simple mutex class.  The implementation is system-dependent.
 *
 * The mutex must be unlocked by the thread that locked it.  They are not
 * recursive, i.e. the same thread can't lock it multiple times.
 */
class NBMutex {
public:
    enum {
        PRIVATE = 0,
        SHARED = 1
    };

    NBMutex();
    NBMutex(const char* name);
    NBMutex(int type, const char* name = NULL);
    ~NBMutex();

    // lock or unlock the mutex
    nb_status_t    lock();
    void        unlock();

    // lock if possible; returns 0 on success, error otherwise
    nb_status_t    tryLock();

    // Manages the mutex automatically. It'll be locked when Autolock is
    // constructed and released when Autolock goes out of scope.
    class NBAutolock {
    public:
        inline NBAutolock(NBMutex& mutex) : mLock(mutex)  { mLock.lock(); }
        inline NBAutolock(NBMutex* mutex) : mLock(*mutex) { mLock.lock(); }
        inline ~NBAutolock() { mLock.unlock(); }
    private:
        NBMutex& mLock;
    };

private:
    friend class NBCondition;

    // A mutex cannot be copied
    NBMutex(const NBMutex&);
    NBMutex&      operator = (const NBMutex&);

    pthread_mutex_t mMutex;
};

// ---------------------------------------------------------------------------

inline NBMutex::NBMutex() {
    pthread_mutex_init(&mMutex, NULL);
}

inline NBMutex::NBMutex(const char* name) {
    pthread_mutex_init(&mMutex, NULL);
}

inline NBMutex::NBMutex(int type, const char* name) {
    if (type == SHARED) {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
        pthread_mutex_init(&mMutex, &attr);
        pthread_mutexattr_destroy(&attr);
    } else {
        pthread_mutex_init(&mMutex, NULL);
    }
}

inline NBMutex::~NBMutex() {
    pthread_mutex_destroy(&mMutex);
}

inline nb_status_t NBMutex::lock() {
    return -pthread_mutex_lock(&mMutex);
}

inline void NBMutex::unlock() {
    pthread_mutex_unlock(&mMutex);
}

inline nb_status_t NBMutex::tryLock() {
    return -pthread_mutex_trylock(&mMutex);
}

// ---------------------------------------------------------------------------

/*
 * Automatic mutex.  Declare one of these at the top of a function.
 * When the function returns, it will go out of scope, and release the
 * mutex.
 */

typedef NBMutex::NBAutolock NBAutoMutex;

#endif //NBMUTEX_H
