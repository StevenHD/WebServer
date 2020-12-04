/*
 * @author: hlhd
 */

#include "task.h"

int task_Setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

/* 将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT */
void task_addfd(int epollfd, int fd, bool one_shot)
{
    epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN | EPOLLET;
    if (one_shot) ev.events |= EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
    task_Setnonblocking(fd);
}

/* 从内核时间表删除描述符 */
void task_removefd(int epollfd, int fd)
{
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

/* 将事件重置为EPOLLONESHOT */
void task_resetONESHOT(int epoll_fd, int fd, int TRIGMode)
{
    epoll_event ev;
    ev.data.fd = fd;

    if (TRIGMode == EPOLLIN)
        ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
    else if (TRIGMode == EPOLLOUT)
        ev.events = EPOLLOUT | EPOLLONESHOT;

    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev);
}

/* 循环读取客户数据，直到无数据可读或对方关闭连接 */
bool Task::read_data()
{
    if (_read_idx >= READ_BUFFER_SIZE) return false;

    int bytes_read = 0;
    while (true)
    {
        /* 从套接字接收数据，存储在_task_read_buf缓冲区 */
        bytes_read = recv(_task_socketfd,
                          _task_read_buf + _read_idx,
                          READ_BUFFER_SIZE - _read_idx,
                          0);
        if (bytes_read == -1)
        {
            /* 非阻塞ET模式下，需要一次性将数据读完 */
            if (errno == EAGAIN) break;
            return false;
        }
        else if (bytes_read == 0)
        {
            return false;
        }

        /* 修改_read_idx的读取字节数 */
        _read_idx += bytes_read;
    }

    return true;
}

Task::HTTP_CODE Task::process_read()
{
    /* 初始化[从状态机]的状态和HTTP请求解析结果 */
    LINE_STATUS _lineStatus = LINE_OK;
    HTTP_CODE ret = NO_REQUEST;
    char* text = nullptr;

    /* parse_line为[从状态机]的具体实现 */
    while ((_check_state == CHECK_STATE_CONTENT && _lineStatus == LINE_OK) ||
            (_lineStatus = parse_line()) == LINE_OK)
    {
        text = get_line();

        /* _start_line是每一个数据行在_read_buf中的起始位置 */
        /* _checked_idx表示[从状态机]在_read_buf中读取的位置 */
        _start_line = _checked_idx;

        /* 主状态机的三种状态转移逻辑 */
        switch (_check_state) {
            case CHECK_STATE_REQUESTLINE:
            {
                /* 解析请求行 */
                ret = parse_request_line(text);
                if (ret == BAD_REQUEST)
                    return BAD_REQUEST;
                break;
            }

            case CHECK_STATE_HEADER:
            {
                /* 解析请求头 */
                ret = parse_request_header(text);
                if (ret == BAD_REQUEST)
                    return BAD_REQUEST;
                /* 完整解析GET请求后，跳转到报文响应函数 */
                else if (ret == GET_REQUEST)
                    return do_request();
                break;
            }

            case CHECK_STATE_CONTENT:
            {
                /* 解析消息体 */
                ret = parse_request_content(text);

                /* 完整解析POST请求后，跳转到报文响应函数 */
                if (ret == GET_REQUEST)
                    return do_request();

                /* 解析完消息体即完成报文解析
                 * 避免再次进入循环，更新line_status */
                _lineStatus = LINE_OPEN;
                break;
            }

            default:
            return INTERNAL_ERROR;
        }
    }

    return NO_REQUEST;
}

/* [从状态机]——用于分析出一行内容
 * 返回值为行的读取状态，有LINE_OK,LINE_BAD,LINE_OPEN */
Task::LINE_STATUS Task::parse_line()
{
    /* _read_idx指向缓冲区m_read_buf的数据末尾的下一个字节 */
    /* _checked_idx指向[从状态机]当前正在分析的字节 */
    char temp;
    for (; _checked_idx < _read_idx; _checked_idx ++ )
    {
        /* temp为将要分析的字节 */
        temp = _task_read_buf[_checked_idx];

        /* 如果当前是\r字符，则有可能会读取到完整行 */
        if (temp == '\r')
        {

        }
    }
}

