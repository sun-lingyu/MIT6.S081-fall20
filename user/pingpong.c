#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    int pping[2];
    int ppong[2];
    char message[1];


    pipe(pping);
    pipe(ppong);

    if(fork() == 0) {//child
        close(pping[1]);
        read(pping[0],message,1);
        fprintf(2,"%d: received ping\n",getpid());
        close(pping[0]);

        close(ppong[0]);
        write(ppong[1], "a", 1);
        close(ppong[1]);
    } else {//parent
        close(pping[0]);
        write(pping[1], "a", 1);
        close(pping[1]);

        close(ppong[1]);
        read(ppong[0],message,1);
        fprintf(2,"%d: received pong\n",getpid());
        close(ppong[0]);
    }
    exit(0);
}