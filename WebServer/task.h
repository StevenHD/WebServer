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
    /* 设置读缓冲区_task_read_buf大小 */
    static const int READ_BUFFER_SIZE = 2048;

    /* 设置写缓冲区_task_write_buf大小 */
    static const int WRITE_BUFFER_SIZE = 1024;

    /* 设置读取文件的名称_real_file大小 */
    static const int FILENAME_LEN = 200;

    /* 读为0, 写为1 */
    int _task_state;

public:
    /* 报文的请求方法 */
    enum METHOD {GET = 0, POST};

    /* 报文解析的结果 */
    enum HTTP_CODE{NO_REQUEST,GET_REQUEST,BAD_REQUEST,NO_RESOURCE,FORBIDDEN_REQUEST,FILE_REQUEST,INTERNAL_ERROR,CLOSED_CONNECTION};

    /* 主状态机的状态 */
    enum CHECK_STATE{CHECK_STATE_REQUESTLINE=0,CHECK_STATE_HEADER,CHECK_STATE_CONTENT};

    /* 从状态机的状态 */
    enum LINE_STATUS{LINE_OK=0,LINE_BAD,LINE_OPEN};

public:
    Task() {}
    Task(int fd, int epollfd) : _task_socketfd(fd), _task_epollfd(epollfd) {}
    ~Task()
    {
        remove_FD(_task_epollfd, _task_socketfd);
    }

public:
    /* 初始化套接字地址和http连接 */
    void init(int sockfd, const sockaddr_in &addr);

    /* 关闭http连接 */
    void close_conn(bool close = true);

    void process_Task();

    /* 读取浏览器端发来的全部数据 */
    bool read_data();

    /* 响应报文写入函数 */
    bool write_data();

    sockaddr_in *get_addr()
    {
        return &_task_address;
    }

    int send_File(const std::string& filename, const char *type,
                    int start, const int statusCode = 200, const char* statusDesc = "OK");
    void parseGET(const std::string& uri, int start = 0);

private:
    /* 从_task_read_buf读取，并处理请求报文 */
    HTTP_CODE process_read();

    /* 向_task_write_buf写入响应报文数据 */
    bool process_write();

    /* 主状态机解析报文中的请求行数据 */
    HTTP_CODE parse_request_line(char* text);

    /* 主状态机解析报文中的请求头数据 */
    HTTP_CODE parse_request_header(char* text);

    /* 主状态机解析报文中的请求内容 */
    HTTP_CODE parse_request_content(char* text);

    /* 生成响应报文 */
    HTTP_CODE do_request();

    /* get_line用于将指针向后偏移，指向未处理的字符 */
    /* _start_line是行在buffer中的起始位置，
     * 将该位置后面的数据赋给text,
     * 此时从状态机已提前将一行的末尾字符\r\n变为\0\0
     * 所以text可以直接取出完整的行进行解析*/
    char* get_line()
    {
        return _task_read_buf + _start_line;
    }

    /* 从状态机读取一行，分析是请求报文的哪一部分 */
    LINE_STATUS parse_line();

    /* 根据响应报文格式，生成对应8个部分
     * 以下函数均由do_request调用 */
    bool add_response(const char* format);
    bool add_content(const char* content);
    bool add_status_line(int status,const char* title);
    bool add_headers(int content_length);
    bool add_content_type();
    bool add_content_length(int content_length);
    bool add_linger();
    bool add_blank_line();

private:
    int _task_socketfd;
    int _task_epollfd;

    sockaddr_in _task_address;
    char _task_read_buf[READ_BUFFER_SIZE];
    char _task_write_buf[WRITE_BUFFER_SIZE];

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
    char _read_filename[FILENAME_LEN];
    char* _url;
    char* _version;
    char* _host;
    char* _content_len;
    bool _linger;

    /* 读取服务器上的文件地址 */
    char* _fileaddr;

    /* 缓冲区中_task_read_buf中数据的最后一个字节的下一个位置 */
    int _read_idx;

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
