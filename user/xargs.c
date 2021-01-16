#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

int
main(int argc, char *argv[])
{
    while(1)
    {
        char buf[512];
        int offset=0;
        //read a line
        if(read(0, buf+offset, 1)==0)//no more lines!
            break;
        while(buf[offset]!='\n')
        {
            offset++;
            read(0, buf+offset, 1);
        }
        buf[offset]='\0';
        
        if(fork()==0)//child
        {
            char* arg[MAXARG];
            int curr_arg_pos=0;
            for(int i=1;i<argc;i++)
                arg[curr_arg_pos++]=argv[i];
            arg[curr_arg_pos++]=buf;
            int i=0;
            
            while(buf[i]!='\0')
            {
                if(buf[i]==' ')
                    buf[i]='\0';
                if(i!=0 && buf[i-1]=='\0' && buf[i]!=' ')
                    arg[curr_arg_pos++]=buf+i;
                i++;       
            }
            
            exec(argv[1],arg);
        }
        wait(0);
    }
    wait(0);
    exit(0);
}