//
// Created by parallels on 9/9/18.
//

#include "NBMediaSource.h"

NBMediaSource::NBMediaSource() {

}

NBMediaSource::~NBMediaSource() {

}

NBMediaSource::ReadOptions::ReadOptions() {
    reset();
}

void NBMediaSource::ReadOptions::reset() {
    mOptions = 0;
    mSeekTimeUs = 0;
    mLatenessUs = 0;
}

void NBMediaSource::ReadOptions::setSeekTo(int64_t time_us, SeekMode mode) {
    mOptions |= kSeekTo_Option;
    mSeekTimeUs = time_us;
    mSeekMode = mode;
}

void NBMediaSource::ReadOptions::clearSeekTo() {
    mOptions &= ~kSeekTo_Option;
    mSeekTimeUs = 0;
    mSeekMode = SEEK_CLOSEST_SYNC;
}

bool NBMediaSource::ReadOptions::getSeekTo(
        int64_t *time_us, SeekMode *mode) const {
    *time_us = mSeekTimeUs;
    *mode = mSeekMode;
    return (mOptions & kSeekTo_Option) != 0;
}

void NBMediaSource::ReadOptions::setLateBy(int64_t lateness_us) {
    mLatenessUs = lateness_us;
}

int64_t NBMediaSource::ReadOptions::getLateBy() const {
    return mLatenessUs;
}
