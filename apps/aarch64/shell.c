#include "lib.h"
#include <stdbool.h>
#include "net_client.h"
static int checks;
static void hex(uint64_t n) {char b[19]="0x0000000000000000";const char *d="0123456789abcdef";for(unsigned i=0;i<16;++i)b[17-i]=d[(n>>(i*4))&15];puts(b);puts("\n");}
#define CHECK(c) do {if(!(c)) {puts("LUME SHELL ABI CHECK FAIL number=");hex((uint64_t)checks+1);return 1;} ++checks;} while(0)
static int abi_check(int script_fd)
{
    CHECK(call(LUME_SYS_ABI,0,0,0)==LUME_ABI_VERSION);
    CHECK(call(0xffff,0,0,0)==-LUME_ENOSYS);
    CHECK(call(LUME_SYS_GETPID,0,0,0)==1);
    CHECK(call(LUME_SYS_NETCTL,0,0,1)==-LUME_EINVAL);
    CHECK(call(LUME_SYS_NETCTL,10,0,sizeof(struct lume_net_control))==-LUME_EINVAL);
    CHECK(call(LUME_SYS_NETCTL,0,0x200000,sizeof(struct lume_net_control))==-LUME_EFAULT);
    CHECK(call(LUME_SYS_NETCTL,0,(uintptr_t)"readonly",sizeof(struct lume_net_control))==-LUME_EFAULT);
    net_init(0);net_request.version=0;CHECK(net_call(LUME_NET_ENUM)==-LUME_EINVAL);
    net_init(0);net_request.reserved=1;CHECK(net_call(LUME_NET_ENUM)==-LUME_EINVAL);
    net_init(0);net_request.cursor=UINT64_MAX;CHECK(net_call(LUME_NET_EVENTS)==-LUME_EINVAL);
    net_init(UINT32_MAX);CHECK(net_call(LUME_NET_GET)==-LUME_ENOENT);
    net_init(0);CHECK(net_call(LUME_NET_ENUM)==0 && net_request.count<=LUME_NET_INTERFACES);
    if(net_request.count) {
        uint32_t index=net_request.data.interfaces[0].ifindex;
        net_init(index);CHECK(net_call(LUME_NET_GET)==0 && net_request.data.interfaces[0].ifindex==index);
        struct lume_net_request legacy;net_zero(&legacy,sizeof(legacy));
        CHECK(call(LUME_SYS_NET,0,(uintptr_t)&legacy,sizeof(legacy))>=0);
        puts("LUME SHELL LEGACY NET ABI CHECK PASS\n");
    }
    puts("LUME SHELL STRUCTURED NET API CHECK PASS\n");
    CHECK(call(LUME_SYS_NET,0,0x200000,sizeof(struct lume_net_request))==-LUME_EFAULT);
    CHECK(call(LUME_SYS_NET,0,0,1)==-LUME_EINVAL);
    CHECK(call(LUME_SYS_NET,8,0,sizeof(struct lume_net_request))==-LUME_EINVAL);
    CHECK(call(LUME_SYS_OPEN,0x200000,0,0)==-LUME_EFAULT);
    CHECK(call(LUME_SYS_WRITE,1,0x200000,1)==-LUME_EFAULT);
    CHECK(call(LUME_SYS_READ,0,0x200000,1)==-LUME_EFAULT);
    CHECK(call(LUME_SYS_READ,0,(uintptr_t)"readonly",1)==-LUME_EFAULT);
    CHECK(call(LUME_SYS_WRITE,1,UINT64_MAX,2)==-LUME_EFAULT);
    CHECK(call(LUME_SYS_READ,0,(UINT64_C(0x4000000000)+0x23fe0),64)==-LUME_EFAULT);
    CHECK(call(LUME_SYS_GETCWD,(uintptr_t)"readonly",64,0)==-LUME_EFAULT);
    CHECK(call(LUME_SYS_READ,0,0,4097)==-LUME_EINVAL);
    char longname[LUME_PATH_MAX];for(unsigned i=0;i<sizeof(longname);++i)longname[i]='a';
    CHECK(open(longname,0)==-LUME_ENAMETOOLONG);
    CHECK(close(99)==-LUME_EBADF);
    CHECK(open("/missing",0)==-LUME_ENOENT);
    CHECK(open("/etc/motd",LUME_O_WRONLY)==-LUME_EROFS);
    CHECK(open("/etc/motd/../motd",0)==-LUME_ENOTDIR);
    CHECK(open("/tmp/x",0x8000)==-LUME_EINVAL);
    int fd=(int)open("/tmp/abi",LUME_O_CREAT|LUME_O_RDWR|LUME_O_TRUNC);
    CHECK(fd>=3);
    CHECK(write(fd,"abc",3)==3);
    CHECK(call(LUME_SYS_SEEK,fd,1,LUME_SEEK_SET)==1);
    char b[8]={0}; CHECK(read(fd,b,7)==2 && eq(b,"bc"));
    CHECK(read(fd,b,1)==0);
    struct lume_stat st;
    CHECK(call(LUME_SYS_STAT,fd,(uintptr_t)&st,0)==0 && st.size==3 && st.kind==LUME_NODE_FILE && st.reserved==0);
    CHECK(call(LUME_SYS_SEEK,fd,(uint64_t)INT64_MIN,LUME_SEEK_CUR)==-LUME_EINVAL);
    CHECK(close(fd)==0);
    fd=(int)open("/tmp/abi",LUME_O_WRONLY|LUME_O_APPEND); CHECK(fd>=3);
    CHECK(write(fd,"d",1)==1 && close(fd)==0);
    fd=(int)open("/tmp/abi",0); CHECK(fd>=3);
    for(unsigned i=0;i<sizeof(b);++i)b[i]=0;
    CHECK(read(fd,b,7)==4 && eq(b,"abcd") && close(fd)==0);
    int fds[LUME_FD_MAX]; unsigned count=0;
    while(count<LUME_FD_MAX && (fd=(int)open("/etc/motd",0))>=0) fds[count++]=fd;
    unsigned opened=count;
    while(count) CHECK(close(fds[--count])==0);
    CHECK(opened==(script_fd?12u:13u) && fd==-LUME_EMFILE);
    puts("LUME SHELL ABI VFS CHECK PASS\n"); return 0;
}
static int run(const char *path,char **argv,unsigned argc)
{
    return (int)call(LUME_SYS_RUN,(uintptr_t)path,(uintptr_t)argv,argc);
}
static int program_check(void)
{
    char *yes[]={"true",0},*no[]={"false",0};
    CHECK(call(LUME_SYS_RUN,0,0,1)==-LUME_EFAULT);
    CHECK(call(LUME_SYS_RUN,0,0,17)==-LUME_E2BIG);
    CHECK(call(LUME_SYS_RUN,(uintptr_t)"/bin/true",0x200000,1)==-LUME_EFAULT);
    CHECK(run("/missing",yes,1)==-LUME_ENOENT);
    CHECK(run("/etc/programs",yes,1)==-LUME_ENOEXEC);
    CHECK(run("/bin/false",no,1)==1);
    CHECK(call(LUME_SYS_CHDIR,(uintptr_t)"/tmp",0,0)==0);
    char *probe[]={"progtest","argument with spaces","olá",0};
    CHECK(run("/bin/progtest",probe,3)==7);
    CHECK(run("/bin/progtest",probe,3)==7);
    char *fault[]={"progtest","fault",0};CHECK(run("/bin/progtest",fault,2)==139);
    int fd=(int)open("/tmp/self",LUME_O_CREAT|LUME_O_WRONLY|LUME_O_TRUNC);
    CHECK(fd>=3 && write(fd,"keep",4)==4 && close(fd)==0);
    char *copy[]={"cp","/tmp/self","/tmp/./self",0};CHECK(run("/bin/cp",copy,3)==1);
    char data[8]={0};fd=(int)open("/tmp/self",0);CHECK(fd>=3 && read(fd,data,7)==4 && eq(data,"keep") && close(fd)==0);
    for(unsigned i=0;i<12;++i)CHECK(run("/bin/true",yes,1)==0);
    char cwd[128];CHECK(call(LUME_SYS_GETCWD,(uintptr_t)cwd,sizeof(cwd),0)>0 && eq(cwd,"/tmp"));
    CHECK(call(LUME_SYS_CHDIR,(uintptr_t)"/",0,0)==0);
    puts("LUME SHELL PROGRAM CHECK PASS\n");return 0;
}
static int external(char **args,unsigned count)
{
    char path[LUME_PATH_MAX];unsigned n=0;
    if(args[0][0]=='/' || args[0][0]=='.') {
        if(len(args[0])>=sizeof(path))return 126;
    } else {const char *prefix="/bin/";while(*prefix)path[n++]=*prefix++;}
    for(unsigned i=0;args[0][i];++i){if(n+1>=sizeof(path))return 126;path[n++]=args[0][i];}path[n]=0;
    int target=-1,saved=-1;
    if(count>=3 && (eq(args[count-2],">") || eq(args[count-2],">>"))) {
        uint64_t flags=LUME_O_CREAT|LUME_O_WRONLY|(eq(args[count-2],">>")?LUME_O_APPEND:LUME_O_TRUNC);
        target=(int)open(args[count-1],flags);if(target<0)return 1;
        saved=(int)open("/dev/console",LUME_O_WRONLY);
        if(saved<0){close(target);return 1;}
        if(call(LUME_SYS_DUP2,1,(unsigned)saved,0)<0 || call(LUME_SYS_DUP2,(unsigned)target,1,0)<0){close(target);close(saved);return 1;}
        count-=2;
    }
    args[count]=0;int status=run(path,args,count);
    if(saved>=0){(void)call(LUME_SYS_DUP2,(unsigned)saved,1,0);close(saved);close(target);}
    if(status<0){puts("shell: programa indisponível\n");return 127;}return status;
}
static int line(int fd,char *p,size_t capacity,int interactive)
{
    size_t n=0;int overflow=0;
    for(;;) {
        char c; int64_t r=read(fd,&c,1);
        if(r==-LUME_EAGAIN)continue;
        if(r<0)return -1;
        if(!r){p[n]=0;return n?1:0;}
        if(c=='\r')c='\n';
        if(c=='\n'){p[n]=0;return overflow?-1:1;}
        if(c==8 || c==127) {
            if(n) {
                do{--n;}while(n && ((unsigned char)p[n]&0xc0)==0x80);
                if(interactive)write(1,"\b \b",3);
            }
            continue;
        }
        if(c==3 || c==21) {
            while(n) {
                do{--n;}while(n && ((unsigned char)p[n]&0xc0)==0x80);
                if(interactive)write(1,"\b \b",3);
            }
            overflow=0;continue;
        }
        if(interactive)write(1,&c,1);
        if(n+1<capacity)p[n++]=c;else overflow=1;
    }
}
int main(int argc,char **argv)
{
    puts("Lume shell / AArch64 / public ABI 1.0\n");
    if(call(LUME_SYS_ABI,0,0,0)!=LUME_ABI_VERSION)return 70;
    int input=0;
    if(argc==2) {input=(int)open(argv[1],0);if(input<0)return 66;}
    else if(argc!=1)return 64;
    int failed=0;
    for(;;) {
        char buffer[256],*args[24];unsigned count=0;
        if(!input)puts("lume> ");
        int r=line(input,buffer,sizeof(buffer),!input);
        if(!r)break;
        if(r<0) {
            if(!input){puts("\nLinha demasiado longa.\n");continue;}
            failed=1;break;
        }
        if(!input)puts("\n");
        char *p=buffer,*out=buffer;int parse_error=0;
        while(*p) {
            while(*p==' ' || *p=='\t')++p;
            if(!*p || *p=='#')break;
            if(count>=LUME_ARG_MAX){parse_error=1;break;}
            args[count++]=out;char quote=0;
            while(*p && (quote || (*p!=' ' && *p!='\t'))) {
                char c=*p++;
                if(c=='\'' || c=='"'){if(!quote){quote=c;continue;}if(quote==c){quote=0;continue;}}
                if(c=='\\' && quote!='\'' && *p && (*p=='\\' || *p=='"' || !quote))c=*p++;
                *out++=c;
            }
            if(quote){parse_error=1;break;}
            while(*p==' ' || *p=='\t')++p;
            *out++=0;
        }
        if(parse_error){puts("Demasiados argumentos.\n");if(input)failed=1;continue;}
        if(!count)continue;
        int error=0;
        if(eq(args[0],"exit")) {if(count>1 && !eq(args[1],"0"))failed=1;break;}
        else if(eq(args[0],"linux") || eq(args[0],"reboot")) {puts("A regressar ao Raspberry Pi OS...\n");break;}
        else if(eq(args[0],"abi-check"))error=abi_check(input);
        else if(eq(args[0],"program-check"))error=program_check();
        else if(eq(args[0],"help"))puts("help cd net exit linux reboot abi-check program-check; external tools in /bin\n");
        else if(eq(args[0],"net"))error=network(args,count);
        else if(eq(args[0],"cd"))error=call(LUME_SYS_CHDIR,(uintptr_t)(count>1?args[1]:"/"),0,0)<0;
        else error=external(args,count);
        if(error){puts("shell: command failed\n");if(input)failed=1;}
    }
    if(input)close(input);
    if(!failed && input && checks)puts("LUME SHELL SCRIPT PASS\n");
    return failed;
}
