/*
 * @author: hlhd
 * */

#ifndef WEBSERVER_WEBSERVER_H
#define WEBSERVER_WEBSERVER_H

#include <netinet/in.h>

#include "threadpool.h"

void add_FD(int epoll_fd, int fd, bool oneshot, struct epoll_event epev);
int set_NonBlocking(int fd);

class WebServer
{
private:
    int _port;
    int _socketFD;
    int _epollFD;
    struct sockaddr_in server_addr;

public:
    WebServer(int port) : _port(port) { bzero(&server_addr, sizeof(server_addr)); }
    ~WebServer() { close(_socketFD); }
    int go();
    void createListenFD();

};

#endif //WEBSERVER_WEBSERVER_H
