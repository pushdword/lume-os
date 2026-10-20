/* Focused tests against the actual upstream process objects and port code. */
#include "config.h"
#undef NDEBUG
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "curses.h"
#include "api.h"
#include "perf.h"
#include "Platform.h"
#include "unsupported/UnsupportedProcess.h"
#include "UsersTable.h"
#include "ProcessTable.h"
#include "CPUMeter.h"
#include "Vector.h"
const char *program="htop";
static unsigned ticks_ms=1000,sleeps,input_waits,writes,bytes_written,write_limit;
static int failing_write,short_write;
static char output[65536];
static const char *input="";
static const char *filter_first,*help_dismiss;
static unsigned filter_caret_checks,help_published_before_input;
static struct lume_perf_snapshot injected;
static int memory_bad;
static int geometry=(24<<16)|80,auto_sample;
int32_t lume_syscall(uint32_t n,uint32_t a,uint32_t b,uint32_t c){
 (void)c;
 if(n==LUME_INFO)return a==LUME_CORES?2:a==LUME_UPTIME?(int32_t)ticks_ms:0;
 if(n==LUME_PERF){assert(b==sizeof injected);if(auto_sample){injected.uptime_ms=ticks_ms;for(unsigned i=0;i<2;i++){injected.cores[i].counter+=7500000;injected.cores[i].ticks[0]+=3000000;injected.cores[i].ticks[1]+=1500000;injected.cores[i].ticks[2]+=2250000;injected.cores[i].ticks[3]+=750000;}injected.processes[1].user_ticks+=1500000;}memcpy((void *)(uintptr_t)a,&injected,b);return 0;}
 if(n==LUME_MEMORY){struct lume_memory_info m={1,sizeof m,4096,1000,memory_bad?1001:750,10,0,0,0,0,0};assert(b==sizeof m);memcpy((void *)(uintptr_t)a,&m,b);return 0;}
 if(n==LUME_TERMINAL&&a==5){input_waits++;if(*input)return 0;ticks_ms+=b;return -11;}
 if(n==LUME_TERMINAL)return a==0?geometry:*input!=0;
 if(n==LUME_SLEEP){assert(a==10||a<=60000);ticks_ms+=250;sleeps++;return 0;}
 return -38;
}
void lume_stdio_raw(int n){(void)n;}
int app_write(int f,const void *p,unsigned n){assert(f==1);writes++;if(n>write_limit)write_limit=n;if(failing_write)return -5;if(short_write&&n>3)n=3;assert(bytes_written+n<sizeof output);memcpy(output+bytes_written,p,n);bytes_written+=n;output[bytes_written]=0;return n;}
int app_read(int f,void *p,unsigned n){assert(f==0&&n==1);if(!*input)return 0;
 if(input==help_dismiss){assert(strstr(output,"Process state: "));help_published_before_input=1;}
 if(filter_first&&input>=filter_first&&input<=filter_first+5){
  unsigned at=(unsigned)(input-filter_first);char caret[32];int bytes=snprintf(caret,sizeof caret,"\033[24;%uH",32+at);
  assert(strstr(output,"Filter: ")&&bytes_written>=(unsigned)bytes&&!memcmp(output+bytes_written-bytes,caret,(size_t)bytes));filter_caret_checks|=1u<<at;
 }
 *(char *)p=*input++;return 1;}
