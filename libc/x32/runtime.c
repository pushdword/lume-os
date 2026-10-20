/* SPDX-License-Identifier: BSD-3-Clause
 * Picolibc services for a private Lume ring3 process. No bare-metal BSP. */
#define _DEFAULT_SOURCE
#include "api.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

extern char **environ;
static char *empty_env[]={NULL};
static int raw_input;
/* Each process owns its stdio state. Keep short interactive lines visible,
 * but send terminal redraws in blocks instead of entering the kernel per byte. */
static char output_buffer[512];
static unsigned output_used;
static char error_buffer[512];
static unsigned error_used;
/* Prefetch only redirected stdio input. Terminal and raw input retain their
 * one-byte requests, and read(0) consumes this queue before touching the FD. */
static unsigned char input_buffer[512];
static unsigned input_start,input_used;
static int input_kind; /* Unknown, terminal, redirected. */
static int out_flush(FILE *stream);
static int err_flush(FILE *stream);
static void input_reset(void){input_start=input_used=0;input_kind=0;}
static int input_is_terminal(void){
    if(!input_kind){
        struct lume_stat s;
        int r=lume_syscall(LUME_FSTAT,0,(uint32_t)(uintptr_t)&s,0);
        /* An older kernel without metadata must keep interactive semantics. */
        input_kind=r<0||(s.mode&0170000)==0020000?1:2;
    }return input_kind==1;
}
void lume_stdio_raw(int raw){raw_input=!!raw;}
void lume_runtime_init(unsigned argc,char **argv,char **envp){
    (void)argc;(void)argv;environ=envp?envp:empty_env;
}
void fflush_all_streams(void){(void)fflush(stdout);(void)fflush(stderr);}
static int result(int r){if(r<0){errno=-r;return -1;}return r;}
static ssize_t write_raw(int fd,const void *buffer,size_t bytes){
    size_t done=0;
    while(done<bytes){unsigned take=bytes-done;if(take>4096)take=4096;
        int n=app_write(fd,(const char *)buffer+done,take);
        if(n==-EINTR)continue;
        if(n<0){errno=-n;return done?(ssize_t)done:-1;}
        if(!n||(unsigned)n>take){errno=EIO;return done?(ssize_t)done:-1;}done+=(unsigned)n;
    }return (ssize_t)done;
}
ssize_t write(int fd,const void *buffer,size_t bytes){
    /* A direct write must not overtake older stdio output. Still deliver
     * stderr diagnostics if stdout fails; its unsent suffix remains queued. */
    if(bytes&&output_used&&(fd==1||fd==2)&&fflush(stdout)<0&&fd==1)return -1;
    if(bytes&&error_used&&(fd==1||fd==2)&&fflush(stderr)<0)return -1;
    return write_raw(fd,buffer,bytes);
}
ssize_t read(int fd,void *buffer,size_t bytes){
    if(!bytes)return result(app_read(fd,buffer,0));
    /* Includes read(0) as well as fgetc/scanf: display prompts before waiting. */
    if(fd==0&&output_used&&fflush(stdout)<0)return -1;
    if(fd==0&&error_used&&fflush(stderr)<0)return -1;
    unsigned take=bytes>4096?4096:bytes;
    if(fd==0&&input_used){
        if(take>input_used)take=input_used;
        memcpy(buffer,input_buffer+input_start,take);
        input_start+=take;input_used-=take;return (ssize_t)take;
    }
    for(;;){int r=app_read(fd,buffer,take);
        if(r==-EAGAIN&&fd==0&&input_is_terminal()){
            int wait=lume_syscall(LUME_TERMINAL,5,1000,0);
            if(wait==-EINVAL||wait==-ENOSYS||wait==-ENOTSUP)lume_syscall(LUME_YIELD,0,0,0);
            continue;
        }
        if(r>(int)take){errno=EIO;return -1;}
        if(r>0&&fd==0&&!raw_input&&input_is_terminal()){unsigned char *p=buffer;
            /* UART-only input: Ctrl-D supplies EOF at a read boundary. */
            if(p[0]==4)return 0;
            for(int i=0;i<r;i++)if(p[i]=='\r')p[i]='\n';
        }return result(r);
    }
}
int open(const char *path,int flags,...){
    unsigned known=O_ACCMODE|O_CREAT|O_TRUNC|O_APPEND|O_EXCL;
    if((unsigned)flags&~known){errno=EINVAL;return -1;}
    unsigned mode=(unsigned)flags&O_ACCMODE;
    if(mode>O_RDWR){errno=EINVAL;return -1;}
    unsigned f=mode==O_WRONLY?LUME_WRONLY:mode==O_RDWR?LUME_RDWR:LUME_RDONLY;
    if(flags&O_CREAT)f|=LUME_CREAT;if(flags&O_TRUNC)f|=LUME_TRUNC;
    if(flags&O_APPEND)f|=LUME_APPEND;if(flags&O_EXCL)f|=LUME_EXCL;
    int r=app_open(path,f);if(r==0)input_reset();return result(r);
}
int close(int fd){int r=app_close(fd);if(!r&&fd==0)input_reset();return result(r);}
off_t lseek(int fd,off_t offset,int whence){
    if(offset>INT32_MAX||offset<INT32_MIN){errno=EOVERFLOW;return -1;}
    /* The underlying file offset includes the prefetched suffix. A relative
     * seek is measured from the next byte that the application would read. */
    if(fd==0&&input_used&&whence==SEEK_CUR){
        if(offset<INT32_MIN+(off_t)input_used){errno=EOVERFLOW;return -1;}
        offset-=(off_t)input_used;
    }
    int r=app_seek(fd,(int32_t)offset,(unsigned)whence);
    if(r>=0&&fd==0)input_start=input_used=0;return result(r);
}
int fsync(int fd){return result(app_sync(fd));}
int stat(const char *path,struct stat *out){
    struct lume_stat s;int r=app_stat(path,&s);if(r<0)return result(r);
    memset(out,0,sizeof *out);out->st_size=s.size;out->st_mode=s.mode;
    out->st_uid=s.uid;out->st_gid=s.gid;out->st_nlink=1;out->st_blksize=512;
    out->st_blocks=(s.size+511)/512;return 0;
}
int fstat(int fd,struct stat *out){
    struct lume_stat s;int r=lume_syscall(LUME_FSTAT,(uint32_t)fd,(uint32_t)(uintptr_t)&s,0);if(r<0)return result(r);
    memset(out,0,sizeof *out);out->st_size=s.size;out->st_mode=s.mode;out->st_uid=s.uid;out->st_gid=s.gid;
    out->st_nlink=1;out->st_blksize=512;out->st_blocks=(s.size+511)/512;return 0;
}
int isatty(int fd){
    struct lume_stat s;int r=lume_syscall(LUME_FSTAT,(uint32_t)fd,(uint32_t)(uintptr_t)&s,0);
    if(r<0){errno=-r;return 0;}
    if((s.mode&0170000)==0020000)return 1;errno=ENOTTY;return 0;
}
int chdir(const char *path){return result(lume_syscall(LUME_CHDIR,(uint32_t)(uintptr_t)path,0,0));}
char *getcwd(char *out,size_t size){
    int allocated=0;if(!out){if(!size)size=128;out=malloc(size);if(!out)return NULL;allocated=1;}
    int r=lume_syscall(LUME_GETCWD,(uint32_t)(uintptr_t)out,size,0);
    if(r<0){errno=-r;if(allocated)free(out);return NULL;}return out;
}
int unlink(const char *path){return result(lume_syscall(LUME_UNLINK,(uint32_t)(uintptr_t)path,0,0));}
int rename(const char *a,const char *b){return result(lume_syscall(LUME_RENAME,(uint32_t)(uintptr_t)a,(uint32_t)(uintptr_t)b,0));}
int mkdir(const char *path,mode_t mode){(void)mode;return result(lume_syscall(LUME_MKDIR,(uint32_t)(uintptr_t)path,0,0));}
int rmdir(const char *path){return result(lume_syscall(LUME_RMDIR,(uint32_t)(uintptr_t)path,0,0));}
static int out_flush(FILE *stream){
    if(!output_used)return 0;
    ssize_t n=write_raw(1,output_buffer,output_used);
    if(n>0){output_used-=(unsigned)n;if(output_used)memmove(output_buffer,output_buffer+n,output_used);}
    if(output_used){
        /* Picolibc's fflush returns the backend result without setting ferror.
         * Set it here for flush failures as well as character callback errors. */
        stream->flags|=__SERR;return _FDEV_ERR;
    }return 0;
}
static int out_put(char c,FILE *stream){
    if(error_used&&fflush(stderr)<0)return _FDEV_ERR;
    if(output_used==sizeof output_buffer&&out_flush(stream)<0)return _FDEV_ERR;
    output_buffer[output_used++]=c;
    return output_used==sizeof output_buffer||c=='\n'?out_flush(stream):0;
}
/* Keep diagnostics line buffered too. A formatter may call this callback for
 * each character, but only complete lines/blocks enter the kernel. Preserve
 * partial writes so a later fflush can retry without losing or duplicating it. */
