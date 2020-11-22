/*
 * @author: hlhd
 * */

#include "threadpool.h"

ThreadPool::ThreadPool(int thread_nums) : _task_nums(thread_nums), _threadID_arr(NULL), _terminate(false)
{
    if (_task_nums < 0)
    {
        std::cout << "task numbers < 0" << std::endl;
        throw std::exception();
    }

    _threadID_arr = new pthread_t[_task_nums];
    /* 创建规定数量的线程 */
    for (int i = 0; i < _task_nums; i ++ )
    {
        if (pthread_create(&_threadID_arr[i], NULL, execute, this) != 0)
        {
            delete [] _threadID_arr;
            std::cout << "pthread create error" << std::endl;
            throw std::exception();
        }
    }
}

ThreadPool::~ThreadPool()
{
    delete [] _threadID_arr;
    _terminate = true;
    _tasks_CondVar.broadcast();
}

void ThreadPool::append_Task(Tasks* task)
{
    _tasks_Mutex.mutex_lock();
    bool _isNULL = _task_Queue.empty();
    _task_Queue.push(task);
    _tasks_Mutex.mutex_unlock();

    if (_isNULL) _tasks_CondVar.signal();
}

void* ThreadPool::execute(void *arg)
{
    ThreadPool* pool = (ThreadPool*)arg;
    pool->run();
    return pool;
}

void ThreadPool::run()
{
    while (!_terminate)
    {
        Tasks * cur_task = get_Task();
        if (!cur_task) _tasks_CondVar.wait();
        else
        {
            cur_task->execute_Task();
            delete cur_task;
        }
    }
}

Tasks* ThreadPool::get_Task()
{
    Tasks* cur_task = NULL;

    _tasks_Mutex.mutex_lock();
    if (!_task_Queue.empty())
    {
        cur_task = _task_Queue.front();
        _task_Queue.pop();
    }
    _tasks_Mutex.mutex_unlock();

    return cur_task;
}


