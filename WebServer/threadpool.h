#ifndef WEBSERVER_THREADPOOL_H
#define WEBSERVER_THREADPOOL_H

#include <pthread.h>
#include <queue>

#include "task.h"
#include "locker.h"


class ThreadPool
{
public:
    ThreadPool(int thread_nums = 16);
    ~ThreadPool();
    bool append_Task(Task* task);

private:
    int _task_nums;
    bool _terminate;

    pthread_t * _threadID_arr;
    std::queue<Task*> _task_Queue;

    Locker _tasks_Mutex;
    ConditionVar _tasks_CondVar;

private:
    static void* execute(void*);

    void run();
    Task* get_Task();

};

#endif //WEBSERVER_THREADPOOL_H
