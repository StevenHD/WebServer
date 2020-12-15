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
#include <stdarg.h>
#include <sys/uio.h>
#include <sys/mman.h>

void http_parser(int& client_fd, char * msg);

/* 对文件描述符设置非阻塞 */
int set_nonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

/* 从内核时间表删除描述符 */
void remove_fd(int epollfd, int fd)
{
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

/* 将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT */
void add_fd(int epollfd, int fd, bool one_shot)
{
    epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN | EPOLLET;
    if (one_shot) ev.events |= EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
    set_nonblocking(fd);
}

/* 将事件重置为EPOLLONESHOT */
void reset_oneshot(int epoll_fd, int fd, int mod)
{
    epoll_event _ev;
    _ev.data.fd = fd;
    _ev.events = mod | EPOLLET | EPOLLONESHOT;
    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &_ev);
}


class Task
{
public:
    /* 报文的请求方法 */
    enum METHOD {GET = 0, POST};

    /* 报文解析的结果 */
    enum HTTP_CODE{NO_REQUEST,GET_REQUEST,BAD_REQUEST,NO_RESOURCE,FORBIDDEN_REQUEST,FILE_REQUEST,INTERNAL_ERROR,CLOSED_CONNECTION};

    /* 主状态机的状态 */
    enum CHECK_STATE{CHECK_STATE_REQUESTLINE = 0,CHECK_STATE_HEADER,CHECK_STATE_CONTENT};

    /* 从状态机的状态 */
    enum LINE_STATUS{LINE_OK = 0,LINE_BAD,LINE_OPEN};

public:
    Task() {}
    ~Task() {}

public:

    void init(int sockfd, const sockaddr_in &addr, char *root);
    void close_conn(bool real_close = true);
    void process();
    bool read_data();
    bool write_data();
    sockaddr_in *get_addr()
    {
        return &_task_address;
    }

    //--
    int send_File(const std::string& filename, const char *type,
                    int start, const int statusCode = 200, const char* statusDesc = "OK");

    void parseGET(const std::string& uri, int start = 0);

private:
    void init_conn();
    void unmap();
    HTTP_CODE process_read();
    bool process_write(HTTP_CODE ret);

    HTTP_CODE parse_request_line(char* text);
    HTTP_CODE parse_request_header(char* text);
    HTTP_CODE parse_request_content(char* text);

    HTTP_CODE do_request();

    /* get_line用于将指针向后偏移，指向未处理的字符 */
    /* _start_line是行在buffer中的起始位置，
     * 将该位置后面的数据赋给text,
     * 此时从状态机已提前将一行的末尾字符\r\n变为\0\0
     * 所以text可以直接取出完整的行进行解析 */
    char* get_line()
    {
        return _read_buf + _start_line;
    }

    LINE_STATUS parse_line();

    /* 根据响应报文格式，生成对应8个部分
     * 以下函数均由do_request调用 */

    bool add_status_line(int status,const char* title);
    bool add_headers(int content_len);
    bool add_content_length(int content_len);
    bool add_content_type();
    bool add_linker();
    bool add_blank_line();
    bool add_content(const char* content);
    bool add_response(const char* format, ...);

public:
    /* 设置读缓冲区_task_read_buf大小 */
    static const int READ_BUFFER_SIZE = 2048;

    /* 设置写缓冲区_task_write_buf大小 */
    static const int WRITE_BUFFER_SIZE = 1024;

    /* 设置读取文件的名称_real_file大小 */
    static const int FILENAME_LEN = 200;

    static int _task_epollfd;

    static int _usr_cnt;

    /* 读为0, 写为1 */
    int _task_state;

private:
    int _task_socketfd;
    sockaddr_in _task_address;

    /* 缓冲区中_task_read_buf中数据的最后一个字节的下一个位置 */
    int _read_idx;
    int _write_idx;

    char _read_buf[READ_BUFFER_SIZE];
    char _write_buf[WRITE_BUFFER_SIZE];

    /* _task_read_buf中已经解析的字符个数 */
    int _start_line;

    /* _checked_idx表示[从状态机]在_read_buf中读取的位置 */
    int _checked_idx;



    /* 主状态机的状态 */
    CHECK_STATE _check_state;

    /* 请求方法 */
    METHOD _method;

    /* 以下为解析请求报文中对应的6个变量 */
    /* 存储读取文件的名称 */
    char _real_filename[FILENAME_LEN];
    char* _url;
    char* _version;
    char* _host;
    int _content_len;
    bool _linker;


    /* 读取服务器上的文件地址 */
    char* _file_addr;
    struct stat _file_stat;

    struct iovec _IOV[2];
    int _IOV_count;

    /* 是否启用POST */
    int cgi;

    /* 存储请求头数据 */
    char* _header_data;

    /* 剩余发送字节数 */
    int bytes_to_send;

    /* 已发送字节数 */
    int bytes_have_send;
};

#endif //WEBSERVER_TASK_H
