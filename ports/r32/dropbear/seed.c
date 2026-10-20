/* Persistent secret rotation: no timing source is credited with entropy. */
#include "api.h"
#include "crypto_client.h"
#include "random.h"
#include <string.h>
void lume_random_checkpoint(void){(void)lume_syscall(LUME_YIELD,0,0,0);}
int lume_rotate_seed(void){
    unsigned char old[72]={0},next[72]={0},derived[96]={0},check[72]={0},digest[32]={0};
    struct lume_random_state private={0};int fd=-1,result=-5;
    fd=app_open("/root/ssh/seed.bin",LUME_RDWR|LUME_DEFER_SYNC);if(fd<0){result=fd;goto done;}
    struct lume_stat st;if(lume_syscall(LUME_FSTAT,(uint32_t)fd,(uint32_t)(uintptr_t)&st,0)<0||st.size!=72)goto done;
    if(app_read(fd,old,sizeof old)!=(int)sizeof old||memcmp(old,"LSR1",4))goto done;
    lume_sha256(old,56,digest);if(memcmp(digest,old+56,16))goto done;
    uint32_t generation;memcpy(&generation,old+4,4);if(!generation||generation==UINT32_MAX)goto done;
    unsigned any=0;for(unsigned i=8;i<56;i++)any|=old[i];if(!any)goto done;
    if(lume_random_seed(&private,old+8,48)||lume_random_bytes(&private,derived,sizeof derived)!=(int)sizeof derived)goto done;
    memcpy(next,"LSR1",4);generation++;memcpy(next+4,&generation,4);memcpy(next+8,derived,48);
    lume_sha256(next,56,digest);memcpy(next+56,digest,16);
    if(app_seek(fd,0,LUME_SEEK_SET)<0)goto done;
    unsigned offset=0;
    while(offset<sizeof next){int n=app_write(fd,next+offset,sizeof next-offset);if(n<=0||(unsigned)n>sizeof next-offset)goto done;offset+=(unsigned)n;}
    if(app_sync(fd)<0||app_seek(fd,0,LUME_SEEK_SET)<0||app_read(fd,check,sizeof check)!=(int)sizeof check||memcmp(check,next,sizeof next))goto done;
    if(app_close(fd)<0){fd=-1;goto done;}fd=-1;
    /* No output is released until the next seed is durable and read back. */
    result=app_crypto(LUME_CRYPTO_SEED,derived+48,48);
    if(!result){app_puts("SSH_SEED_ROTATED generation=");app_number(generation);app_puts("\n");}
done:
    if(fd>=0)(void)app_close(fd);
    lume_secret_clear(&private,sizeof private);lume_secret_clear(old,sizeof old);
    lume_secret_clear(next,sizeof next);lume_secret_clear(derived,sizeof derived);
    lume_secret_clear(check,sizeof check);lume_secret_clear(digest,sizeof digest);return result;
}
