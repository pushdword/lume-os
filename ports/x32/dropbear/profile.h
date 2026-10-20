#ifndef LUME_SSH_PROFILE_H
#define LUME_SSH_PROFILE_H
enum {SSH_P_SHA256,SSH_P_SHA512,SSH_P_SCALAR,SSH_P_AES,SSH_P_CURVE,
    SSH_P_RNG,SSH_P_SELECT,SSH_P_SLEEP,SSH_P_NETIO,SSH_P_COUNT};
#ifdef LUME_SSH_PROFILE
void ssh_profile_reset(void);
void ssh_profile_print(void);
#ifdef LUME_SSH_PROFILE_PHASES
unsigned ssh_profile_now(void);
void ssh_profile_record(unsigned,unsigned);
#else
static inline unsigned ssh_profile_now(void){return 0;}
static inline void ssh_profile_record(unsigned kind,unsigned n){(void)kind;(void)n;}
#endif
#else
static inline unsigned ssh_profile_now(void){return 0;}
static inline void ssh_profile_record(unsigned kind,unsigned n){(void)kind;(void)n;}
static inline void ssh_profile_reset(void){}
static inline void ssh_profile_print(void){}
#endif
#endif
