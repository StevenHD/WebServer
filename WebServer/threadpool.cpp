/*
 * @author: hlhd
 * */

#include "threadpool.h"

/* 线程池的创建与回收 */
ThreadPool::ThreadPool(int thread_nums, int max_requests)
: _s_thread_number(thread_nums), _threadID_arr(nullptr), _s_max_requests(max_requests)
{
    if (thread_nums <= 0)
    {
        std::cout << "task numbers < 0" << std::endl;
        throw std::exception();
    }

    if (max_requests <= 0)
    {
        std::cout << "max requests < 0" << std::endl;
        throw std::exception();
    }

    /* 线程id初始化 */
    _threadID_arr = new pthread_t[_s_thread_number];
    if (!_threadID_arr)
    {
        std::cout << "_threadID_arr is NULL" << std::endl;
        throw std::exception();
    }

    /* 创建规定数量的线程 */
    for (int i = 0; i < thread_nums; i ++ )
    {
        /* 循环创建线程，并启动工作线程execute */
        if (pthread_create(&_threadID_arr[i], nullptr, execute, this))
        {
            delete [] _threadID_arr;
            std::cout << "pthread create error" << std::endl;
            throw std::exception();
        }

        /* 将线程设置成分离态，这样就不用单独对工作线程进行回收了 */
        if (pthread_detach(_threadID_arr[i]))
        {
            delete[] _threadID_arr;
            throw std::exception();
        }
    }
}

ThreadPool::~ThreadPool()
{
    delete [] _threadID_arr;
//    _terminate = true;
//    _tasks_CondVar.broadcast();
}

/* 向任务队列中添加任务请求 */
bool ThreadPool::append_Task(Task* task, int state)
{
    _tasks_Mutex.mutex_lock();

    /* 提前设置好任务队列的最大容量 */
    if (_task_Queue.size() >= _s_max_requests)
    {
        _tasks_Mutex.mutex_unlock();
        return false;
    }

    task->_task_state = state;

    /* 添加任务 */
    _task_Queue.push(task);
    _tasks_Mutex.mutex_unlock();

    // bool _isNULL = _task_Queue.empty();
    // if (_isNULL) _tasks_CondVar.signal();
    /* 信号量提醒有任务要处理 */
    _taskqueue_Stat.post();
    return true;
}

/* 线程处理函数 */
void* ThreadPool::execute(void *arg)
{
    /* 将参数arg强转为线程池类，从而调用成员方法run() */
    ThreadPool* pool = (ThreadPool*)arg;
    pool->run();
    return pool;
}

/* run()执行任务请求，使用条件变量 */
void run()
{
//    while (!_terminate)
//    {
//        Task * cur_task = get_Task();
//        if (!cur_task) _tasks_CondVar.wait();
//        else
//        {
//            cur_task->execute_Task();
//            delete cur_task;
//        }
//    }
}

/* run()执行任务请求，使用信号量 */
void ThreadPool::run()
{
    while (!_terminate)
    {
        /* 信号量等待 */
        _taskqueue_Stat.wait();

        /* 被唤醒后先加互斥锁 */
        _tasks_Mutex.mutex_lock();
        if (_task_Queue.empty())
        {
            _tasks_Mutex.mutex_unlock();
            continue;
        }

        /* 从任务队列中取出第一个任务 */
        /* 取出后删除该任务 */
        Task* task = _task_Queue.front();
        _task_Queue.pop();
        _tasks_Mutex.mutex_unlock();

        if (!task) continue;

        /* execute_Task()进行处理 */
        task->execute_Task();
    }
}

Task* ThreadPool::get_Task()
{
    Task* cur_task = nullptr;

    _tasks_Mutex.mutex_lock();
    if (!_task_Queue.empty())
    {
        cur_task = _task_Queue.front();
        _task_Queue.pop();
    }
    _tasks_Mutex.mutex_unlock();

    return cur_task;
}


