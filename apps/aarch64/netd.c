/* Native EL0 network manager. Drivers/stack expose mechanisms; all persistent
 * selection, autoconnect and retry policy is here, outside the kernel. */
#include "lib.h"
#include "lume/service.h"
#include "net_policy.h"
static struct net_profiles profiles;
static struct lume_net_service_status status;
static struct lume_net_control request;
static int store_slot=-1,chosen=-1;
static unsigned rotation;
static bool paused,force;
static const char *const stores[]={"/data/system/net0.cfg","/data/system/net1.cfg"};
static int load_file(const char *path,uint8_t *b,unsigned size){
    int fd=(int)open(path,0);if(fd<0)return fd;struct lume_stat st;
    int result=-LUME_EIO;
    if(!call(LUME_SYS_STAT,(unsigned)fd,(uintptr_t)&st,0)&&st.size==size){unsigned done=0;while(done<size){int64_t n=read(fd,b+done,size-done);if(n<=0)break;done+=(unsigned)n;}if(done==size)result=0;}
    (void)close(fd);return result;
}
static bool store(void){
    if(profiles.generation==UINT64_MAX)return false;
    uint8_t b[NET_STORE_BYTES],check[NET_STORE_BYTES];++profiles.generation;
    bool ok=net_encode(&profiles,b);int next=store_slot==0?1:0,fd=-1;
    if(ok){fd=(int)open(stores[next],LUME_O_WRONLY|LUME_O_CREAT|LUME_O_TRUNC);ok=fd>=0;}
    unsigned done=0;while(ok&&done<sizeof(b)){int64_t n=write(fd,b+done,sizeof(b)-done);if(n<=0)ok=false;else done+=(unsigned)n;}
    if(fd>=0 && close(fd)<0)ok=false;
    if(ok){ok=load_file(stores[next],check,sizeof(check))==0;for(unsigned i=0;ok&&i<sizeof(b);++i)if(b[i]!=check[i])ok=false;}
    if(ok)store_slot=next;else --profiles.generation;
    net_wipe(b,sizeof(b));net_wipe(check,sizeof(check));return ok;
}
static bool import(struct net_profile *p,const char *name){uint8_t legacy[104];bool ok=load_file("/data/system/wifi.cfg",legacy,sizeof(legacy))==0&&net_legacy(p,legacy,sizeof(legacy),name);net_wipe(legacy,sizeof(legacy));return ok;}
static bool load(void){
    struct net_profiles a={0},b={0};uint8_t raw[NET_STORE_BYTES];
    int ra=load_file(stores[0],raw,sizeof(raw));bool va=!ra&&net_decode(&a,raw,sizeof(raw));
    int rb=load_file(stores[1],raw,sizeof(raw));bool vb=!rb&&net_decode(&b,raw,sizeof(raw));net_wipe(raw,sizeof(raw));
    bool ok=va||vb;
    if(ok){store_slot=vb&&(!va||b.generation>a.generation)?1:0;profiles=store_slot?b:a;}
    else if(ra==-LUME_ENOENT&&rb==-LUME_ENOENT){
        profiles=(struct net_profiles){.generation=1,.automatic=1};
        if(import(&profiles.profile[0],"current")){profiles.count=1;ok=store();}
    }
    net_wipe(&a,sizeof(a));net_wipe(&b,sizeof(b));return ok;
}
static int control(unsigned op){return (int)call(LUME_SYS_NETCTL,op,(uintptr_t)&request,sizeof(request));}
static void init(uint32_t index){net_wipe(&request,sizeof(request));request.version=1;request.ifindex=index;}
static void online(void){
    char out[64]="net: online (WPA2 + DHCP)\nDHCP ";unsigned used=(unsigned)len(out);
    for(unsigned i=0;i<4;++i){unsigned n=request.data.interfaces[0].address[i];if(i)out[used++]='.';if(n>=100)out[used++]=(char)('0'+n/100);if(n>=10)out[used++]=(char)('0'+n/10%10);out[used++]=(char)('0'+n%10);}
    out[used++]='\n';(void)write(1,out,used);
}
static void disconnect(void){if(status.ifindex){init(status.ifindex);(void)control(LUME_NET_WIFI_DISCONNECT);}chosen=-1;}
static void publish(void){
    status.version=1;status.pid=3;status.generation=profiles.generation;status.automatic=profiles.automatic;status.profiles=profiles.count;
    net_wipe(status.active,sizeof(status.active));net_wipe(status.profile,sizeof(status.profile));
    if(chosen>=0)for(unsigned j=0;j<16;++j)status.active[j]=profiles.profile[chosen].name[j];
    for(unsigned i=0;i<profiles.count;++i){for(unsigned j=0;j<16;++j)status.profile[i].name[j]=profiles.profile[i].name[j];status.profile[i].priority=profiles.profile[i].priority;}
    (void)call(LUME_SYS_SERVICE,LUME_SERVICE_PUBLISH,(uintptr_t)&status,sizeof(status));
}
static int find(const char *name){for(unsigned i=0;i<profiles.count;++i)if(eq(name,profiles.profile[i].name))return (int)i;return -1;}
static void command(const struct lume_service_message *m){
    int result=0;bool change=false;struct net_profiles before=profiles;
    if(m->version!=1||m->command<NETD_IMPORT||m->command>NETD_RELOAD)result=-LUME_EINVAL;
    for(unsigned i=0;i<sizeof(m->reserved);++i)if(m->reserved[i])result=-LUME_EINVAL;
    if(!result) switch(m->command){
    case NETD_IMPORT: {
        if(!net_name(m->name)){result=-LUME_EINVAL;break;}
        int n=find(m->name);if(n<0){if(profiles.count==4){result=-LUME_ENOSPC;break;}n=(int)profiles.count++;}
        if(!import(&profiles.profile[n],m->name)){result=-LUME_EIO;break;}change=true;break;
    }
    case NETD_FORGET: {
        int n=net_name(m->name)?find(m->name):-1;if(n<0){result=-LUME_ENOENT;break;}
        disconnect();for(unsigned i=(unsigned)n;i+1<profiles.count;++i)profiles.profile[i]=profiles.profile[i+1];
        net_wipe(&profiles.profile[--profiles.count],sizeof(profiles.profile[0]));change=true;break;
    }
    case NETD_PREFER: {
        int n=net_name(m->name)?find(m->name):-1;if(n<0){result=-LUME_ENOENT;break;}
        for(unsigned i=0;i<profiles.count;++i)profiles.profile[i].priority=i==(unsigned)n?255:128;change=true;break;
    }
    case NETD_AUTO_ON:profiles.automatic=1;paused=false;change=true;break;
    case NETD_AUTO_OFF:profiles.automatic=0;paused=true;disconnect();change=true;break;
    case NETD_CONNECT: {
        int n=net_name(m->name)?find(m->name):-1;if(n<0){result=-LUME_ENOENT;break;}
        disconnect();chosen=n;paused=false;force=true;status.retry_at=0;status.attempts=0;status.state=NETD_WAITING;break;
    }
    case NETD_PAUSE:paused=true;force=false;disconnect();break;
    case NETD_RECONNECT:paused=false;force=true;disconnect();status.retry_at=0;rotation=0;status.attempts=0;status.state=NETD_WAITING;break;
    case NETD_RELOAD:disconnect();if(!load())result=-LUME_EIO;else{paused=false;force=false;status.retry_at=0;status.state=NETD_WAITING;}break;
    }
    if(change&&!result&&!store())result=-LUME_EIO;
    if(result)profiles=before;else if(change){rotation=0;status.retry_at=0;}
    net_wipe(&before,sizeof(before));status.acknowledged=m->sequence;status.result=result;
}
static void step(uint64_t now){
    if(!status.ifindex){init(0);if(!control(LUME_NET_ENUM))for(unsigned i=0;i<request.count;++i)if(request.data.interfaces[i].kind==LUME_IF_WIFI){status.ifindex=request.data.interfaces[i].ifindex;break;}}
    if(!status.ifindex){status.state=NETD_WAITING;return;}
    init(status.ifindex);if(control(LUME_NET_GET)){status.state=NETD_ERROR;return;}
    struct lume_netif_info info=request.data.interfaces[0];
    if(paused||(!profiles.automatic&&!force)||!(info.flags&LUME_IF_UP)){status.state=NETD_OFF;return;}
    if((info.flags&LUME_IF_LINK)&&(info.flags&LUME_IF_DHCP)&&info.address[0]){status.state=NETD_ONLINE;status.attempts=0;status.retry_at=0;return;}
    if(status.state==NETD_ONLINE){puts("net: link lost; reconnect scheduled\n");chosen=-1;status.retry_at=now+1000;rotation=0;}
    if(now<status.retry_at){status.state=NETD_WAITING;return;}
    if(chosen<0)chosen=net_preferred(&profiles,rotation);
    if(chosen<0){status.state=NETD_OFF;return;}
    status.state=NETD_CONNECTING;++status.attempts;publish();puts("net: connecting\n");
    init(status.ifindex);const struct net_profile *p=&profiles.profile[chosen];request.data.connect.ssid_bytes=p->ssid_bytes;request.data.connect.key_bytes=p->key_bytes;
    for(unsigned i=0;i<p->ssid_bytes;++i)request.data.connect.ssid[i]=p->ssid[i];for(unsigned i=0;i<p->key_bytes;++i)request.data.connect.key[i]=p->key[i];
    int result=control(LUME_NET_WIFI_CONNECT);net_wipe(&request,sizeof(request));
    if(!result){init(status.ifindex);result=control(LUME_NET_DHCP_START);}
    if(!result){status.state=NETD_ONLINE;status.retry_at=0;status.attempts=0;online();}
    else {disconnect();++rotation;status.retry_at=net_retry((uint64_t)call(LUME_SYS_CLOCK,0,0,0),status.attempts-1);status.state=NETD_WAITING;puts("net: connection failed; retry pending\n");}
}
int main(int argc,char **argv){
    /* An independent client can drop the link without changing manager intent.
     * Useful for exercising recovery and for low-level interface diagnostics. */
    if(argc==2&&eq(argv[1],"drop")){init(0);if(control(LUME_NET_ENUM))return 1;uint32_t index=0;for(unsigned i=0;i<request.count;++i)if(request.data.interfaces[i].kind==LUME_IF_WIFI)index=request.data.interfaces[i].ifindex;init(index);return control(LUME_NET_WIFI_DISCONNECT)?1:0;}
    if(call(LUME_SYS_GETPID,0,0,0)!=3)return 64;
    puts("LUME NET USERSPACE START PID3\n");
    bool loaded=load();if(!loaded){paused=true;status.state=NETD_ERROR;puts("net: profiles unavailable; automatic connection disabled\n");}
    else puts("net: persistent profiles loaded\n");
    for(;;){
        struct lume_service_message m={0};
        if(!call(LUME_SYS_SERVICE,LUME_SERVICE_RECEIVE,(uintptr_t)&m,sizeof(m)))command(&m);
        if(loaded||profiles.generation)step((uint64_t)call(LUME_SYS_CLOCK,0,0,0));publish();
        (void)call(LUME_SYS_SLEEP,250,0,0);
    }
}
