#include <stdio.h>
#include <unistd.h>
#include "uthread.h"

#define THREAD_NUM 2

void* routine(void* arg)
{
    int err;
    const char* msg = (const char*)arg;
    for (int i = 0; i < 1000; i++)
    {
        printf("[%d][%d]: %s\n", uthread_self(), i, msg);
        // sleep(1);
        // printf("[%d]: yielding to next thread\n", uthread_self());
        // uthread_yield();
        // printf("[%d]: someone yielded to me\n", uthread_self());
    }

    return arg;
}

int main(void)
{
    uthread_t tid[THREAD_NUM];
    const char* args[] = { "hello", "world" };
    int err;
    for (int i = 0; i < THREAD_NUM; i++)
    {
        err = uthread_create(tid + i, routine, (void*)args[i]);
        if (err == -1)
        {
            // perror
            abort();
        }
    }

    for (int i = 0; i < THREAD_NUM; i++)
    {
        void* ret;
        printf("[main]: try to join %d thread\n", tid[i]);
        err = uthread_join(tid[i], &ret);
        if (err == -1)
        {
            // perror
            abort();
        }

        printf("[main]: joined %d thread. retval: %s\n", tid[i], (const char*)ret);
    }

    printf("[main]: life is good!\n");

    err = garbage_collector();
    if (err == -1)
    {
        // perror
        abort();
    }

    return 0;
}