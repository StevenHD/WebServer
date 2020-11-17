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

int createListenFD(int &port)
{
    int listen_socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    // set socket options for "reuse address"
    int on = 1;
    setsockopt(listen_socket_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));

    /* 填充地址结构体 */
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);   // 小端转大端
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    socklen_t server_len = sizeof(server_addr);

    // bind IP and port
    int res = bind(listen_socket_fd, (struct sockaddr*)&server_addr, server_len);
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

int main(int argc, char * argv[])
{
    if (argc < 2)
    {
        printf("eg: WebServer port\n");
        exit(-1);
    }
    int port = atoi(argv[1]);

    /* 创建监听套接字，返回一个套接字的文件描述符 */
    int listen_socket_fd = createListenFD(port);

    /* 初始化客户端的地址 */
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    while (1) {

        /* 接收客户端的连接 */
        int connection_fd = accept(listen_socket_fd, (struct sockaddr*)&client_addr, &client_len); // 这里会阻塞，有结果才会往下走
        if (connection_fd == -1)
        {
            perror("accept error");
            exit(-1);
        }
        printf("有客户端连接\n");

        /* 初始化缓冲区，用来接收客户端发送的数据 */
        /* 处理客户端发来的请求 */
        char read_buf[1024] = {0};
        int recv_len = recv(connection_fd, read_buf, sizeof(read_buf), 0);
        if (recv_len == -1) {
            perror("recv error");
            exit(-1);
        } else if (recv_len == 0) {
            printf("client disconnected..");
            close(connection_fd);
        } else {
            printf("The client's request is: %s\n", read_buf);
            http_parser(connection_fd, read_buf);
            close(connection_fd);
        }
    }
    close(listen_socket_fd);
    return 0;
}
