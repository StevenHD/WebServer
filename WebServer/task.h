/*
 * @author: hlhd
 */

#ifndef WEBSERVER_TASK_H
#define WEBSERVER_TASK_H

#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/sendfile.h>


const int BUFFER_SIZE = 1024;

void remove_FD(int epoll_fd, int fd);
void reset_ONESHOT(int epoll_fd, int fd);

class Task
{
private:
    int _connect_fd;
    int _epoll_fd;

public:
    Task() {}
    Task(int fd, int epollfd) : _connect_fd(fd), _epoll_fd(epollfd) {}
    ~Task()
    {
        remove_FD(_epoll_fd, _connect_fd);
    }

    void execute_Task();

    //void http_parser(int _connect_fd, char* read_buf);
    int send_File(const std::string& filename, const char *type,
                    int start, const int statusCode = 200, const char* statusDesc = "OK");

    void parseGET(const std::string& uri, int start = 0);
    //void parsePOST(const std::string& uri, char *buf);
};

#endif //WEBSERVER_TASK_H
