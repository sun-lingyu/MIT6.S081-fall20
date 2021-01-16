#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#define MAX 35

void fork_r(int read_pipe[])
{
    close(read_pipe[1]);//Very important!!!
    /*A pipe is owned by two processes after fork. Whenever there remains a single process with write end open, the read system call will never return 0!*/ 

    int p[2];
    int num;
    int flag=0;//whether child has been created
    int base;//first number read, must be a prime
    read(read_pipe[0],&num,sizeof(num));
    base=num;
    fprintf(2,"prime %d\n",base);    

    while(read(read_pipe[0],&num,sizeof(num))!=0)
    {
        if(num%base!=0)
        {
            if(flag==0)//child has not been created
            {
                flag=1;
                pipe(p);
                if(fork()==0)//child
                {
                    fork_r(p);
                    exit(0);
                }
            }
            write(p[1],&num,sizeof(num));
        }
    }   
    close(read_pipe[0]);
    if(flag==1)
        close(p[1]);
    
    wait(0);
}

int
main(int argc, char *argv[])
{
    int p[2];
    pipe(p);

    if(fork()==0)//child
    {
        fork_r(p);
        exit(0);
    }
    else
    {
        close(p[0]);
        for(int i=2;i<=35;i++)
        {
            write(p[1],&i,sizeof(i));
        }
        close(p[1]);
        wait(0);
    }    
    exit(0);
}