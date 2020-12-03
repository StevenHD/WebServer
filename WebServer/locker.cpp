/*
 * @author: hlhd
 * */

#include "locker.h"

Sem::Sem()
{
    /* 信号量初始化 */
    if (sem_init(&_sem, 0, 0))
    {
        throw std::exception();
    }
}

Sem::Sem(int num)
{
    if (sem_init(&_sem, 0, num))
    {
        throw std::exception();
    }
}

Sem::~Sem()
{
    /* 信号量销毁 */
    sem_destroy(&_sem);
}

bool Sem::wait()
{
    return sem_wait(&_sem) == 0;
}

bool Sem::post()
{
    return sem_post(&_sem) == 0;
}

Locker::Locker()
{
    if (pthread_mutex_init(&_mutexlock, NULL) != 0)
    {
        std::cout << "mutex_lock init error.." << std::endl;
        throw std::exception();
    }
}

Locker::~Locker()
{
    pthread_mutex_destroy(&_mutexlock);
}

bool Locker::mutex_lock()
{
    return pthread_mutex_lock(&_mutexlock) == 0;
}

bool Locker::mutex_unlock()
{
    return pthread_mutex_unlock(&_mutexlock) == 0;
}

pthread_mutex_t* Locker::get()
{
    return &_mutexlock;
}


/* Conditional Variable */

ConditionVar::ConditionVar()
{
    if (pthread_mutex_init(&_mutex, NULL) != 0)
    {
        throw std::exception();
    }

    if (pthread_cond_init(&_condvar, NULL) != 0)
    {
        pthread_cond_destroy(&_condvar);
        throw std::exception();
    }
}

ConditionVar::~ConditionVar()
{
    pthread_mutex_destroy(&_mutex);
    pthread_cond_destroy(&_condvar);
}

/* cond要与mutex搭配使用，避免造成共享数据的混乱 */
bool ConditionVar::wait()
{
    int ret = 0;

    /* 内部会有一次加锁和解锁 */
    pthread_mutex_lock(&_mutex);
    ret = pthread_cond_wait(&_condvar, &_mutex);
    pthread_mutex_unlock(&_mutex);

    return ret == 0;
}

/* 唤醒等待该条件变量的某一个线程 */
bool ConditionVar::signal()
{
    return pthread_cond_signal(&_condvar) == 0;
}

/* 唤醒所有等待该条件变量的线程 */
bool ConditionVar::broadcast()
{
    return pthread_cond_broadcast(&_condvar) == 0;
}