void Task::process_Task()
{
    HTTP_CODE read_ret = process_read();

    /* NO_REQUEST，表示请求不完整，需要继续接收请求数据 */
    if (read_ret == NO_REQUEST)
    {
        /* 注册并监听读事件 */
        task_resetONESHOT(_task_epollfd, _task_socketfd, EPOLLIN);
        return;
    }

    /* 调用process_write完成报文响应 */
    bool write_ret = process_write(read_ret);
    if (!write_ret) close_conn();

    /* 注册并监听写事件 */
    task_resetONESHOT(_task_epollfd, _task_socketfd, EPOLLOUT);
}



void http_parser(int& client_fd, char * msg)
{
    // Parse out the client's request string e.g. GET /index.html HTTP/1.1
    std::istringstream iss(msg);
    std::vector<std::string> parsed((std::istream_iterator<std::string>(iss)), std::istream_iterator<std::string>());

    std::string content = "404 Not Found";
    std::string filename = "/index.html";
    int errorCode = 404;

    if (parsed.size() >= 3 && parsed[0] == "GET")
    {
        filename = parsed[1];
        if (filename == "/") filename = "/index.html";
    }

    // Open the document in the local file system
    std::ifstream f("." + filename);

    // Check if it opened and if it did, grab the entire contents
    if (f.good())
    {
        std::string str_file((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        content = str_file;
        errorCode = 200;
    }
    f.close();

    // Write the document back to the client
    std::ostringstream oss;
    oss << "HTTP/1.1 " << errorCode << " OK\r\n";
    oss << "Cache-Control: no-cache, private\r\n";
    oss << "Content-Type: text/html\r\n";
    oss << "Content-Length: " << content.size() << "\r\n";
    oss << "\r\n";
    oss << content;

    std::string output = oss.str();
    int size = output.size() + 1;

    send(client_fd, output.c_str(), size, 0);
}

void Task::init(int sockfd, const sockaddr_in &addr)
{
    _task_socketfd = sockfd;
    _task_address = addr;
    task_addfd(_task_epollfd, _task_socketfd, true);
    memset(_task_read_buf, '\0', READ_BUFFER_SIZE);
    memset(_task_write_buf, '\0', WRITE_BUFFER_SIZE);
}

void Task::process_Task()
{
    /* 读数据 */
    /* 初始化缓冲区，用来接收客户端发送的数据 */
    /* 处理客户端发来的请求 */
    char in_buf[1024] = {0};
    int readData_len;

    /* 循环读数据 */
    while ((readData_len = recv(_task_socketfd, in_buf, sizeof(in_buf), 0)) > 0)
    {
        /* 将数据打印到终端 */
        printf("The client's request is: %s\n", in_buf);

        /* 发送给客户端 */
        int start_id = 0;
        char method[8], uri[1024], version[16];
        http_parser(_task_socketfd, in_buf);
    }
    //close(_connect_fd);

    if (readData_len == 0)
    {
        printf("客户端断开了连接..");
    }
    else if (readData_len == -1)
    {
        if (errno == EAGAIN)
        {
            reset_ONESHOT( _task_epollfd, _task_socketfd );
            printf("缓冲区数据已经读完了\n");
        }
        else
        {
            printf("recv error\n");
            exit(-1);
        }
    }
}

int Task::send_File(const std::string& filename, const char * type, int start, const int statusCode, const char* statusDesc)
{
    struct stat file_status;
    int ret = stat(filename.c_str(), &file_status);
    if (ret < 0 || !S_ISREG(file_status.st_mode))
    {
        std::cout << "file not found: " << filename << std::endl;
        send_File("html/404.html", "text/html", 0, 404, "Not Found");
        return -1;
    }

    char response_header[256];
    sprintf(response_header, "HTTP1.1 %d %s\r\nServer: StevenHD\r\nContent-Length: %d\r\nContent-Type: %s;charset:utf-8\r\n\r\n",
            statusCode, statusDesc, int(file_status.st_size - start), type);

    send(_task_socketfd, response_header, strlen(response_header), 0);
    int send_fd = open(filename.c_str(), O_RDONLY);
    int sum = start;

    while (sum < file_status.st_size)
    {
        off_t t = sum;
        int recv_len = sendfile(_task_socketfd, send_fd, &t, file_status.st_size);
        if (recv_len < 0)
        {
            printf("errno = %d, recv_len = %d\n", errno, recv_len);
            if (errno == EAGAIN)
            {
                printf("errno is EAGAIN\n");
                continue;
            }
            else
            {
                perror("send file error");
                close(send_fd);
                break;
            }
        }
        else sum += recv_len;
    }

    close(send_fd);
    return 0;
}