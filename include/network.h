/*
 * StellaOS - network.h
 * Network stack interface and interface definitions
 */

#ifndef STELLA_NETWORK_H
#define STELLA_NETWORK_H

#include "types.h"

/* Network interface status */
#define NET_STATUS_DOWN 0
#define NET_STATUS_UP   1

/* Network interface structure */
typedef struct net_interface {
    char     name[16];
    uint8_t  mac[6];
    uint8_t  ip[4];
    uint8_t  netmask[4];
    uint8_t  gateway[4];
    uint32_t status;
} __attribute__((packed)) net_interface_t;

/* Network Public API */
void network_init(void);
bool network_is_available(void);
void network_get_ip_string(char *buf, size_t len);
void network_get_mac_string(char *buf, size_t len);
net_interface_t *network_get_interface(void);

#endif /* STELLA_NETWORK_H */