static void reset_output(void){writes=bytes_written=write_limit=0;output[0]=0;}
static void push_key(const char *s,int key){input=s;assert(getch()==key);assert(!*input);}
static void test_terminal(void){
 initscr();assert(COLS==80&&LINES==24);assert(refresh()==OK);assert(writes==1&&write_limit<=4096);reset_output();
 assert(refresh()==OK&&writes==0);
 init_pair(2,9,10);attrset(COLOR_PAIR(2)|A_BOLD|A_DIM|A_REVERSE);mvaddstr(1,2,"hello");assert(refresh()==OK);assert(writes==1&&strstr(output,"91;102;1;2;7mhello"));reset_output();
 mvaddch(2,4,'x');failing_write=1;assert(refresh()==ERR);failing_write=0;assert(refresh()==OK&&bytes_written>1900);reset_output();assert(refresh()==OK&&writes==0);
 short_write=1;mvaddstr(3,0,"short write");assert(refresh()==OK);assert(writes>1);short_write=0;reset_output();assert(refresh()==OK&&writes==0);
 /* Real upstream htop depends on curses getch's implicit refresh. */
 curs_set(0);reset_output();mvaddstr(5,0,"implicit refresh");push_key("q",'q');assert(strstr(output,"implicit refresh"));reset_output();push_key("q",'q');assert(writes==0);
 mvaddstr(5,0,"retry from getch");failing_write=1;input="q";assert(getch()==ERR&&*input=='q');failing_write=0;assert(getch()=='q'&&!*input&&strstr(output,"retry from getch"));reset_output();curs_set(1);
 assert(curs_set(0)==1);assert(strstr(output,"\033[?25l"));move(4,8);assert(curs_set(1)==0);assert(strstr(output,"\033[5;9H\033[?25h"));curs_set(0);
 push_key("\033[A",KEY_UP);push_key("\033[B",KEY_DOWN);push_key("\033[17~",KEY_F(6));push_key("\033[21~",KEY_F(10));push_key("q",'q');push_key("\003",'q');push_key("\r",KEY_ENTER);
 timeout(500);unsigned old=input_waits;assert(getch()==ERR&&input_waits-old==1);nodelay(stdscr,true);old=input_waits;assert(getch()==ERR&&input_waits==old);
 geometry=(48<<16)|128;assert(getch()==KEY_RESIZE);reset_output();assert(refresh()==OK&&writes==2&&write_limit==4096);reset_output();assert(refresh()==OK&&writes==0);geometry=(24<<16)|80;assert(getch()==KEY_RESIZE);
 assert(move(-1,0)==ERR&&move(0,80)==ERR);assert(endwin()==OK);
}
static void test_backend(void){
 memset(&injected,0,sizeof injected);injected.version=1;injected.bytes=sizeof injected;injected.clock_hz=30000000;injected.uptime_ms=1000;injected.core_count=2;injected.process_count=2;
 injected.processes[0].pid=0;injected.processes[0].parent=UINT32_MAX;injected.processes[0].state=LUME_WAITING;strcpy(injected.processes[0].name,"init.x32");
 injected.processes[1].pid=7;injected.processes[1].parent=0;injected.processes[1].state=LUME_RUNNING;injected.processes[1].user_ticks=1705032704;strcpy(injected.processes[1].name,"clang.x32");
 assert(Platform_init()&&Settings_isReadonly());UsersTable *u=UsersTable_new();Machine *m=Machine_new(u,0);ProcessTable *t=ProcessTable_new(m,NULL);
 Settings *settings=Settings_new(m,NULL,NULL,NULL);Machine_populateTablesFromSettings(m,settings,&t->super);
 Machine_scan(m);ProcessTable_goThroughEntries(t);bool exists;Process *init=ProcessTable_getProcess(t,0,&exists,UnsupportedProcess_new);assert(exists&&Process_getPid(init)==0&&init->state==SLEEPING);
 Process *p=ProcessTable_getProcess(t,7,&exists,UnsupportedProcess_new);assert(exists&&isnan(p->percent_cpu)&&!strcmp(p->procComm,"clang.x32")&&p->time==0);
 Meter meter={0};double values[CPU_METER_ITEMCOUNT]={0};meter.values=values;Platform_setMemoryValues(&meter);assert(meter.total==4000&&values[0]==1000);
 injected.uptime_ms+=1000;for(unsigned c=0;c<2;c++){injected.cores[c].counter+=30000000;injected.cores[c].ticks[0]+=12000000;injected.cores[c].ticks[1]+=6000000;injected.cores[c].ticks[2]+=9000000;injected.cores[c].ticks[3]+=3000000;}
 injected.processes[1].user_ticks+=6000000;injected.processes[1].kernel_ticks=3000000;
 Machine_scan(m);ProcessTable_goThroughEntries(t);assert(fabs(p->percent_cpu-30)<0.001&&p->time==30);assert(fabs(Platform_setCPUValues(&meter,0)-70)<0.001&&fabs(values[CPU_METER_NORMAL]-40)<0.001);assert(fabs(Platform_setCPUValues(&meter,2)-70)<0.001);
 assert(Process_compareByKey_Base(p,init,PERCENT_CPU)>0);t->super.incFilter="CLANG";assert(!Process_rowMatchesFilter(&p->super,&t->super)&&Process_rowMatchesFilter(&init->super,&t->super));t->super.incFilter=NULL;
 /* Fractional centiseconds accumulate rather than disappearing per refresh. */
 for(int i=0;i<4;i++){injected.uptime_ms+=1;injected.cores[0].counter+=30000;injected.processes[1].user_ticks+=90000;Machine_scan(m);ProcessTable_goThroughEntries(t);}assert(p->time==31);
 /* Both process category counters and free-running wall counter wrap u32. */
 injected.uptime_ms+=1000;injected.cores[0].counter=UINT32_MAX-100;injected.processes[1].user_ticks=UINT32_MAX-100;Machine_scan(m);ProcessTable_goThroughEntries(t);
 injected.uptime_ms+=1000;injected.cores[0].counter=29999899;injected.processes[1].user_ticks=899899;Machine_scan(m);ProcessTable_goThroughEntries(t);assert(fabs(p->percent_cpu-3)<0.001);
 uint64_t before_gap=p->time;injected.uptime_ms+=144000;injected.processes[1].user_ticks=1;settings->ss->treeView=true;Machine_scan(m);ProcessTable_goThroughEntries(t);assert(p->time==before_gap&&isnan(p->percent_cpu)&&!settings->ss->treeView);
 memory_bad=1;Machine_scan(m);Platform_setMemoryValues(&meter);assert(isnan(meter.total)&&isnan(values[0]));
 Htop_Action keys[KEY_MAX+256]={0};Action_setBindings(keys);Platform_setBindings(keys);assert(keys['q']&&keys[KEY_F(6)]&&keys['/']&&keys['\\']&&!keys['M']&&!keys['k']&&!keys['s']&&!keys['t']&&!keys[KEY_F(5)]);
 /* Restore valid machine memory before destruction. */
 Settings_delete(settings);Machine_delete(m);UsersTable_delete(u);
}
int CommandLine_run(int,char **);
int main(void){test_terminal();test_backend();
 memory_bad=0;auto_sample=1;injected.core_count=2;input="";geometry=(24<<16)|80;reset_output();
 const char *rc=getenv("LUME_HTOP_HOST_RC");assert(rc);setenv("HTOPRC",rc,1);
 int interactive=getenv("LUME_HTOP_HOST_INTERACTIVE")!=NULL;
 char *finite[]={"htop","-n","2","-d","1",NULL};char *keys[]={"htop","-d","1",NULL};
 if(interactive){input="\033[11~ \033[14~clang\rq";help_dismiss=input+5;filter_first=input+11;}
 assert(CommandLine_run(interactive?3:5,interactive?keys:finite)==0);
 const char *capture=getenv("LUME_HTOP_HOST_CAPTURE");assert(capture);FILE *dump=fopen(capture,"wb");assert(dump);assert(fwrite(output,1,bytes_written,dump)==bytes_written);assert(!fclose(dump));
 assert(strstr(output,"init.x32")&&strstr(output,"clang.x32")&&strstr(output,"UPg")&&strstr(output,"PID")&&strstr(output,"Help"));
 if(interactive)assert(!*input&&strstr(output,"Filter")&&strstr(output,"clang")&&filter_caret_checks==63&&help_published_before_input);
 puts("PASS original htop UI finite / F1 help before input / F4 filter caret six positions / Lume CPU PID0 pool / implicit refresh diff retry keys sleep");return 0;}
