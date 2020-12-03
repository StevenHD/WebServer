#include <stdio.h>
#include <string.h>
#include <pthread.h>

static int value = 0;
static pthread_mutex_t mutex;

void* mythread(void* arg)
{
    int i, tmp;
    for (i = 0; i < 1000000; i ++ )
    {
        pthread_mutex_lock(&mutex);
        tmp = value;
        tmp ++;
        value = tmp;
        pthread_mutex_unlock(&mutex);
    }
    printf("pthread ID %lx value %d\n", pthread_self(), value);
    return (void*)0;
}

int main(int argc, char** argv)
{
    pthread_t tid0, tid1;
    pthread_mutex_init(&mutex, NULL);

    pthread_create(&tid0, NULL, mythread, (void*)0);
    pthread_create(&tid1, NULL, mythread, (void*)1);

    pthread_join(tid0, NULL);
    pthread_join(tid1, NULL);

    pthread_mutex_destroy(&mutex);
    return 0;
}