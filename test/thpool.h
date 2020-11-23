///*
// * @author: hlhd
// *
// */
//
//#ifndef WEBSERVER_THPOOL_H
//#define WEBSERVER_THPOOL_H
//
//#include <queue>
//#include "locker.h"
//
//template <typename T>
//class ThreadPool
//{
//public:
//    ThreadPool( int thread_nums = 20 );
//    ~ThreadPool();
//    bool append_task(T *task);
//
//private:
//    int _thread_number;                     //线程池中的线程数
//    pthread_t *_threads_arr;                //线程数组
//    std::queue<T*> _task_queue;             //任务队列
//    Locker _locker_taskQueue;               //任务队列的互斥锁
//    ConditionVar _condvar_taskQueue;        //任务队列的条件变量
//    bool _stop;                             //是否结束线程
//
//    static void* worker(void*);
//    void run();
//    T* get_Task();
//};
//
//#endif //WEBSERVER_THPOOL_H
