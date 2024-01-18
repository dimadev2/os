#pragma once

#include <stdatomic.h>

typedef atomic_int spin_t;

void spin_init(spin_t* spinlock);
int spin_lock(spin_t* spinlock);
int spin_unlock(spin_t* spinlock);
