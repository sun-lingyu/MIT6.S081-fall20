#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

//based on ls

char*
fmtname(char *path)
{
  static char buf[DIRSIZ+1];
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  // Return name.
  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  buf[strlen(p)]=0;

  return buf;
}

void find(char path[],char name[])
{
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    if((fd = open(path, 0)) < 0){
    fprintf(2, "find: cannot open %s\n", path);
    return;
    }

    if(fstat(fd, &st) < 0){
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return;
    }

    switch(st.type){
    case T_FILE:

    if(strcmp(fmtname(path),name)==0)
        printf("%s\n",path);
    break;

    case T_DIR:
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
        printf("find: path too long\n");
        break;
    }
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
        if(de.inum == 0)
            continue;
        memmove(p, de.name, DIRSIZ);

        if(stat(buf, &st) < 0){
            printf("find: cannot stat %s\n", buf);
            continue;
        }

        if(strcmp(fmtname(buf),"..")!=0 && strcmp(fmtname(buf),".")!=0)
            find(buf,name);
    }
    break;
    }
    close(fd);
}


int
main(int argc, char *argv[])
{

    if(argc < 3){
        fprintf(2,"number of arguments not right!");
        exit(0);
    }
    find(argv[1],argv[2]);

    exit(0);
}
