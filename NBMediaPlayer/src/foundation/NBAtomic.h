//
// Created by parallels on 9/8/18.
//

#ifndef NBATOMIC_H
#define NBATOMIC_H

#include <stdint.h>

int32_t nb_atomic_inc(int32_t* val);
int32_t nb_atomic_dec(int32_t* val);

#endif //NBATOMIC_H
