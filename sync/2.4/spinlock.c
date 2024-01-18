#include "spinlock.h"

void spin_init(spin_t* spinlock) {
    atomic_exchange(spinlock, 0);
}

int spin_lock(spin_t* spinlock) {
    while (atomic_flag_test_and_set(spinlock)) {  }
}

int spin_unlock(spin_t* spinlock) {
    atomic_exchange(spinlock, 0);
}
