#ifndef MUTEX_H
#define MUTEX_H

#define _GNU_SOURCE
#include <stdatomic.h>
#include <unistd.h>
#include <linux/futex.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define MUTEX_MAX_LOCKCOUNT 10

typedef struct _Mutex
{
    atomic_int lock;
    pid_t owner;
}
mutex_t;

int mutex_init(mutex_t* mutex);

int mutex_lock(mutex_t* mutex);
int mutex_unlock(mutex_t* mutex);

#endif