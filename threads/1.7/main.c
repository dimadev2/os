#include <stdio.h>
#include <unistd.h>
#include "uthread.h"

#define THREAD_NUM 2

typedef struct Arg {
    const char* msg;
    int wait_time;
}
thread_arg;

void* routine(void* arg)
{
    int err;
    thread_arg* a = (thread_arg*)arg;
    int delay = a->wait_time;
    const char* msg = a->msg;
    for (int i = 0; i < 5; i++)
    {
        printf("[%d][%d]: %s\n", uthread_self(), i, msg);
        sleep(delay);
        // printf("[%d]: yielding to next thread\n", uthread_self());
        // uthread_yield();
        // printf("[%d]: someone yielded to me\n", uthread_self());
    }

    return (void*)(a->msg);
}

int main(void)
{
    uthread_t tid[THREAD_NUM];
    thread_arg args[] = {
        (thread_arg){ "hello", 1 },
        (thread_arg){ "world", 3 }
    };
    int err;
    for (int i = 0; i < THREAD_NUM; i++)
    {
        err = uthread_create(tid + i, routine, (void*)(args + i));
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