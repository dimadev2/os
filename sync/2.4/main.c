#include <stdio.h>
#include <pthread.h>
#include "spinlock.h"

#define N 1000000

int sum = 0;
spin_t spin;

void* routine(void* arg)
{
    for (int i = 0; i < N; i++)
    {
        spin_lock(&spin);
        sum++;
        spin_unlock(&spin);
    }
}

int main(void)
{
    spin_init(&spin);
    pthread_t t1, t2;
    pthread_create(&t1, NULL, routine, NULL);
    pthread_create(&t2, NULL, routine, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("sum: %d\n", sum);

    return 0;
}
