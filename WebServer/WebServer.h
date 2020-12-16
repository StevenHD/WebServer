/*
 * @author: hlhd
 */

#ifndef WEBSERVER_WEBSERVER_H
#define WEBSERVER_WEBSERVER_H

#include <netinet/in.h>
#include <netinet/in.h>
#include <cassert>
#include "threadpool.h"
#include "List_Timer.h"

const int TIMESLOT = 5;             // 最小超时单位
const int MAX_FD = 65536;           // 最大文件描述符
const int MAX_EVENT_NUMBER = 10000; // 最大事件数
static Sort_Timer_List timer_lst;   // 定时器相关参数

class WebServer
{
public:
    WebServer(int port);
    ~WebServer();

    void init(int port, int thread_num);
    void thread_pool();
    void event_listen();
    void event_loop();
    void timer(int connfd, struct sockaddr_in client_addr);
    void adjust_timer(List_Timer* timer);

    void dealwith_timer(List_Timer* timer, int socketfd);
    bool dealwith_clntdata();
    bool dealwith_signal(bool &timeout, bool &stop_server);
    void dealwith_read(int sockfd);
    void dealwith_write(int sockfd);

public:

    /* 基础 */
    int _s_port;
    char* _s_root;

    int _s_pipefd[2];
    int _s_epollfd;
    Task* _users;

    /* 线程池相关 */
    ThreadPool* _pool;
    int _s_thread_num;

    /* epoll相关 */
    epoll_event _s_events[MAX_EVENT_NUMBER];
    int _s_listenfd;

    /* 定时器相关 */
    Utils _utils;

    // 每个user（http请求）对应的timer
    Client_Data* _usrs_timer;
};

#endif //WEBSERVER_WEBSERVER_H
