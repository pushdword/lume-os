/* Pipe-backed SSH terminal output processing. Non-PTY channels stay byte exact.
 * Buffer expansion separately so even a one-byte channel window can drain,
 * and select sees pending output after the child has closed its pipe. */
#include "terminal.h"
#include "errno_map.h"
#include <string.h>
struct terminal_output {int active,fd;unsigned previous_cr,at,used;unsigned char data[1024];};
static struct terminal_output outputs[2];
static struct terminal_output *output(int fd){
 for(unsigned i=0;i<2;i++)if(outputs[i].active&&outputs[i].fd==fd)return outputs+i;
 return 0;
}
void ssh_terminal_register(int out,int err,int tty){
 memset(outputs,0,sizeof outputs);
 if(tty){outputs[0].active=outputs[1].active=1;outputs[0].fd=out;outputs[1].fd=err;}
}
void ssh_terminal_forget(int fd){struct terminal_output *s=output(fd);if(s)memset(s,0,sizeof *s);}
unsigned ssh_terminal_pending(int fd){struct terminal_output *s=output(fd);return s?s->used-s->at:0;}
int ssh_terminal_read(int fd,void *data,unsigned size){
 struct terminal_output *s=output(fd);
 if(!s||!size)return lume_posix_read(fd,data,size);
 if(s->at==s->used){
  unsigned char input[512];int n=lume_posix_read(fd,input,sizeof input);if(n<=0)return n;
  s->at=s->used=0;
  for(unsigned i=0;i<(unsigned)n;i++){
   unsigned char c=input[i];
   if(c=='\n'&&!s->previous_cr)s->data[s->used++]='\r';
   s->data[s->used++]=c;s->previous_cr=c=='\r';
  }
 }
 unsigned n=s->used-s->at;if(size<n)n=size;
 memcpy(data,s->data+s->at,n);s->at+=n;return (int)n;
}
