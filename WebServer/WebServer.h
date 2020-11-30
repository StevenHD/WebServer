/*
 * @author: hlhd
 * */

#ifndef WEBSERVER_WEBSERVER_H
#define WEBSERVER_WEBSERVER_H

#include <netinet/in.h>
#include <cassert>
#include "threadpool.h"

const int TIMESLOT = 5;

void add_FD(int epoll_fd, int fd, bool oneshot, struct epoll_event epev);
void add_Sig(int sig, void(handler)(int), bool restart);
int set_NonBlocking(int fd);

class WebServer
{
private:
    int _port;
    int _socketFD;
    int _epollFD;
    struct sockaddr_in server_addr;
    static int pipefd[2];

public:
    WebServer(int port);
    ~WebServer();

    int go();
    void createListenFD();
    void timer(int connfd, struct sockaddr_in client_addr);

};

#endif //WEBSERVER_WEBSERVER_H
