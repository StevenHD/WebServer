/*
 * @author: hlhd
 */

#include "WebServer.h"

int main(int argc, char * argv[])
{
    if (argc != 2)
    {
        printf("eg: WebServer port\n");
        exit(-1);
    }

    int port = atoi(argv[1]);   // string to int

    WebServer _webserver(port);
    _webserver.go();

    return 0;
}
