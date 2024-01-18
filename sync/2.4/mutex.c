#include "mutex.h"

static int futex(int* uaddr, int futex_op, int val, const struct timespec* timeout,
                    int* uaddr2, int val3)
{
    return syscall(SYS_futex, uaddr, futex_op, val, timeout, uaddr2, val3);
}

int mutex_init(mutex_t *mutex)
{
    mutex->owner = -1;
    mutex->lock = 0;

    return 0;
}

int mutex_lock(mutex_t *mutex)
{
    pid_t self = gettid();

    // printf("[%d]: trying to lock\n", self);

    if (mutex->owner == self)
    {
        // deadlock
        return -1;
    }

    int expected = 0;
    while (atomic_flag_test_and_set(&mutex->lock))
    { 
        expected = 0;

        // futex wait
        // printf("[%d]: not got lock, going sleep\n", self);
        // printf("owner: %d\nlock: %d\n", mutex->owner, mutex->lock);

        futex((int*)&mutex->lock, FUTEX_WAIT, 1, NULL, NULL, 0);

        // printf("[%d]: awaking from sleep\n", self);
    }

    // printf("[%d]: got lock\n", self);
    mutex->owner = self;

    return 0;
}

int mutex_unlock(mutex_t *mutex)
{
    pid_t self = gettid();

    // printf("[%d]: trying to unlock mutex\n", self);

    // wrong owner
    if (mutex->owner != self) 
    {
        // printf("[%d]: wrong owner\n", self);
        return -1;
    }

    // try to release free mutex
    if (mutex->owner == -1)
    {
        // printf("[%d]: mutex is unlocked\n", self);
        return -1;
    }

    mutex->owner = -1;
    atomic_exchange(&mutex->lock, 0);
    // futex wake
    // printf("[%d]: unlocked mutex, waking others\n", self);
    futex((int*)&mutex->lock, FUTEX_WAKE, 1, NULL, NULL, 0);

    return 0;
}
