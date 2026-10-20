#ifndef LUME_NET_POLICY_H
#define LUME_NET_POLICY_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define NET_PROFILES 4u
#define NET_STORE_BYTES 544u
struct net_profile {char name[16];uint32_t priority;uint8_t ssid_bytes,key_bytes;uint8_t ssid[32],key[64];};
struct net_profiles {uint64_t generation;uint32_t automatic,count;struct net_profile profile[NET_PROFILES];};
bool net_name(const char *);
bool net_profiles_valid(const struct net_profiles *);
bool net_decode(struct net_profiles *,const uint8_t *,size_t);
bool net_encode(const struct net_profiles *,uint8_t [NET_STORE_BYTES]);
bool net_legacy(struct net_profile *,const uint8_t *,size_t,const char *);
int net_preferred(const struct net_profiles *,unsigned);
uint64_t net_retry(uint64_t,unsigned);
void net_wipe(void *,size_t);
#endif
