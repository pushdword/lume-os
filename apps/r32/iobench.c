/* Finite IO/compute benchmark using INFO uptime, never PERF or user MMIO.
 * Measurements stay in private RAM until all timed work has finished. */
#include "api.h"
static unsigned char bytes[8192];
struct result {unsigned kind,count,bytes,ms,hash;};
static struct result results[8];static unsigned used;
static uint32_t now(void){return (uint32_t)lume_syscall(LUME_INFO,LUME_UPTIME,0,0);}
static int output(const unsigned char *p,unsigned n){
    while(n){unsigned take=n>4096?4096:n;int r=app_write(1,p,take);if(r==-4)continue;
        if(r<=0||(unsigned)r>take)return 0;p+=r;n-=(unsigned)r;}return 1;
}
static void save(unsigned kind,unsigned count,unsigned size,unsigned start,unsigned hash){
    results[used++]=(struct result){kind,count,size,now()-start,hash};
}
static void report(void){
    for(unsigned i=0;i<used;i++){
        const struct result *r=results+i;app_puts("IOBENCH kind=");app_number(r->kind);
        app_puts(" count=");app_number(r->count);app_puts(" bytes=");app_number(r->bytes);
        app_puts(" ms=");app_number(r->ms);app_puts(" hash=");app_number(r->hash);app_puts("\n");
    }
}
static int video(void){
    int old=lume_syscall(LUME_TERMINAL,3,0,0);if(old<0||lume_syscall(LUME_TERMINAL,2,1,0)<0)return 0;
    unsigned g=(unsigned)lume_syscall(LUME_TERMINAL,0,0,0),columns=g&65535,rows=g>>16;
    if(!columns||columns>128||!rows||rows>48)return 0;
    unsigned n=0;const unsigned char prefix[]="\033[2J\033[H";
    for(unsigned i=0;i<sizeof prefix-1;i++)bytes[n++]=prefix[i];
    for(unsigned row=0;row<rows;row++){
        for(unsigned col=0;col<columns;col++)bytes[n++]='.';
        if(row+1<rows)bytes[n++]='\n';
    }
    if(!output(bytes,n))return 0;
    for(unsigned kind=0;kind<6;kind++){
        unsigned start=now(),count=kind<2?8:128,total=0;
        for(unsigned i=0;i<count;i++){
            const unsigned char *p=bytes;unsigned size=n;
            if(kind==1)bytes[sizeof prefix-1]=(unsigned char)('A'+i%26);
            if(kind==2){p=(const unsigned char *)"iobench scroll\n";size=15;}
            if(kind==3){p=(const unsigned char *)"x";size=1;}
            if(kind==4){p=(const unsigned char *)"\n";size=1;}
            if(kind==5){p=(const unsigned char *)"\033[2;2Hx";size=7;}
            if(!output(p,size))return 0;total+=size;
        }save(kind+1,count,total,start,0);
    }
    return lume_syscall(LUME_TERMINAL,2,(unsigned)old,0)>=0;
}
static int uart(unsigned blocks){
    static const unsigned sizes[]={0,1,511,512,4095,4096};
    for(unsigned i=0;i<4096;i++)bytes[i]=(unsigned char)('A'+i%26);
    for(unsigned k=0;k<6;k++){
        unsigned count=k==5?blocks:1,start=now();
        for(unsigned i=0;i<count;i++)if(!output(bytes,sizes[k]))return 0;
        save(10+k,count,sizes[k]*count,start,0);
    }return 1;
}
static void compute(void){
    uint32_t hash=2166136261u,start=now();
    for(unsigned i=0;i<300000;i++){hash^=i;hash*=16777619u;}
    save(20,300000,0,start,hash);
}
int app_main(int argc,char **argv){
    if(argc!=2){app_puts("usage: iobench video|uart|compute|mixed\n");return 2;}
    unsigned perf=(unsigned)lume_syscall(LUME_INFO,LUME_PERF_STATUS,0,0);
    if(app_equal(argv[1],"video")){if(!video())return 1;}
    else if(app_equal(argv[1],"uart")){if(!uart(8))return 1;}
    else if(app_equal(argv[1],"compute"))compute();
    else if(app_equal(argv[1],"mixed")){
        const char *v[]={"iobench","video"},*c[]={"iobench","compute"};
        int vp=app_spawn("/usr/bin/iobench" LUME_APP_SUFFIX "",2,v),cp=app_spawn("/usr/bin/iobench" LUME_APP_SUFFIX "",2,c);
        if(vp<0||cp<0)return 1;if(!uart(8))return 1;
        struct lume_wait_status status;
        if(app_wait(vp,&status)<0||status.code||app_wait(cp,&status)<0||status.code)return 1;
    }else return 2;
    app_puts("\nIOBENCH_RESULT pid=");app_number((unsigned)lume_syscall(LUME_PID,0,0,0));
    app_puts(" perf_before=");app_number(perf);app_puts(" perf_after=");app_number((unsigned)lume_syscall(LUME_INFO,LUME_PERF_STATUS,0,0));app_puts("\n");
    report();return 0;
}
