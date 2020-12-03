/*
 * @author: hlhd
 */

#ifndef WEBSERVER_TASK_H
#define WEBSERVER_TASK_H

#include <iostream>
#include <string>
#include <cstring>

#include <vector>
#include <sstream>
#include <iterator>
#include <fstream>

#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <netinet/in.h>


const int BUFFER_SIZE = 1024;

void http_parser(int& client_fd, char * msg);
void remove_FD(int epoll_fd, int fd);
void reset_ONESHOT(int epoll_fd, int fd);

class Task
{
public:
    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 1024;

    /* 读为0, 写为1 */
    int _task_state;

private:
    int _task_socketfd;
    int _task_epollfd;

    sockaddr_in _task_address;
    char _task_read_buf[READ_BUFFER_SIZE];
    char _task_write_buf[WRITE_BUFFER_SIZE];

public:
    Task() {}
    Task(int fd, int epollfd) : _task_socketfd(fd), _epoll_fd(epollfd) {}
    ~Task()
    {
        remove_FD(_task_epollfd, _task_socketfd);
    }
    void init(int sockfd, const sockaddr_in &addr);

    void execute_Task();
    int send_File(const std::string& filename, const char *type,
                    int start, const int statusCode = 200, const char* statusDesc = "OK");
    void parseGET(const std::string& uri, int start = 0);

};

#endif //WEBSERVER_TASK_H
