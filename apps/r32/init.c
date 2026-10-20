#include "api.h"
#include "crypto_client.h"
static int ssh_available(void)
{
    struct lume_crypto_info crypto;
    unsigned required=LUME_CRYPTO_AES_GCM|LUME_CRYPTO_X25519_HW|LUME_CRYPTO_ED25519_HW;
    if(app_crypto(LUME_CRYPTO_INFO,&crypto,sizeof crypto)<0||
       (crypto.accelerated&required)!=required)return 0;
    static const char *paths[]={"/usr/bin/dropbear" LUME_APP_SUFFIX "","/usr/bin/sshshell" LUME_APP_SUFFIX "",
        "/root/ssh/hostkey","/root/ssh/authkeys","/root/ssh/seed.bin"};
    struct lume_stat st;
    for(unsigned i=0;i<sizeof paths/sizeof paths[0];i++)
        if(app_stat(paths[i],&st)<0||!st.size)return 0;
    return 1;
}
static int ssh_start(void)
{
    const char *args[]={"/usr/bin/dropbear" LUME_APP_SUFFIX "","-F"};
    return app_spawn(args[0],2,args);
}
int app_main(void)
{
    app_puts("LumeOS init PID=");
    app_number((uint32_t)lume_syscall(LUME_PID, 0, 0, 0));
    app_puts(" UID=0\n");
    const char *shell = "/bin/sh" LUME_APP_SUFFIX "";
    int32_t shell_pid[2]={-1,-1};
    int32_t network_pid=-1;
    int32_t ssh_pid=-1;
    uint32_t ssh_started=0,ssh_delay=1000;
    shell_pid[0]=lume_syscall(LUME_SPAWN,(uint32_t)(uintptr_t)shell,0,0);
    if (shell_pid[0] < 0)
    {
        app_puts("init: shell load failed\n");
        return 1;
    }
    /* Selector2 is additive; old kernels return EINVAL and keep one shell. */
    if(lume_syscall(LUME_TERMINAL,2,1,0)==0){
        shell_pid[1]=lume_syscall(LUME_SPAWN,(uint32_t)(uintptr_t)shell,0,0);
        lume_syscall(LUME_TERMINAL,2,0,0);
        app_puts(shell_pid[1]<0?"init: terminal2 shell load failed\n":"Alt+F1: terminal1 | Alt+F2: terminal2\n");
    }
    /* Optional service: older TF cards without netd still start both shells. */
    struct lume_stat network_stat;
    if(app_stat("/usr/bin/netd" LUME_APP_SUFFIX "",&network_stat)==0)
        network_pid=app_spawn("/usr/bin/netd" LUME_APP_SUFFIX "",0,0);
    /* Dropbear handles one connection per process, without fork. Optional
     * provisioning keeps older cards bootable. Each restart rotates the seed
     * durably before listening; init supplies supervision without another slot. */
    if(ssh_available()){
        ssh_pid=ssh_start();
        ssh_started=(uint32_t)lume_syscall(LUME_INFO,LUME_UPTIME,0,0);
    }
    for (;;)
    {
        struct lume_wait_status status;
        int32_t exited=app_wait(-1,&status);
        if(network_pid>=0&&exited==network_pid)
            network_pid=app_spawn("/usr/bin/netd" LUME_APP_SUFFIX "",0,0);
        for(unsigned tty=0;tty<2;tty++)if(exited==shell_pid[tty]){
            lume_syscall(LUME_TERMINAL,2,tty,0);
            shell_pid[tty]=lume_syscall(LUME_SPAWN,(uint32_t)(uintptr_t)shell,0,0);
            lume_syscall(LUME_TERMINAL,2,0,0);
        }
        if(ssh_pid>=0&&exited==ssh_pid){
            uint32_t now=(uint32_t)lume_syscall(LUME_INFO,LUME_UPTIME,0,0);
            if(status.code&&now-ssh_started<2000){
                lume_syscall(LUME_SLEEP,ssh_delay,0,0);
                if(ssh_delay<30000)ssh_delay=ssh_delay>15000?30000:ssh_delay*2;
            }else ssh_delay=1000;
            ssh_pid=ssh_available()?ssh_start():-1;
            ssh_started=(uint32_t)lume_syscall(LUME_INFO,LUME_UPTIME,0,0);
        }
        lume_syscall(LUME_YIELD, 0, 0, 0);
    }
}
