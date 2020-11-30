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

const int TIMESLOT = 5;
const int FD_LIMIT = 65535;
static Sort_Timer_List timer_lst;

class WebServer
{
private:
    int _port;
    int _socketFD;
    int _epollFD;
    struct sockaddr_in server_addr;

    int _m_pipefd[2];

public:
    WebServer(int port);
    ~WebServer();

    int go();
    void eventListen();
    void timer(int connfd, struct sockaddr_in client_addr);

public:
    /* 定时器相关 */
    Client_Data* _users_timer;
    Utils _utils;
};

#endif //WEBSERVER_WEBSERVER_H
