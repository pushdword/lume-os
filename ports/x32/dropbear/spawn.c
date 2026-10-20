#include "config.h"
#include "platform.h"
#include "api.h"
#include "fd_client.h"
#include "terminal.h"
#include "errno_map.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
unsigned ssh_columns=80,ssh_rows=24;
int ssh_spawn_command(const char *command,const char *term,int *in,int *out,int *err,int *pid){
 int pipes[6]={-1,-1,-1,-1,-1,-1},saved[3]={-1,-1,-1};int result=-1;
 if(ssh_child_pid){errno=EBUSY;return -1;}
 if(command&&strlen(command)>240){errno=E2BIG;return -1;}
 for(unsigned i=0;i<3;i++){int n=app_fd_pair(pipes+2*i,0,LUME_FD_CLOEXEC);if(n<0){errno=lume_errno_from_syscall((unsigned)-n);goto done;}}
 for(unsigned i=0;i<3;i++){saved[i]=app_fd_dup(i,16,0,LUME_FD_CLOEXEC);if(saved[i]<0){errno=EMFILE;goto done;}}
 if(dup2(pipes[0],0)<0||dup2(pipes[3],1)<0||dup2(pipes[5],2)<0)goto restore;
 const char *argv[]={"/usr/bin/sshshell.x32", "-c",command,NULL};
 char termenv[128];snprintf(termenv,sizeof termenv,"TERM=%s",term?term:"dumb");
 char columns[24],rows[24];snprintf(columns,sizeof columns,"COLUMNS=%u",ssh_columns);snprintf(rows,sizeof rows,"LINES=%u",ssh_rows);
 const char *env[]={"HOME=/root","USER=root","LOGNAME=root","SHELL=/usr/bin/sshshell.x32","PATH=/usr/bin:/bin",termenv,"LUME_SSH=1",columns,rows,term?"LUME_SSH_TTY=1":"LUME_SSH_TTY=0",NULL};
 *pid=app_spawn_fds(argv[0],command?3:1,argv,10,env);if(*pid<0){errno=EIO;goto restore;}
 ssh_child_pid=*pid;result=0;
restore:
 for(unsigned i=0;i<3;i++)if(saved[i]>=0&&(dup2(saved[i],i)<0))result=-1;
done:
 for(unsigned i=0;i<3;i++)if(saved[i]>=0)close(saved[i]);
 if(result==0){*in=pipes[1];*out=pipes[2];*err=pipes[4];(void)fcntl(*in,F_SETFL,O_NONBLOCK);(void)fcntl(*out,F_SETFL,O_NONBLOCK);(void)fcntl(*err,F_SETFL,O_NONBLOCK);ssh_terminal_register(*out,*err,term!=NULL);pipes[1]=pipes[2]=pipes[4]=-1;}
 for(unsigned i=0;i<6;i++)if(pipes[i]>=0)close(pipes[i]);return result;
}
