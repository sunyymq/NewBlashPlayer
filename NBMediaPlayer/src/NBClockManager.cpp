//
// Created by liuenbao on 18-9-12.
//

#include "NBClockManager.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <libavcodec/avcodec.h>
#include <libavutil/time.h>

#ifdef __cplusplus
}
#endif

NBClockManager::NBClockManager()
    :mAVSyncType(NBClockManager::AV_SYNC_AUDIO_MASTER) {
    memset(mClkProviders, 0, sizeof(mClkProviders));
}

NBClockManager::~NBClockManager() {

}

int64_t NBClockManager::GetClock(AVSyncType type) {
    if (mClkProviders[type] == NULL) {
        return 0;
    }
    return mClkProviders[type]->getCurrentClock();
}

NBClockManager::AVSyncType NBClockManager::getMasterSyncType() {
    if (mAVSyncType == AV_SYNC_VIDEO_MASTER) {
        if (mClkProviders[AV_SYNC_VIDEO_MASTER] != NULL)
            return AV_SYNC_VIDEO_MASTER;
        else
            return AV_SYNC_AUDIO_MASTER;
    } else if (mAVSyncType == AV_SYNC_AUDIO_MASTER) {
        if (mClkProviders[AV_SYNC_AUDIO_MASTER] != NULL)
            return AV_SYNC_AUDIO_MASTER;
        else
            return AV_SYNC_EXTERNAL_CLOCK;
    } else {
        return AV_SYNC_EXTERNAL_CLOCK;
    }
}

int64_t NBClockManager::getMasterClock()
{
    int64_t val;

    switch (getMasterSyncType()) {
        case AV_SYNC_VIDEO_MASTER:
            val = GetClock(AV_SYNC_VIDEO_MASTER);
            break;
        case AV_SYNC_AUDIO_MASTER:
            val = GetClock(AV_SYNC_AUDIO_MASTER);
            break;
        default:
            val = GetClock(AV_SYNC_EXTERNAL_CLOCK);
            break;
    }
    return val;
}

nb_status_t NBClockManager::registerClockProvider(AVSyncType type, NBClockProvider* clkProvider) {
    mClkProviders[type] = clkProvider;
    return OK;
}

nb_status_t NBClockManager::unregisterClockProvider(AVSyncType type) {
    mClkProviders[type] = NULL;
    return OK;
}
