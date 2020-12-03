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
void task_resetONESHOT(int epoll_fd, int fd)
{
    epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev);
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

void Task::execute_Task()
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