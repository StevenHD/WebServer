/*
 * @author hlhd
 * */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <string>
#include <sstream>
#include <unistd.h>
#include <fcntl.h>
#include <vector>
#include <string>
#include <istream>
#include <sstream>
#include <fstream>
#include <iostream>
#include <streambuf>
#include <vector>
#include <iterator>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <errno.h>
#include <signal.h>
#include <cassert>

int createListenFD(const char* IP, int &port)
{
    /* 创建套接字 */
    int listen_socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    // set socket options for "reuse address"
    int on = 1, res = -1;
    res = setsockopt(listen_socket_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    if (res == -1) std::cout << "Failed to set socket options!" << std::endl;

    /* 初始化服务器 */
    struct sockaddr_in server_addr;
    socklen_t server_len = sizeof(server_addr);

    /* 填充地址结构体 */
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;               // 地址族
    inet_pton(AF_INET, IP, &server_addr.sin_addr);
    server_addr.sin_port = htons(port);   // 小端转大端, 设置端口
    //server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 监听本机所有的IP


    // bind IP and port
    res = bind(listen_socket_fd, (struct sockaddr*)&server_addr, server_len);
    if (res == -1) perror("bind");

    /* start listen.. */
    /* The role of "listen_socket_fd" is letting "client" could connect to "server" */
    /* and commit request for "server" */
    int backlog = 128;
    res = listen(listen_socket_fd, backlog);
    if (res == -1)
    {
        perror("listen error");
        exit(-1);
    }
    printf("WebServer start accept connection..\n");    // wait for connecting

    return listen_socket_fd;
}

/* 处理客户端的请求 */
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

/* 忽略SIGPIPE信号 */
void addsig( int sig, void( handler )(int), bool restart = true )
{
    struct sigaction sa;
    memset( &sa, '\0', sizeof( sa ) );
    sa.sa_handler = handler;
    if( restart )
    {
        sa.sa_flags |= SA_RESTART;
    }
    sigfillset( &sa.sa_mask );
    assert( sigaction( sig, &sa, NULL ) != -1 );
}

void reset_oneshot( int& epollfd, int& fd )
{
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
    epoll_ctl( epollfd, EPOLL_CTL_MOD, fd, &event );
}

/* 设置为非阻塞 */
int setnonblocking( int& fd )
{
    int old_option = fcntl( fd, F_GETFL );
    int new_option = old_option | O_NONBLOCK;
    fcntl( fd, F_SETFL, new_option );
    return old_option;
}

void add_fd( int& epoll_fd, int& fd, bool oneshot )
{
    /* 设置文件描述符connection_fd为非阻塞模式 */
    if (oneshot)
    {
        setnonblocking( fd );
        /* 将新得到的connection_fd挂到epoll树上 */
        struct epoll_event tmp;
        /* 设置边沿触发 */
        tmp.events = EPOLLIN | EPOLLET;
        if (oneshot) tmp.events |= EPOLLONESHOT;
        tmp.data.fd = fd;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &tmp); // 将connection_fd的信息存入到tmp这个结构体变量中
    }
    else {
        struct epoll_event ev;
        ev.data.fd = fd;
        ev.events = EPOLLIN | EPOLLET;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev);
    }
}

int main(int argc, char * argv[])
{
    if (argc <= 2)
    {
        printf("eg: WebServer port\n");
        exit(-1);
    }

    const char* IP = argv[1];
    int port = atoi(argv[2]);   // string to int

    /* 忽略SIGPIPE信号 */
    addsig( SIGPIPE, SIG_IGN );

    /* 创建监听套接字，返回一个套接字的文件描述符 */
    int listen_socket_fd = createListenFD(IP, port);

    /* 初始化客户端的地址 */
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    /* 创建epoll树的根节点 */
    int epoll_fd = epoll_create(2000);

    /* epoll树是用来检测节点对应的文件描述符有没有发生变化 */
    /* 初始化epoll树 */
    struct epoll_event ev; // 往树上挂节点本质上是挂一个结构体, 所以初始化一个结构体; ev对应的是listen_socket_fd中的信息
    add_fd(epoll_fd, listen_socket_fd, false);

    /* epoll_wait()中的第2个参数，用来通知是哪些事件发生了变化 */
    struct epoll_event events[2000];

    while (1)
    {
        /* 使用epoll通知内核进行文件流的检测 */
        int ret = epoll_wait(epoll_fd, events, sizeof(events) / sizeof(events[0]), -1);
        printf("=====================================epoll_wait=====================================\n");

        /* 通过epoll_wait()可以知道这棵树上有ret个节点发生了变化 */
        /* 遍历events数组中的前ret个元素 */
        for (int i = 0; i < ret; i ++ )
        {
            int cur_fd = events[i].data.fd;

            /* 判断是否有新连接 */
            if (cur_fd == listen_socket_fd)
            {
                /* 接收客户端的连接请求 */
                int connection_fd = accept(listen_socket_fd, (struct sockaddr*)&client_addr, &client_len); // 这里会阻塞，有结果才会往下走
                if (connection_fd == -1)
                {
                    perror("accept error");
                    exit(-1);
                }
                printf("有客户端连接\n");

                add_fd(epoll_fd, connection_fd, true);

                /* 打印客户端信息 */
                char client_ip[64] = {0};
                printf("New Client IP: %s, Port: %d\n",
                       inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, client_ip, sizeof(client_ip)),
                       ntohs(client_addr.sin_port));
            }
            else
            {
                /* 处理已经连接的客户端发送过来的数据 */
                /* 这里只处理"读操作" */
                if ((events[i].events & EPOLLIN) == 0) continue;

                /* 读数据 */
                /* 初始化缓冲区，用来接收客户端发送的数据 */
                /* 处理客户端发来的请求 */
                char read_buf[1024] = {0};
                int readData_len;

                /* 循环读数据 */
                while ((readData_len = recv(cur_fd, read_buf, sizeof(read_buf), 0)) > 0)
                {
                    /* 将数据打印到终端 */
                    printf("The client's request is: %s\n", read_buf);
                    //write(STDOUT_FILENO, read_buf, readData_len);

                    /* 发送给客户端 */
                    http_parser(cur_fd, read_buf);
                }
                //close(cur_fd);

                if (readData_len == 0)
                {
                    printf("客户端断开了连接..");

                    /* 将fd从epoll树上删掉 */
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, cur_fd, NULL);
                    close(cur_fd);
                }
                else if (readData_len == -1)
                {
                    if (errno == EAGAIN)
                    {
                        reset_oneshot( epoll_fd, cur_fd );
                        printf("缓冲区数据已经读完了\n");
                    }
                    else
                    {
                        printf("recv error\n");
                        exit(-1);
                    }
                }
            }
        }
    }
    close(listen_socket_fd);
    return 0;
}
