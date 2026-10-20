#ifndef LUME_FD_CLIENT_H
#define LUME_FD_CLIENT_H
#include "api.h"
#include "fd.h"
static inline uint32_t lume_fd_pointer(const void *p){return (uint32_t)(uintptr_t)p;}
static inline struct lume_fd_request app_fd_request(void){
    return (struct lume_fd_request){.version=1,.bytes=sizeof(struct lume_fd_request),.fd=-1};
}
static inline int app_fd_call(unsigned op,struct lume_fd_request *r){return lume_syscall(LUME_FD,op,lume_fd_pointer(r),sizeof *r);}
static inline int app_fd_pair(int pair[2],unsigned duplex,unsigned flags){
    struct lume_fd_request r=app_fd_request();r.flags=flags;
    int n=app_fd_call(duplex?LUME_FD_SOCKETPAIR:LUME_FD_PIPE,&r);
    if(!n){pair[0]=r.fd;pair[1]=r.other;}return n;
}
static inline int app_fd_dup(int fd,int to,unsigned exact,unsigned flags){
    struct lume_fd_request r=app_fd_request();r.fd=fd;r.other=to;r.flags=flags;
    return app_fd_call(exact?LUME_FD_DUP2:LUME_FD_DUP,&r);
}
static inline int app_fd_flags(int fd,unsigned op,unsigned flags){
    struct lume_fd_request r=app_fd_request();r.fd=fd;r.flags=flags;return app_fd_call(op,&r);
}
static inline int app_poll(struct lume_pollfd *p,unsigned count,uint32_t timeout){
    struct lume_fd_request r=app_fd_request();r.buffer=lume_fd_pointer(p);r.count=count;r.timeout_ms=timeout;
    return app_fd_call(LUME_FD_POLL,&r);
}
static inline int app_spawn_fds(const char *path,unsigned argc,const char *const *argv,unsigned envc,const char *const *envp){
    struct lume_spawn_request spawn={lume_fd_pointer(path),argc,lume_fd_pointer(argv),envc,lume_fd_pointer(envp)};
    struct lume_fd_request r=app_fd_request();r.buffer=lume_fd_pointer(&spawn);return app_fd_call(LUME_FD_SPAWN,&r);
}
#endif
