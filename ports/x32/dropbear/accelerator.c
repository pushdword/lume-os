#include "api.h"
#include "crypto_client.h"
#include <stdint.h>
#include "profile.h"
static int qualified=-1;
static unsigned hardware_packets,curve_calls[3];
/* This board profile requires hardware. Missing capabilities and device errors
 * fail closed; there is no slow scalar/AES fallback for SSH. */
int ssh_aes_packet(unsigned decrypt,const unsigned char *key,unsigned key_bytes,
    const unsigned char nonce[12],const unsigned char *header,const unsigned char *input,
    unsigned char *output,unsigned bytes,unsigned char *tag){
 if(qualified<0){struct lume_crypto_info info;int r=app_crypto(LUME_CRYPTO_INFO,&info,sizeof info);qualified=r==0&&!!(info.accelerated&LUME_CRYPTO_AES_GCM);}
 if(!qualified||bytes>4096)return -95;
 struct lume_aead_request r={1,sizeof r,(uint32_t)(uintptr_t)key,key_bytes,
  (uint32_t)(uintptr_t)nonce,12,(uint32_t)(uintptr_t)header,4,
  (uint32_t)(uintptr_t)input,bytes,(uint32_t)(uintptr_t)output,(uint32_t)(uintptr_t)tag};
 unsigned started=ssh_profile_now();int result=app_crypto(decrypt?LUME_CRYPTO_AES_DECRYPT:LUME_CRYPTO_AES_ENCRYPT,&r,sizeof r);
 ssh_profile_record(SSH_P_AES,ssh_profile_now()-started);
 if(!result)hardware_packets++;
 else{app_puts("SSH_AES_ERROR decrypt=");app_number(decrypt);app_puts(" bytes=");app_number(bytes);app_puts(" ");app_error("syscall",result);}
 return result;
}
int ssh_curve_packet(unsigned operation,const unsigned char *a,const unsigned char *b,
    const unsigned char *point,unsigned char *output){
 if(operation<1||operation>3)return -22;
 struct lume_curve_request r={1,sizeof r,(uint32_t)(uintptr_t)a,(uint32_t)(uintptr_t)b,
  (uint32_t)(uintptr_t)point,(uint32_t)(uintptr_t)output,0};
 unsigned started=ssh_profile_now();int result=app_crypto(LUME_CRYPTO_X25519+operation-1,&r,sizeof r);
 ssh_profile_record(SSH_P_CURVE,ssh_profile_now()-started);
 if(!result)curve_calls[operation-1]++;
 return result;
}
int ssh_crypto_require_hardware(void){
 struct lume_crypto_info info;int result=app_crypto(LUME_CRYPTO_INFO,&info,sizeof info);
 unsigned needed=LUME_CRYPTO_AES_GCM|LUME_CRYPTO_X25519_HW|LUME_CRYPTO_ED25519_HW;
 return result<0?result:(info.accelerated&needed)==needed?0:-95;
}
void ssh_crypto_metrics(void){
 app_puts("SSH_AES hw=");app_number(hardware_packets);app_puts(" software=0\nSSH_CURVE x25519=");
 app_number(curve_calls[0]);app_puts(" base=");app_number(curve_calls[1]);
 app_puts(" verify=");app_number(curve_calls[2]);app_puts(" software=0\n");
 ssh_profile_print();
}
