#ifndef LUME_POSIX_NET_IF_H
#define LUME_POSIX_NET_IF_H
#define IFF_UP 1u
#define IFF_RUNNING 64u
#define IFNAMSIZ 16

struct if_nameindex
{
    unsigned if_index;
    char *if_name;
};
#ifdef __cplusplus
extern "C"
{
#endif
    struct if_nameindex *if_nameindex(void);
    void if_freenameindex(struct if_nameindex *);
#ifdef __cplusplus
}
#endif
#endif
