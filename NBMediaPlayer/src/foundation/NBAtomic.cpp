//
// Created by parallels on 9/8/18.
//

#include "NBAtomic.h"
#include <stdlib.h>

int32_t nb_atomic_inc(int32_t* val) {
    return __atomic_fetch_add(val, 1, __ATOMIC_SEQ_CST);
}

int32_t nb_atomic_dec(int32_t* val) {
    return __atomic_fetch_sub(val, 1, __ATOMIC_SEQ_CST);
}