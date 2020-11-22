/*
 * @author: hlhd
 * */

#ifndef WEBSERVER_WEBSERVER_H
#define WEBSERVER_WEBSERVER_H

#include <iostream>
#include <netinet/in.h>
#include <strings.h>
#include <zconf.h>
#include <signal.h>
#include <arpa/inet.h>
#include <cassert>

#include "threadpool.h"
#include "task.h"


class WebServer
{
public:
    WebServer(int port);
    ~WebServer();
    int go();
    void igno_SIG(int sig, void( handler )(int), bool restart = true);
    int createListenFD(int &port);
    void add_FD(int& epoll_fd, int& fd, bool oneshot);
    void set_NonBlocking(int &fd);

private:
    int _port;
    int _socketFD;
    int _epollFD;
    struct sockaddr_in server_addr;
};

#endif //WEBSERVER_WEBSERVER_H
