/* LNET1 stream protocol. Integer fields are little endian, never C structs.
 * Request32: magic LNET, version u8=1, op u8(put1/get2), reserved u16=0,
 * path length u32, file length u32, CRC32 u32, reserved12 zero, then path.
 * Reply24: LACK, version u32=1, status i32(100 ready/0 done/-errno),
 * length u32, CRC32 u32, reserved u32=0. PUT: READY, body, optional
 * VERIFY101 replies(length=bytes verified), DONE. Verification progress is
 * sent at most5s apart, so a large readback does not look like a dead peer.
 * GET: READY(length), body, DONE(CRC). No shell commands or execution.
 */
#ifndef LUME_NET_PROTOCOL_H
#define LUME_NET_PROTOCOL_H
#include <stdint.h>
#define NET_PORT 8022u
#define NET_PATH_MAX 127u
#define NET_FILE_MAX (128u*1024u*1024u)
static inline uint32_t net_get32(const unsigned char *p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static inline void net_put32(unsigned char *p,uint32_t n){p[0]=(unsigned char)n;p[1]=(unsigned char)(n>>8);p[2]=(unsigned char)(n>>16);p[3]=(unsigned char)(n>>24);}
static inline uint32_t net_crc(uint32_t crc,const unsigned char *p,unsigned n){
    static const uint32_t table[16]={0,0x1db71064,0x3b6e20c8,0x26d930ac,0x76dc4190,0x6b6b51f4,0x4db26158,0x5005713c,0xedb88320,0xf00f9344,0xd6d6a3e8,0xcb61b38c,0x9b64c2b0,0x86d3d2d4,0xa00ae278,0xbdbdf21c};
    while(n--){crc^=*p++;crc=(crc>>4)^table[crc&15];crc=(crc>>4)^table[crc&15];}return crc;
}
static inline int net_valid_path(const char *p){
    if(*p!='/'||!p[1])return 0;
    unsigned part=0;for(unsigned i=1;p[i];i++){
        unsigned char c=(unsigned char)p[i];if(c<'!'||c>126||c=='\\'||c==':')return 0;
        if(c=='/'){if(!part)return 0;part=0;}else part++;
    }
    /* Reject dot components as well as traversal; allow ordinary .x32 suffixes. */
    for(unsigned i=0;p[i];i++)if(p[i]=='/'&&p[i+1]=='.'&&(p[i+2]=='/'||!p[i+2]||p[i+2]=='.'))return 0;
    return part!=0;
}
#endif
