//
// Created by liuenbao on 18-9-12.
//

#ifndef NBCLOCKMANAGER_H
#define NBCLOCKMANAGER_H

#include <NBMacros.h>

class NBClockProvider {
public:
    NBClockProvider() {
    }
    ~NBClockProvider() {
    }

public:
    virtual int64_t getCurrentClock() = 0;
};

class NBClockManager {
public:
    typedef enum AVSyncType {
        AV_SYNC_AUDIO_MASTER, /* default choice */
        AV_SYNC_VIDEO_MASTER,
        AV_SYNC_EXTERNAL_CLOCK, /* synchronize to an external clock */
        AV_SYNC_OBJECT_MAX,
    } AVSyncType;

public:
    NBClockManager();
    ~NBClockManager();

public:
    AVSyncType getMasterSyncType();

    /* get the current master clock value */
    int64_t getMasterClock();

    int64_t getAudioClock() {
        return GetClock(AV_SYNC_AUDIO_MASTER);
    }

    int64_t getVideoClock() {
        return GetClock(AV_SYNC_VIDEO_MASTER);
    }

    int64_t getExternalClock() {
        return GetClock(AV_SYNC_EXTERNAL_CLOCK);
    }

public:
    nb_status_t registerClockProvider(AVSyncType type, NBClockProvider* clkProvider);

    nb_status_t unregisterClockProvider(AVSyncType type);

private:
    int64_t GetClock(AVSyncType type);

private:
    AVSyncType mAVSyncType;

private:
    NBClockProvider* mClkProviders[AV_SYNC_OBJECT_MAX];
};

#endif //NBCLOCKMANAGER_H
