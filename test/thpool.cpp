///*
// * @author: hlhd
// */
//
//#include "thpool.h"
//
//template <typename T>
//ThreadPool<T>::ThreadPool( int thread_nums ) : _thread_number(thread_nums), _threads_arr(NULL), _stop(false)
//{
//    if (_thread_number < 0)
//    {
//        std::cout << "thread numbers < 0" << std::endl;
//        throw std::exception();
//    }
//
//    _threads_arr = new pthread_t[_thread_number];
//    if (_threads_arr == NULL)
//    {
//        std::cout << "_threads_arr is NULL" << std::endl;
//        throw std::exception();
//    }
//
//    for (int i = 0; i < _thread_number; i ++ )
//    {
//        if (pthread_create(&_threads_arr[i], NULL, worker, this) != 0)
//        {
//            delete[] _threads_arr;
//            std::cout << "pthread_create error" << std::endl;
//            throw std::exception();
//        }
//
//        if (pthread_detach(_threads_arr[i]))
//        {
//            delete [] _threads_arr;
//            std::cout << "pthread_detach error" << std::endl;
//            throw std::exception();
//        }
//    }
//}
//
//template <typename T>
//ThreadPool<T>::~ThreadPool()
//{
//    delete [] _threads_arr;
//    _stop = true;
//    _condvar_taskQueue.broadcast();
//}
//
///* 添加任务时需要先上锁，并判断队列是否为空 */
//template <typename T>
//bool ThreadPool<T>::append_task(T *task)
//{
//    _locker_taskQueue.mutex_lock();
//    bool st = _task_queue.empty();
//    _task_queue.push(task);
//    _locker_taskQueue.mutex_unlock();
//
//    if (st) _condvar_taskQueue.signal();
//    return true;
//}
//
//template<typename T>
//void * ThreadPool<T>::worker(void * arg)
//{
//    ThreadPool *pool = (ThreadPool*)arg;
//    pool->run();
//    return pool;
//}
//
//template<typename T>
//T* ThreadPool<T>::get_Task()
//{
//    T* cur_task = NULL;
//
//    _locker_taskQueue.mutex_lock();
//    if (_task_queue.size())
//    {
//        std::cout << "length = " << _task_queue.size() << std::endl;
//        cur_task = _task_queue.front();
//        _task_queue.pop();
//        std::cout << "length = " << _task_queue.size() << std::endl;
//    }
//    _locker_taskQueue.mutex_unlock();
//
//    return cur_task;
//}
//
//template<typename T>
//void ThreadPool<T>::run()
//{
//    while (_stop == false)
//    {
//        T* cur_task = get_Task();
//        if (cur_task == NULL) _condvar_taskQueue.wait();
//        else
//        {
//            cur_task->doit();
//            delete cur_task;
//        }
//    }
//}