#ifndef UTHREAD_H
#define UTHREAD_H

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <ucontext.h>
#include <sys/signal.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <error.h>

#define UTHREAD_MAXTHREADNUM 10
#define MAIN_THREAD_TID 0
#define STACK_SIZE 1024 * 32

// #define UTHREAD_YIELD_MODE MANUAL
#define UTHREAD_YIELD_MODE TIMED

#if UTHREAD_YIELD_MODE == TIMED
    #define UTHREAD_TIMED_YIELDING_DELAY 500
#endif

#define _INIT_THREAD_ERROR ((_uthread_info*)-1)

typedef int uthread_t;
typedef void* (*uthread_routine)(void*);

typedef struct _UthreadInfo
{
    uthread_t tid;
    uthread_routine routine;
    void* arg;
    void* retval;
    ucontext_t ctx;

    volatile int is_completed;
    volatile int is_joined;
}
_uthread_info;

int uthread_create(uthread_t* tid, uthread_routine routine, void* arg);
int uthread_join(uthread_t tid, void** retval);
void uthread_yield();
/*  Always success  */
uthread_t uthread_self();

int garbage_collector();

#endif