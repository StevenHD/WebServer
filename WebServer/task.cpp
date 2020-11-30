/*
 * @author: hlhd
 */

#include "task.h"

void remove_FD(int epoll_fd, int fd)
{
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

void reset_ONESHOT(int epoll_fd, int fd)
{
    epoll_event ev;
    ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
    ev.data.fd = fd;
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

void Task::execute_Task()
{
    /* 读数据 */
    /* 初始化缓冲区，用来接收客户端发送的数据 */
    /* 处理客户端发来的请求 */
    char in_buf[1024] = {0};
    int readData_len;

    /* 循环读数据 */
    while ((readData_len = recv(_connect_fd, in_buf, sizeof(in_buf), 0)) > 0)
    {
        /* 将数据打印到终端 */
        printf("The client's request is: %s\n", in_buf);

        /* 发送给客户端 */
        int start_id = 0;
        char method[8], uri[1024], version[16];
        http_parser(_connect_fd, in_buf);
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
            reset_ONESHOT( _epoll_fd, _connect_fd );
            printf("缓冲区数据已经读完了\n");
        }
        else
        {
            printf("recv error\n");
            exit(-1);
        }
    }
}


void Task::parseGET(const std::string &uri, int start)
{
    std::string filename = uri.substr(1);

    if (uri == "/" || uri == "/index.html") send_File("./index.html", "text/html", start);
    else if (uri.find(".jpg") != std::string::npos || uri.find(".png") != std::string::npos)
        send_File(filename, "image/jpg", start);
    else if (uri.find(".html") != std::string::npos) send_File(filename, "text/html", start);
    else if (uri.find(".js") != std::string::npos) send_File(filename, "yexy/javascript", start);
    else if (uri.find(".css") != std::string::npos) send_File(filename, "text/css", start);
    else if (uri.find(".mp3") != std::string::npos) send_File(filename, "audio/mp3", start);
    else if (uri.find(".mp4") != std::string::npos) send_File(filename, "audio/mp4", start);
    else send_File(filename, "text/plain", start);
}

//void Task::parsePOST(const std::string & uri, char *buf)
//{
//    std::string filename = uri.substr(1);
//    if (uri.find("adder") != std::string::npos)
//    {
//        int content_len = 0, a, b;
//        char * tmp = buf;
//        char *request = std::strstr(tmp, "Content-Length:");
//
//        sscanf(request, "Content-Length: %d", &content_len);
//        content_len = strlen(tmp) - content_len;
//
//        tmp += content_len;
//        sscanf(tmp, "a=%d&b=%d", &a, &b);
//        sprintf(tmp, "%d+%d,%d", a, b, _connect_fd);
//
//        if (fork() == 0) execl(filename.c_str(), tmp, NULL);
//        wait(NULL);
//    }
//    else send_File("html/404.html", "text/html", 0, 404, "Not Found");
//}

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

    send(_connect_fd, response_header, strlen(response_header), 0);
    int send_fd = open(filename.c_str(), O_RDONLY);
    int sum = start;

    while (sum < file_status.st_size)
    {
        off_t t = sum;
        int recv_len = sendfile(_connect_fd, send_fd, &t, file_status.st_size);
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

//int Tasks::get_Size(const std::string &filename)
//{
//    struct stat file_status;
//    int ret = stat(filename.c_str(), &file_status);
//    if (ret < 0)
//    {
//        std::cout << "file not found: " << filename << std::endl;
//        return 0;
//    }
//
//    return file_status.st_size;
//}