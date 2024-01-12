#include "spinlock.h"

void spin_init(spin_t* spinlock) {
    *spinlock = ATOMIC_VAR_INIT(0);
}

int spin_lock(spin_t* spinlock) {
    static int expected = 0;
    static int desired = 1;
    // while (!atomic_compare_exchange_strong(spinlock, &expected, desired)) {}
    while (!__sync_bool_compare_and_swap(spinlock, 0, 1)) {}
    // __atomic_compare_exchange (spinlock, &expected,	&desired, 0, 5, 5);
    // while (!__atomic_compare_exchange (spinlock, &expected,	&desired, 0, 5, 5)) {}
}

int spin_unlock(spin_t* spinlock) {
    static int expected = 1;
    static int desired = 0;
    // atomic_compare_exchange_strong(spinlock, &expected, desired);
    // __atomic_compare_exchange (spinlock, &expected,	&desired, 0, 5, 5);
   __sync_bool_compare_and_swap(spinlock, 1, 0);
}
