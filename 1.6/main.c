#include <stdio.h>
#include "mythread.h"

void* routine(void* arg)
{
    const char* msg = (const char*)arg;
    mythread_t tid = mythread_self();
    for (int i = 0; i < 10; i++)
    {
        printf("[%d]: %s\n", tid, msg);
    }

    return arg;
}

int main(void)
{
    int err;
    mythread_t tid1, tid2;
    err = mythread_create(&tid1, NULL, routine, "hello");
    if (err == -1)
    {
        fprintf(stderr, "mythread_create [1] failed\n");
        abort();
    }

    mythread_attr_t attr;
    mythread_attr_init(&attr);
    err = mythread_attr_setdetachstate(&attr, MYTHREAD_CREATE_DETACHED);
    if (err == -1)
    {
        fprintf(stderr, "mythread_attr_setdetachstate failed\n");
        abort();
    }
    err = mythread_create(&tid2, &attr, routine, "world");
    if (err == -1)
    {
        fprintf(stderr, "mythread_create [2] failed\n");
        abort();
    }

    void *ret1;
    err = mythread_join(tid1, &ret1);
    if (err == -1)
    {
        fprintf(stderr, "mythread_join [1] failed\n");
        abort();
    }

    printf("ret1: %s\n", (const char*)ret1);

    return 0;
}
