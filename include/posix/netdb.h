#ifdef __cplusplus
extern "C"
{
#endif
#ifndef LUME_POSIX_NETDB_H
#define LUME_POSIX_NETDB_H
#include <sys/socket.h>

    struct hostent
    {
        char *h_name;
        char **h_aliases;
        int h_addrtype, h_length;
        char **h_addr_list;
    };

#define h_addr h_addr_list[0]
#define HOST_NOT_FOUND 1
#define TRY_AGAIN 2
#define NO_RECOVERY 3
#define NO_DATA 4
    int *__lume_h_errno(void);
#define h_errno (*__lume_h_errno())
    struct hostent *gethostbyname(const char *);
    struct hostent *gethostbyaddr(const void *, socklen_t, int);

    struct addrinfo
    {
        int ai_flags, ai_family, ai_socktype, ai_protocol;
        socklen_t ai_addrlen;
        struct sockaddr *ai_addr;
        char *ai_canonname;
        struct addrinfo *ai_next;
    };

#define AI_PASSIVE 1
#define AI_NUMERICHOST 4
#define AI_CANONNAME 2
#define NI_NUMERICHOST 1
#define NI_NUMERICSERV 2
#define NI_NAMEREQD 8
#define NI_MAXHOST 1025
#define NI_MAXSERV 32
#define EAI_NONAME -2
#define EAI_FAMILY -6
#define EAI_MEMORY -10
    int getaddrinfo(const char *, const char *, const struct addrinfo *, struct addrinfo **);
    void freeaddrinfo(struct addrinfo *);
    const char *gai_strerror(int);
    int getnameinfo(const struct sockaddr *, socklen_t, char *, socklen_t, char *, socklen_t, int);
#define AI_NUMERICSERV 1024
#define AI_ADDRCONFIG 32
#define EAI_AGAIN -3
#define EAI_SYSTEM -11
#define RRSET_VALIDATED 1
#define ERRSET_SUCCESS 0
#define ERRSET_NOMEMORY 1
#define ERRSET_FAIL 2
#define ERRSET_INVAL 3
#define ERRSET_NONAME 4
#define ERRSET_NODATA 5

    struct rdatainfo
    {
        unsigned rdi_length;
        unsigned char *rdi_data;
    };

    struct rrsetinfo
    {
        unsigned rri_flags, rri_rdclass, rri_rdtype, rri_ttl, rri_nrdatas, rri_nsigs;
        char *rri_name;
        struct rdatainfo *rri_rdatas, *rri_sigs;
    };

    int getrrsetbyname(const char *, unsigned, unsigned, unsigned, struct rrsetinfo **);
    void freerrset(struct rrsetinfo *);

    struct servent
    {
        char *s_name;
        char **s_aliases;
        int s_port;
        char *s_proto;
    };
    struct servent *getservbyname(const char *, const char *);
#endif
#ifdef __cplusplus
}
#endif
