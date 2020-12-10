#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <string.h>
#include <strings.h>
#include <signal.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <ctype.h>
#include <unistd.h>
#include <poll.h>
#include "ListNode.h"

const int SERV_PORT = 8888;
const int MAXBYTES = 4096;

const char CMD_BEGIN = 0x02;
const char CMD_END = 0x03;
const char CMD_LOGIN = 0x11;
const char CMD_LIST = 0x12;
const char CMD_CHAT = 0x13;

const int TIMEOUT = 10 * 1000;

static int listenfd;
static Node* head;

void sig_handler(int sig)
{
    printf("pthread ID %ld\n", (unsigned long) pthread_self());
}

ssize_t readT(int fd, char *buf, size_t size)
{
    int ret;
    struct pollfd pf;
    pf.fd = fd;
    pf.events = POLLIN;

    ret = poll(&pf, 1, TIMEOUT);
    if (ret == 1)
    {
        ret = read(fd, buf, size);
    }
    else
    {
        ret = 0;
    }

    return ret;
}
/* 读取一个完整的0203数据包 */
ssize_t readPack(int fd, char* buf, size_t size)
{
    int ret, index = 0;
    char c;
    do
    {
        ret = readT(fd, &c, 1);
        if (ret <= 0) return ret;
    }
    while (c != CMD_BEGIN);

    buf[index ++] = c;

    for (; index < size; index ++)
    {
        ret = readT(fd, &c, 1);
        if (ret <= 0) return ret;
        else
        {
            buf[index] = c;
            if (c == CMD_END)
            {
                index++;
                break;
            }
        }
    }

    return index;
}

const int IDLEN = 4;
/* 解析数据包，组合应答包, 返回值表示send_buf写入的字节数 */
int analyzeData(int clientfd, char* recv_buf, size_t recv_len,
                char* send_buf, size_t send_len,
                int* destFD)
{
    int index = 0;
    char tmp[8];

    if (recv_buf[1] == CMD_LOGIN)
    {
        puts("client login success");

        send_buf[index ++] = CMD_BEGIN;
        send_buf[index ++] = CMD_LOGIN;
        sprintf(send_buf + index, "%04d", clientfd);
        index += IDLEN;
        send_buf[index ++] = CMD_END;

        insertlist(head, clientfd);
        *destFD = clientfd;
    }
    else if (recv_buf[1] == CMD_LIST)
    {
        send_buf[index ++ ] = CMD_BEGIN;
        send_buf[index ++ ] = CMD_LIST;
        index += getlist(head, send_buf + index, send_len - index);
        printf("index %d\n", index);

        send_buf[index ++] = CMD_END;
        *destFD = clientfd;
    }
    else if (recv_buf[1] == CMD_CHAT)
    {
        /* 获取目的ID */
        memcpy(tmp, recv_buf + 2, IDLEN);
        tmp[IDLEN] = '\0';
        *destFD = atoi(tmp);
        memcpy(send_buf, recv_buf, recv_len);
        index = recv_len;
    }

    return index;
}

void *do_work(void *arg)
{
    char recv_buf[MAXBYTES];
    char send_buf[MAXBYTES];
    int ret, i;
    int clntfd, destFD;

    struct sockaddr_in clntaddr;
    socklen_t clntlen;
    clntlen = sizeof(clntaddr);
    bzero(&clntaddr, clntlen);

    printf("do work pthread ID %ld\n", (unsigned long)pthread_self());
    pthread_detach(pthread_self());

    while (1) {
        clntfd = accept(listenfd, (struct sockaddr *) &clntaddr, &clntlen);
        if (clntfd == -1) continue;

        while (1)
        {
            /* 接收客户端的请求 */
            ret = readPack(clntfd, recv_buf, sizeof(recv_buf));
            if (ret == 0) {
                puts("client exit");
                break;
            } else if (ret == -1) {
                puts("error occur");
                break;
            }
            printf("read bytes %d\n", ret);

            /* 解析数据包 */
            ret = analyzeData(clntfd, recv_buf, ret,
                              send_buf, sizeof(send_buf), &destFD);

            /* 发送应答包 */
            if (ret > 0) write(destFD, send_buf, ret);
        }

        close(clntfd);
        deletelist(head, clntfd);
    }

    return (void*) 0;
}

int main(int argc, char** argv)
{
    if (argc <= 2)
    {
        printf("eg: WebServer port\n");
        exit(-1);
    }

    const char* IP = argv[1];
    int port = atoi(argv[2]);   // string to int

    char buf[MAXBYTES];

    /* 注册信号处理函数 */
    signal(SIGPIPE, sig_handler);

    /* 初始化地址结构体 */
    struct sockaddr_in servaddr, cliaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    inet_pton(AF_INET, IP, &servaddr.sin_addr);
    servaddr.sin_port = htons(port);   // 小端转大端, 设置端口

    /* 定义一个TCP协议的套接字 */
    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    /* 把套接字与IP和端口绑定在一起 */
    bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    listen(listenfd, 128);

    printf("Accepting connections ...\n");

    head = createlist_head();
    int ret, i;
    pthread_t th;
    for (int i = 0; i < 100; i ++ )
    {
        ret = pthread_create(&th, NULL, do_work, NULL);
        if (ret != 0)
        {
            printf("pthread_create %s\n", strerror(ret));
            break;
        }
    }

    while (1)
    {
        sleep(10);
    }

    close(listenfd);
    return 0;
}

