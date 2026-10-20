/* Bounded DHCPv4 acquisition with transport callbacks.
 * Transport callbacks isolate protocol parsing from the network device.
 * No background lease renewal, DNS queries or general network services. */
#ifndef LUME_DHCP_H
#define LUME_DHCP_H
#include <stdint.h>
#define LUME_DHCP_PACKET 576u
enum lume_dhcp_status { LUME_DHCP_IDLE=0,LUME_DHCP_BOUND=1,
    LUME_DHCP_TIMEOUT=2,LUME_DHCP_IO=3,LUME_DHCP_NAK=4 };
enum lume_dhcp_event { LUME_DHCP_DISCOVER=1,LUME_DHCP_OFFER=2,
    LUME_DHCP_REQUEST=3,LUME_DHCP_ACK=4 };
struct lume_dhcp_lease {
    unsigned char ip[4],mask[4],gateway[4],dns[4],server[4];
    uint32_t status,seconds,xid,ignored;
};
struct lume_dhcp_io {
    void *context;
    int (*send)(void *,const unsigned char *,unsigned,const unsigned char *,unsigned);
    int (*receive)(void *,unsigned char *,unsigned *,unsigned char *,unsigned);
    uint32_t (*milliseconds)(void *);
    int (*idle)(void *);
    void (*event)(void *,unsigned);
};
void lume_dhcp_acquire(const struct lume_dhcp_io *io,const unsigned char mac[6],uint32_t xid,struct lume_dhcp_lease *lease);
/* DHCPRELEASE is unicast. Caller first applies the acquired IP/mask/gateway,
 * then releases and clears the temporary socket/config even on send failure. */
int lume_dhcp_release(const struct lume_dhcp_io *io,const unsigned char mac[6],const struct lume_dhcp_lease *lease);
#endif
