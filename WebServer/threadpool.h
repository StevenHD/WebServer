#ifndef WEBSERVER_THREADPOOL_H
#define WEBSERVER_THREADPOOL_H

#include <pthread.h>
#include <queue>

#include "task.h"
#include "locker.h"

class ThreadPool
{
public:
    ThreadPool(int thread_nums = 16, int max_requests = 10000);
    ~ThreadPool();

    /* 向任务队列中插入任务请求 */
    bool append_Task(Task* task, int state);

private:

    /* 线程池中的线程数 */
    int _s_thread_number;

    /* 任务队列中允许的最大请求数 */
    int _s_max_requests;

    /* 描述线程池的数组 */
    /* 大小为_s_thread_number */
    pthread_t * _threadID_arr;

    /* 任务队列 */
    std::queue<Task*> _task_Queue;

    /* 保护任务队列的互斥锁 */
    Locker _tasks_Mutex;

    /* 是否结束线程 */
    bool _terminate;

    /* 是否有任务需要处理1 */
    ConditionVar _tasks_CondVar;

    /* 是否有任务需要处理2 */
    Sem _taskqueue_Stat;

private:

    /* 工作线程运行的函数 */
    /* 它不断从任务队列中取出任务请求并执行 */
    static void* execute(void*);

    void run();
    Task* get_Task();

};

#endif //WEBSERVER_THREADPOOL_H
