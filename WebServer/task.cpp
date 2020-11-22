/*
 * @author: hlhd
 */

#include "task.h"

Tasks::Tasks() {}

Tasks::Tasks(int fd, int epollfd) :
    _connect_fd(fd), _epoll_fd(epollfd) {}

Tasks::~Tasks()
{
    removeFD(_epoll_fd, _connect_fd);
}

void Tasks::removeFD(int epoll_fd, int fd)
{
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

void Tasks::resetONESHOT(int epoll_fd, int fd)
{
    epoll_event ev;
    ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
    ev.data.fd = fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev);
}

void Tasks::execute_Task()
{
    /* 读数据 */
    /* 初始化缓冲区，用来接收客户端发送的数据 */
    /* 处理客户端发来的请求 */
    char read_buf[1024] = {0};
    int readData_len;

    /* 循环读数据 */
    while ((readData_len = recv(_connect_fd, read_buf, sizeof(read_buf), 0)) > 0)
    {
        /* 将数据打印到终端 */
        printf("The client's request is: %s\n", read_buf);
        //write(STDOUT_FILENO, read_buf, readData_len);

        /* 发送给客户端 */
        http_parser(_connect_fd, read_buf);
    }
    close(_connect_fd);

    if (readData_len == 0)
    {
        printf("客户端断开了连接..");

        /* 将fd从epoll树上删掉 */
        epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, _connect_fd, NULL);
        close(_connect_fd);
    }
    else if (readData_len == -1)
    {
        if (errno == EAGAIN)
        {
            resetONESHOT( _epoll_fd, _connect_fd );
            printf("缓冲区数据已经读完了\n");
        }
        else
        {
            printf("recv error\n");
            exit(-1);
        }
    }
}

void Tasks::http_parser(int _connect_fd, char *read_buf)
{
    int start_id = 0;
    char method[8], uri[1024], version[16];
    sscanf(read_buf, "%s %s %s", method, uri, version);

    if (strcmp(method, "GET") == 0) parseGET(uri, start_id);
    else if (strcmp(method, "POST")) parsePOST(uri, read_buf);
    else
    {
        const char* header = "HTTP/1.1 501 Not Implemented\r\nContent-Type: text/plain;charset=utf-8\r\n\r\n";
        send(_connect_fd, header, strlen(header), 0);
    }
}

void Tasks::parseGET(const std::string &uri, int start)
{
    std::string filename = uri.substr(1);

    if (uri == "/" || uri == "/index.html") sendFile("index.html", "text/html", start);
    else if (uri.find(".jpg") != std::string::npos || uri.find(".png") != std::string::npos) sendFile(filename, "image/jpg", start);
    else if (uri.find(".html") != std::string::npos) sendFile(filename, "text/html", start);
    else if (uri.find(".js") != std::string::npos) sendFile(filename, "yexy/javascript", start);
    else if (uri.find(".css") != std::string::npos) sendFile(filename, "text/css", start);
    else if (uri.find(".mp3") != std::string::npos) sendFile(filename, "audio/mp3", start);
    else if (uri.find(".mp4") != std::string::npos) sendFile(filename, "audio/mp4", start);
    else sendFile(filename, "text/plain", start);
}

void Tasks::parsePOST(const std::string & uri, char *buf)
{
    std::string filename = uri.substr(1);
    if (uri.find("adder") != std::string::npos)
    {
        int content_len = 0, a, b;
        char * tmp = buf;
        char *request = std::strstr(tmp, "Content-Length:");

        sscanf(request, "Content-Length: %d", &content_len);
        content_len = strlen(tmp) - content_len;

        tmp += content_len;
        sscanf(tmp, "a=%d&b=%d", &a, &b);
        sprintf(tmp, "%d+%d,%d", a, b, _connect_fd);

        if (fork() == 0) execl(filename.c_str(), tmp, NULL);
        wait(NULL);
    }
    else sendFile("html/404.html", "text/html", 0, 404, "Not Found");
}

int Tasks::sendFile(const std::string& filename, const char * type, int start,
                   const int statusCode, const char* statusDesc) {
    struct stat file_status;
    int ret = stat(filename.c_str(), &file_status);
    if (ret < 0 || !S_ISREG(file_status.st_mode))
    {
        std::cout << "file not found: " << filename << std::endl;
        sendFile("html/404.html", "text/html", 0, 404, "Not Found");
        return -1;
    }

    char response_header[256];
    sprintf(response_header, "HTTP1.1 %d %s\r\nServer: StevenHD\r\nContent-Length: %d\r\nContent-Type: %s;charset:utf-8\r\n\r\n",
            statusCode, statusDesc, int(file_status.st_size - start), type);

    send(_connect_fd, response_header, strlen(response_header), 0);
    int fd = open(filename.c_str(), O_RDONLY);
    int sum = start;

    while (sum < file_status.st_size)
    {
        off_t t = sum;
        int recv_len = sendfile(_connect_fd, fd, &t, file_status.st_size);
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
                close(fd);
                break;
            }
        }
        else sum += recv_len;
    }

    close(fd);
    return 0;
}

int Tasks::get_Size(const std::string &filename)
{
    struct stat file_status;
    int ret = stat(filename.c_str(), &file_status);
    if (ret < 0)
    {
        std::cout << "file not found: " << filename << std::endl;
        return 0;
    }

    return file_status.st_size;
}