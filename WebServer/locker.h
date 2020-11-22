/*
 * @author: hlhd
 * */

#ifndef WEBSERVER_LOCKER_H
#define WEBSERVER_LOCKER_H

#include <iostream>
#include <exception>
#include <pthread.h>

/* 线程锁 */
class Locker
{
public:
    Locker();
    ~Locker();
    bool mutex_lock();
    bool mutex_unlock();

private:
    pthread_mutex_t _mutexlock;     // 互斥锁
};

/* 条件变量 */
class ConditionVar
{
public:
    ConditionVar();
    ~ConditionVar();
    bool wait();
    bool signal();
    bool broadcast();
private:
    pthread_mutex_t _mutex;
    pthread_cond_t _condvar;
};

#endif //WEBSERVER_LOCKER_H
