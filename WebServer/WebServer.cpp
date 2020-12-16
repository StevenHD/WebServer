/*
 * @author: hlhd
 */

#include "WebServer.h"

WebServer::WebServer(int port): _s_port(port)
{
    // Task类对象
    _users = new Task[MAX_FD];

    // root文件夹路径
    char server_path[256];
    getcwd(server_path, 256);
    char root[6] = "/root";
    _s_root = (char*)malloc(strlen(server_path) + strlen(root) + 1);
    strcpy(_s_root, server_path);
    strcat(_s_root, root);

   // 定时器
    _usrs_timer = new Client_Data[MAX_FD];
}

WebServer::~WebServer()
{
    close(_s_epollfd);
    close(_s_listenfd);
    close(_s_pipefd[0]);
    close(_s_pipefd[1]);

    delete[] _users;
    delete[] _usrs_timer;
    delete[] _pool;
}

void WebServer::init(int port, int thread_num)
{
    _s_port = port;
    _s_thread_num = thread_num;
}

void WebServer::thread_pool()
{
    _pool = new ThreadPool(_s_thread_num);
}

void WebServer::event_listen()
{
    /* 创建套接字 */
    _s_listenfd = socket(AF_INET, SOCK_STREAM, 0);

    /* 初始化服务器 */
    int ret = 0;
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    socklen_t server_len = sizeof(server_addr);

    /* 填充地址结构体 */
    server_addr.sin_family = AF_INET;               // 地址族
    server_addr.sin_port = htons(_s_port);   // 小端转大端, 设置端口
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 监听本机所有的IP

    // 端口复用
    int on = 1, res = -1;
    res = setsockopt(_s_listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    if (res == -1) std::cout << "Failed to set socket options!" << std::endl;

    // bind IP and port
    res = bind(_s_listenfd, (struct sockaddr*)&server_addr, server_len);
    if (res == -1) perror("bind");

    int backlog = 128;
    res = listen(_s_listenfd, backlog);
    if (res == -1)
    {
        perror("listen error");
        exit(-1);
    }
    printf("WebServer start accept connection..\n");    // wait for connecting

    _utils.init(TIMESLOT);

    // epoll创建内核事件表
    epoll_event evnts[MAX_EVENT_NUMBER];

    // 创建epoll树的根节点
    _s_epollfd = epoll_create(1024);
    if (_s_epollfd < 0)
    {
        std::cout << "epoll_create error" << std::endl;
        exit(-1);
    }

    _utils.add_fd(_s_epollfd, _s_listenfd, false);
    Task::_task_epollfd = _s_epollfd;

    // 创建管道，注册pipefd[0]上的可读事件
    ret = socketpair(AF_UNIX, SOCK_STREAM, 0, _s_pipefd);
    if (ret == -1)
    {
        perror("socketpair error");
        exit(-1);
    }
    _utils.set_nonblocking(_s_pipefd[1]);   // 设置管道写端为非阻塞

    _utils.add_fd(_s_epollfd, _s_pipefd[0], false); // 设置管道读端为ET非阻塞，并添加到epoll内核事件表
    _utils.add_sig(SIGPIPE, SIG_IGN, false);
    _utils.add_sig(SIGALRM, _utils.sig_handler, false);
    _utils.add_sig(SIGTERM, _utils.sig_handler, false);

    // 每隔TIMESLOT时间触发SIGALRM信号
    alarm(TIMESLOT);

    // epoll树是用来检测节点对应的文件描述符有没有发生变化
    // 初始化epoll树
    // 往树上挂节点本质上是挂一个结构体
    // 所以初始化一个结构体
    // ev对应的是listen_socket_fd中的信息
    // epoll_wait()中的第2个参数，用来通知是哪些事件发生了变化

    // 工具类,信号和描述符基础操作
    Utils::_u_pipefd = _s_pipefd;
    Utils::_u_epollfd = _s_epollfd;
}

void WebServer::timer(int connfd, struct sockaddr_in client_addr)
{
    _users[connfd].init(connfd, client_addr, _s_root);

    // 初始化clnt data
    _usrs_timer[connfd].addr = client_addr;
    _usrs_timer[connfd].socketfd = connfd;

    // 创建定时器
    List_Timer* timer = new List_Timer;

    // 绑定用户数据
    timer->_usrData = &_usrs_timer[connfd];

    // 设置回调函数和超时时间
    timer->cb_func = cb_func;
    time_t cur_time = time(nullptr);
    timer->_expireTime = cur_time + 3 * TIMESLOT;
    _usrs_timer[connfd].timer = timer;

    // 将定时器添加到链表中
    timer_lst.add_timer(timer);
}

/* 若有数据传输，则将定时器往后延迟3个单位 */
/* 并对新的定时器在链表上的位置进行调整 */
void WebServer::adjust_timer(List_Timer *timer)
{
    time_t cur_time = time(NULL);
    timer->_expireTime = cur_time + TIMESLOT * 3;
    _utils._timer_lst.adjust_timer(timer);
}

void WebServer::dealwith_timer(List_Timer* timer, int socketfd)
{
    timer->cb_func(&_usrs_timer[socketfd]);
    if (timer)
    {
        _utils._timer_lst.del_timer(timer);
    }
}

bool WebServer::dealwith_clntdata()
{
    // 初始化客户端的地址
    struct sockaddr_in clnt_addr;
    socklen_t clnt_len = sizeof(clnt_addr);

    // 接收客户端的连接请求
    int connect_fd = accept(_s_listenfd, (struct sockaddr*)&clnt_addr, &clnt_len); // 这里会阻塞，有结果才会往下走
    if (connect_fd == -1)
    {
        perror("accept error");
        exit(-1);
    }

    if (Task::_usr_cnt >= MAX_FD)
    {
        _utils.show_error(connect_fd, "server busy");
        return false;
    }
    printf("新的客户端连接\n");

    timer(connect_fd, clnt_addr);

    return true;
}

bool WebServer::dealwith_signal(bool &timeout, bool &stop_server)
{
    int res = 0;
    char _signals[1024];

    res = recv(_s_pipefd[0], _signals, sizeof(_signals), 0);
    if (res == -1)
    {
        return false;
    }
    else if (res == 0)
    {
        return false;
    }
    else
    {
        for (int i = 0; i < res; i ++ )
        {
            switch(_signals[i])
            {
                case SIGALRM:
                {
                    // timeout变量标记有定时任务需要处理
                    // 但不立即处理定时任务
                    // 因为定时任务的优先级不是很高
                    // 优先处理其他更重要的任务
                    timeout = true;
                    break;
                }
                case SIGTERM:
                {
                    stop_server = true;
                    break;
                }
            }
        }
    }

    return true;
}


/* 处理"读操作" */
void WebServer::dealwith_read(int sockfd)
{
    List_Timer* timer = _usrs_timer[sockfd].timer;
    if (timer) adjust_timer(timer);

    if (_users[sockfd].read_data())
    {
        // 若监测到读事件，将该事件放入请求队列
        _pool->append_Task(_users + sockfd);
        if (timer) adjust_timer(timer);
        else
        {
            dealwith_timer(timer, sockfd);
        }
    }
}

void WebServer::dealwith_write(int sockfd)
{
    List_Timer* timer = _usrs_timer[sockfd].timer;
    if (_users[sockfd].write_data())
    {
        if (timer) adjust_timer(timer);
        else dealwith_timer(timer, sockfd);
    }
}

void WebServer::event_loop()
{
    bool stop_server = false;
    bool timeout = false;
    while (!stop_server)
    {
        // 使用epoll通知内核进行文件流的检测
        int ret = epoll_wait(_s_epollfd, _s_events, sizeof(_s_events) / sizeof(_s_events[0]), -1);
        printf("=====================================epoll_wait=====================================\n");
        if (ret < 0 && errno != EINTR)
        {
            break;
        }

        // 通过epoll_wait()可以知道这棵树上有ret个节点发生了变化
        // 遍历events数组中的前ret个元素
        for (int i = 0; i < ret; i ++ )
        {
            int cur_fd = _s_events[i].data.fd;

            // 判断是否有新连接
            if (cur_fd == _s_listenfd)
            {
                bool flag = dealwith_clntdata();
                if (!flag) continue;
            }
            // 处理信号
            else if ((cur_fd == _s_pipefd[0]) && (_s_events[i].events & EPOLLIN))
            {
                bool flag = dealwith_signal(timeout, stop_server);
            }
            else if (_s_events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                // 服务器端关闭连接，移除对应的定时器
                List_Timer* timer = _usrs_timer[cur_fd].timer;
                dealwith_timer(timer, cur_fd);
            }
            // 处理客户连接上接收到的数据
            else if (_s_events[i].events & EPOLLIN)
            {
                dealwith_read(cur_fd);
            }
            else if (_s_events[i].events & EPOLLOUT)
            {
                dealwith_write(cur_fd);
            }
        }

        if (timeout)
        {
            _utils.timer_handler();
            timeout = false;
        }
    }
}