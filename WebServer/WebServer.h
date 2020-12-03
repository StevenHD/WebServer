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

const int TIMESLOT = 5;             //最小超时单位
const int MAX_FD = 65536;           //最大文件描述符
const int MAX_EVENT_NUMBER = 10000; //最大事件数
static Sort_Timer_List timer_lst;

class WebServer
{
public:
    WebServer(int port);
    ~WebServer();

    int go();
    void eventListen();

    void timer(int connfd, struct sockaddr_in client_addr);
    void adjust_timer(List_Timer* timer);

    bool dealwith_newConnection();
    bool dealwith_signal(bool &timeout, bool &stop_server);
    void dealwith_read(int sockfd);

public:

    /* 基础 */
    int _s_port;
    int _s_listenfd;
    int _s_epollfd;
    int _s_pipefd[2];
    struct sockaddr_in server_addr;
    Task* _task_users;

    /* epoll相关 */
    epoll_event _s_events[MAX_EVENT_NUMBER];

    /* 定时器相关 */
    Client_Data* _users_timer;
    Utils _utils;
};

#endif //WEBSERVER_WEBSERVER_H
