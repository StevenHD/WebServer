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

class Tasks
{
public:
    Tasks();
    Tasks(int fd, int epollfd);
    ~Tasks();

    void removeFD(int epoll_fd, int fd);
    void resetONESHOT(int epoll_fd, int fd);
    void execute_Task();
    void http_parser(int _connect_fd, char* read_buf);
    int sendFile(const std::string& filename, const char *type,
                    int start, const int statusCode = 200, const char* statusDesc = "OK");

    int get_Size(const std::string& filename);
    void parseGET(const std::string& uri, int start = 0);
    void parsePOST(const std::string& uri, char *buf);

private:
    int _connect_fd;
    int _epoll_fd;
};

#endif //WEBSERVER_TASK_H
