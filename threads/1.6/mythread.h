#ifndef MYTHREAD_H
#define MYTHREAD_H

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>

#define USE_HEAP_FOR_STACK
// #define USE_MAP_FOR_STACK

#define MYTHREAD_MAXTHREADNUM 10
#define MYTHREAD_STACK_SIZE 1024 * 64

#define MYTHREAD_CREATE_JOINABLE 1
#define MYTHREAD_CREATE_DETACHED 0

#define _INIT_THREAD_ERROR ((_mythread_info*)-1)
#define _MAIN_THREAD ((_mythread_info*)0)

typedef int mythread_t;
typedef void* (*mythread_routine)(void*);

typedef struct _MythreadInfo
{
    mythread_t tid;
    void* arg;
    void* stack;
    void* retval;
    mythread_routine routine;
    int is_joinable;

    sem_t is_joined;
    sem_t is_completed;
}
_mythread_info;

typedef struct _MythreadAttr
{
    int is_joinable;
}
mythread_attr_t;

int mythread_create(mythread_t* tid, mythread_attr_t* attr, mythread_routine routine, void* arg);
int mythread_join(mythread_t tid, void** retval);
/*  Always success  */
mythread_t mythread_self();

/*  Always success  */
int mythread_attr_init(mythread_attr_t* attr);
int mythread_attr_setdetachstate(mythread_attr_t* attr, int detachstate);

#endif