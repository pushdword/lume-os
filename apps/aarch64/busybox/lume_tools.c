/* Lume-owned tools; not represented as upstream BusyBox applets. */
#include "libbb.h"
int ls_main(int argc,char **argv)
{
    if(argc>2)return 2;
    int fd=open(argc==2?argv[1]:".",O_RDONLY);if(fd<0)return 1;
    struct lume_dirent e;int64_t n;
    while((n=lume_call(LUME_SYS_GETDENTS,(unsigned)fd,(uintptr_t)&e,sizeof(e)))>0)
        printf("%s%s\n",e.name,e.kind==LUME_NODE_DIR?"/":"");
    close(fd);return n<0;
}
int cp_main(int argc,char **argv)
{
    if(argc!=3 || !strcmp(argv[1],argv[2]))return 2;
    struct stat source,target;
    if(stat(argv[1],&source) || S_ISDIR(source.st_mode))return 1;
    if(!stat(argv[2],&target) && source.st_ino==target.st_ino && source.st_dev==target.st_dev)return 1;
    int in=open(argv[1],O_RDONLY);if(in<0)return 1;
    int out=open(argv[2],O_WRONLY|O_CREAT|O_TRUNC);if(out<0){close(in);return 1;}
    char buf[512];ssize_t n;int error=0;
    while((n=read(in,buf,sizeof(buf)))>0)if(write(out,buf,(size_t)n)!=n){error=1;break;}
    close(in);close(out);return error || n<0;
}
int rm_main(int argc,char **argv)
{
    if(argc<2)return 2;int error=0;
    for(int i=1;i<argc;++i)if(argv[i][0]=='-' || unlink(argv[i]))error=1;
    return error;
}
int touch_main(int argc,char **argv)
{
    if(argc<2)return 2;int error=0;
    for(int i=1;i<argc;++i){struct stat s;if(!stat(argv[i],&s)){bb_error_msg("timestamp updates are not available");error=1;continue;}int fd=open(argv[i],O_WRONLY|O_CREAT);if(fd<0)error=1;else close(fd);}
    return error; /* Creation only; this ABI does not expose file timestamps. */
}
