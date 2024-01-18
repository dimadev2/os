#include <stdio.h>
#include <pthread.h>
#include "mutex.h"

#define N 10000000

int sum = 0;
mutex_t mutex;

void* routine(void* arg)
{
    for (int i = 0; i < N; i++)
    {
        mutex_lock(&mutex);
        sum++;
        mutex_unlock(&mutex);
    }
}

int main(void)
{
    mutex_init(&mutex);
    pthread_t t1, t2;
    pthread_create(&t1, NULL, routine, NULL);
    pthread_create(&t2, NULL, routine, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("sum: %d\n", sum);

    return 0;
}
