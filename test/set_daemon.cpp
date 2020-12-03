#include <stdio.h>
#include <unistd.h>
#include <cstdlib>
#include <time.h>
#include <sys/stat.h>
#include <sys/param.h>

void init_daemon()
{
    pid_t pid = fork();
    if (pid == 0)
    {
        setsid();
        chdir("/");
        umask(0000);
        int i = 0;
        for (; i < NOFILE; i ++ )
            close(i);
    }
    if (pid > 0)
    {
        exit(0);
    }
    else if (pid == -1)
    {
        perror("fork error");
    }
}

int main(int argc, char** argv) {
    FILE *fp;
    time_t now_time;
    init_daemon();

    while (1) {
        fp = fopen("/log", "a+");
        if (fp == nullptr) {
            perror("fopen");
            exit(-1);
        }

        now_time = time(NULL);
        fputs(ctime(&now_time), fp);
        fclose(fp);
        sleep(1);
    }

    return 0;
}