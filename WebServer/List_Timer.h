#ifndef WEBSERVER_LIST_TIMER_H
#define WEBSERVER_LIST_TIMER_H

#include <time.h>
#include <netinet/in.h>
#include <cstdio>

const int BUFFER_SIZE = 64;

class List_Timer;

struct Client_Data
{
    sockaddr_in addr;
    int socketfd;
    List_Timer * timer;
};

/* 定时器类 */
class List_Timer
{
public:
    List_Timer();

public:

    /* 任务的超时时间 */
    time_t _expireTime{};

    /* 任务的回调函数 */
    void (*cb_func)(Client_Data*){};

    /* 回调函数处理的客户数据 */
    Client_Data* _usrData{};

    /* 指向前一个定时器 */
    List_Timer* _prev;

    /* 指向后一个定时器 */
    List_Timer* _next;
};

/* 定时器容器，一个升序双向链表 */
class Sort_Timer_List
{
public:
    Sort_Timer_List();

    ~Sort_Timer_List();
    void add_timer(List_Timer * timer);
    void adjust_timer(List_Timer * timer);
    void del_timer(List_Timer * timer);
    void tick();

private:
    void add_timer(List_Timer* timer, List_Timer* List_head);
    List_Timer* _head;
    List_Timer* _tail;
};

class Utils
{
public:
    Utils() = default;
    ~Utils() = default;

    /* 初始化 */
    void init(int timeslot);

    /* 将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT */
    void add_FD(int epoll_fd, int fd, bool oneshot);

    /* 设置信号函数 */
    void add_Sig(int sig, void(handler)(int), bool restart);

    /* 对文件描述符设置非阻塞 */
    int set_NonBlocking(int fd);

    /* 信号处理函数 */
    static void sig_handler(int sig);

    /*
     * 定时处理任务
     * 重新定时以不断触发SIGALRM信号
     */
    void timer_handler();

    void show_error(int connectfd, const char *info);

public:
    static int *_pipefd;
    static int _epollfd;
    Sort_Timer_List _timer_lst;
    int _TIMESLOT;
};

/* 定时器回调函数，用来删除非活动连接socket上的注册事件，并关闭 */
void cb_func(Client_Data* user_data);

#endif //WEBSERVER_LIST_TIMER_H
