//
// Created by parallels on 9/9/18.
//

#include "NBMediaBuffer.h"

NBMediaBuffer::NBMediaBuffer(int64_t duration)
    :mDuration(duration) {
    INIT_LIST_HEAD(&list);
}

NBMediaBuffer::~NBMediaBuffer() {
}

int64_t NBMediaBuffer::getDuration() {
    return mDuration;
}
