/*
 * @author: hlhd
 */

#include "WebServer.h"

WebServer::WebServer(int port) : _port(port)
{
    socklen_t server_len = sizeof(server_addr);
}

WebServer::~WebServer()
{
    close(_socketFD);
}

int WebServer::createListenFD(int &port)
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
    //inet_pton(AF_INET, IP, &server_addr.sin_addr);
    server_addr.sin_port = htons(port);   // 小端转大端, 设置端口
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 监听本机所有的IP

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

void WebServer::set_NonBlocking(int &fd)
{
    int old_option = fcntl( fd, F_GETFL );
    int new_option = old_option | O_NONBLOCK;
    fcntl( fd, F_SETFL, new_option );
}

void WebServer::add_FD(int &epoll_fd, int &fd, bool oneshot)
{
    /* 设置文件描述符connection_fd为非阻塞模式 */
    if (oneshot)
    {
        set_NonBlocking( fd );
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

void WebServer::igno_SIG(int sig, void( handler )(int), bool restart)
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

int WebServer::go()
{
    /* 忽略SIGPIPE信号 */
    igno_SIG( SIGPIPE, SIG_IGN );

    /* 填充地址结构体 */
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;               // 地址族
    //inet_pton(AF_INET, IP, &server_addr.sin_addr);
    server_addr.sin_port = htons(_port);   // 小端转大端, 设置端口
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 监听本机所有的IP

    /* 创建监听套接字，返回一个套接字的文件描述符 */
    int listen_socket_fd = createListenFD(_port);

    /* 创建线程池 */
    ThreadPool threadPool(64);

    /* 创建epoll树的根节点 */
    _epollFD = epoll_create(2048);
    if (_epollFD < 0)
    {
        std::cout << "epoll_create error" << std::endl;
        exit(-1);
    }

    /* epoll树是用来检测节点对应的文件描述符有没有发生变化 */
    /* 初始化epoll树 */
    struct epoll_event ev; // 往树上挂节点本质上是挂一个结构体, 所以初始化一个结构体; ev对应的是listen_socket_fd中的信息
    add_FD(_epollFD, listen_socket_fd, false);

    /* epoll_wait()中的第2个参数，用来通知是哪些事件发生了变化 */
    struct epoll_event events[2048];

    while (true)
    {
        /* 使用epoll通知内核进行文件流的检测 */
        int ret = epoll_wait(_epollFD, events, sizeof(events) / sizeof(events[0]), -1);
        printf("=====================================epoll_wait=====================================\n");

        /* 通过epoll_wait()可以知道这棵树上有ret个节点发生了变化 */
        /* 遍历events数组中的前ret个元素 */
        for (int i = 0; i < ret; i ++ )
        {
            int cur_fd = events[i].data.fd;

            /* 判断是否有新连接 */
            if (cur_fd == listen_socket_fd)
            {
                /* 初始化客户端的地址 */
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);

                /* 接收客户端的连接请求 */
                int connection_fd = accept(listen_socket_fd, (struct sockaddr*)&client_addr, &client_len); // 这里会阻塞，有结果才会往下走
                if (connection_fd == -1)
                {
                    perror("accept error");
                    exit(-1);
                }
                printf("新的客户端连接\n");

                add_FD(_epollFD, connection_fd, true);

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

                /* 当有新数据写入的时候，新建任务 */
                Tasks * task = new Tasks(cur_fd, _epollFD);
                /* 添加任务 */
                threadPool.append_Task(task);
            }
        }
    }

    return 0;
}