//
// Created by hlhd on 2020/11/20.
//

#ifndef WEBSERVER_THREADPOOL_H
#define WEBSERVER_THREADPOOL_H

#include <iostream>
#include <pthread.h>
#include <queue>
#include "locker.h"
#include "task.h"

// execute queue
class Workers
{
public:
    Workers();
    int getID();

private:
    pthread_t _thread_id;

    Workers* prev;
    Workers* next;
};

class ThreadPool
{
public:
    ThreadPool(int thread_nums);
    ~ThreadPool();
    void append_Task(Tasks* task);


private:
    int _task_nums;
    bool _terminate;

    pthread_t * _threadID_arr;
    std::queue<Tasks*> _task_Queue;

    Locker _tasks_Mutex;
    ConditionVar _tasks_CondVar;

    static void* execute(void*);

    void run();
    Tasks* get_Task();
};



#endif //WEBSERVER_THREADPOOL_H
