/*
 * @author: hlhd
 */

#include "WebServer.h"
#include "task.h"

int main(int argc, char * argv[])
{
    if (argc <= 2)
    {
        printf("eg: WebServer port\n");
        exit(-1);
    }

    const char* IP = argv[1];
    int port = atoi(argv[2]);   // string to int

    WebServer _WebServer(port);
    _WebServer.go();

    return 0;
}
