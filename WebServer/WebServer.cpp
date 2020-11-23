/*
 * @author: hlhd
 */

#include "WebServer.h"

int set_NonBlocking(int fd)
{
    int old_option = fcntl( fd, F_GETFL );
    int new_option = old_option | O_NONBLOCK;
    fcntl( fd, F_SETFL, new_option );
    return old_option;
}

void add_FD(int epoll_fd, int fd, bool oneshot)
{
    epoll_event ev;
    /* 设置文件描述符connection_fd为非阻塞模式 */

    /* 将新得到的connection_fd挂到epoll树上 */
    /* 设置边沿触发 */
    ev.events = EPOLLIN | EPOLLET;
    if (oneshot) ev.events |= EPOLLONESHOT;
    ev.data.fd = fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev); // 将connection_fd的信息存入到tmp这个结构体变量中

    set_NonBlocking( fd );
}

void WebServer::createListenFD()
{
    /* 创建套接字 */
    _socketFD = socket(AF_INET, SOCK_STREAM, 0);

    // set socket options for "reuse address"
    int on = 1, res = -1;
    res = setsockopt(_socketFD, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    if (res == -1) std::cout << "Failed to set socket options!" << std::endl;

    /* 初始化服务器 */
    socklen_t server_len = sizeof(server_addr);

    // bind IP and port
    res = bind(_socketFD, (struct sockaddr*)&server_addr, server_len);
    if (res == -1) perror("bind");

    /* start listen.. */
    /* The role of "listen_socket_fd" is letting "client" could connect to "server" */
    /* and commit request for "server" */
    int backlog = 128;
    res = listen(_socketFD, backlog);
    if (res == -1)
    {
        perror("listen error");
        exit(-1);
    }
    printf("WebServer start accept connection..\n");    // wait for connecting

    //return _socketFD;
}

int WebServer::go()
{
    /* 忽略SIGPIPE信号 */
    //igno_SIG( SIGPIPE, SIG_IGN );
    signal( SIGPIPE, SIG_IGN );

    /* 填充地址结构体 */
    server_addr.sin_family = AF_INET;               // 地址族
    //inet_pton(AF_INET, IP, &server_addr.sin_addr);
    server_addr.sin_port = htons(_port);   // 小端转大端, 设置端口
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 监听本机所有的IP

    /* 创建监听套接字，返回一个套接字的文件描述符 */
    createListenFD();

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
    epoll_event ev; // 往树上挂节点本质上是挂一个结构体, 所以初始化一个结构体; ev对应的是listen_socket_fd中的信息
    //add_FD(_epollFD, _socketFD, false);
    ev.data.fd = _socketFD;
    ev.events = EPOLLIN;
    epoll_ctl(_epollFD, EPOLL_CTL_ADD, _socketFD, &ev);

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
            if (cur_fd == _socketFD)
            {
                /* 初始化客户端的地址 */
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);

                /* 接收客户端的连接请求 */
                int connection_fd = accept(cur_fd, (struct sockaddr*)&client_addr, &client_len); // 这里会阻塞，有结果才会往下走
                if (connection_fd == -1)
                {
                    perror("accept error");
                    exit(-1);
                }
                printf("新的客户端连接\n");

                add_FD(_epollFD, connection_fd, true);
            }
            else
            {
                /* 处理已经连接的客户端发送过来的数据 */
                /* 这里只处理"读操作" */
                if ((events[i].events & EPOLLIN) == 0) continue;

                /* 当有新数据写入的时候，新建任务 */
                Task * task = new Task(cur_fd, _epollFD);
                /* 添加任务 */
                threadPool.append_Task(task);
            }
        }
    }

    return 0;
}