static int err_flush(FILE *stream){
    if(!error_used)return 0;
    ssize_t n=write_raw(2,error_buffer,error_used);
    if(n>0){error_used-=(unsigned)n;if(error_used)memmove(error_buffer,error_buffer+n,error_used);}
    if(error_used){stream->flags|=__SERR;return _FDEV_ERR;}return 0;
}
static int err_put(char c,FILE *stream){
    if(output_used&&!error_used)(void)fflush(stdout);
    if(error_used==sizeof error_buffer&&err_flush(stream)<0)return _FDEV_ERR;
    error_buffer[error_used++]=c;
    return error_used==sizeof error_buffer||c=='\n'?err_flush(stream):0;
}
static int in_get(FILE *stream){
    (void)stream;
    if(input_used){
        if(output_used&&fflush(stdout)<0)return _FDEV_ERR;
        if(error_used&&fflush(stderr)<0)return _FDEV_ERR;
        --input_used;return input_buffer[input_start++];
    }
    unsigned take=raw_input||input_is_terminal()?1:sizeof input_buffer;
    ssize_t n;do{n=read(0,input_buffer,take);}while(n<0&&errno==EINTR);
    if(n>0){input_start=1;input_used=(unsigned)n-1;return input_buffer[0];}
    return n==0?_FDEV_EOF:_FDEV_ERR;
}
static int no_flush(FILE *stream){(void)stream;return 0;}
static FILE input=FDEV_SETUP_STREAM(NULL,in_get,no_flush,_FDEV_SETUP_READ);
static FILE output=FDEV_SETUP_STREAM(out_put,NULL,out_flush,_FDEV_SETUP_WRITE);
static FILE errors=FDEV_SETUP_STREAM(err_put,NULL,err_flush,_FDEV_SETUP_WRITE);
FILE *const stdin=&input;
FILE *const stdout=&output;
FILE *const stderr=&errors;
/* Picolibc exit reaches this hook after its termination callbacks. Unlike the
 * old byte-at-a-time backend it may still have output pending at that point. */
_Noreturn void _exit(int status){fflush_all_streams();app_exit((unsigned)status);}
_Noreturn void _Exit(int status){app_exit((unsigned)status);}
