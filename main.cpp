#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <unistd.h>

int createListenFD(int &port)
{
    int listen_socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    // set socket options for "reuse address"
    int on = 1;
    setsockopt(listen_socket_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    struct sockaddr_in server_addr;
    socklen_t server_len = sizeof(server_addr);
    bzero(&server_addr, sizeof(server_addr));
    /* fill a addr_struct */
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);   // little-endian to ig-endian
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // bind IP and port
    bind(listen_socket_fd, (struct sockaddr*)&server_addr, server_len);

    /* start listen.. */
    /* The role of "listen_socket_fd" is letting "client" could connect to "server" */
    /* and commit request for "server" */
    int backlog = 128;
    int res = listen(listen_socket_fd, backlog);
    if (res == -1)
    {
        perror("listen error");
        exit(-1);
    }
    printf("WebServer start accept connection..\n");    // wait for connecting

    return listen_socket_fd;
}

int main(int argc, char * argv[])
{
    if (argc < 2)
    {
        printf("eg: WebServer port\n");
        exit(-1);
    }
    int port = atoi(argv[1]);

    int listen_socket_fd = createListenFD(port);

    /* Init client address */
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    /* obtain "client's FD" */
    int connection_fd = accept(listen_socket_fd, (struct sockaddr*)&client_addr, &client_len);
    if (connection_fd == -1)
    {
        perror("accept error");
        exit(-1);
    }

    /* Init buffer to recv data from client */
    char read_buf[1024] = {0};
    int recv_len = recv(connection_fd, read_buf, sizeof(read_buf), 0);
    if (recv_len == -1)
    {
        perror("recv error");
        exit(-1);
    }
    else if (recv_len == 0)
    {
        printf("client disconnected..");
        close(connection_fd);
    }
    else
    {
        printf("The client's request is: %s\n", read_buf);
    }

    close(listen_socket_fd);
    return 0;
}
