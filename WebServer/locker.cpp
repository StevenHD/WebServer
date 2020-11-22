/*
 * @author: hlhd
 * */

#include "locker.h"

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
    return pthread_mutex_unlock(&_mutexlock);
}

//-------------------Conditional Variable-------------------
ConditionVar::ConditionVar()
{
    if (pthread_mutex_init(&_mutex, NULL) != 0) throw std::exception();

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
    pthread_mutex_lock(&_mutex);
    int ret = pthread_cond_wait(&_condvar, &_mutex);
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

