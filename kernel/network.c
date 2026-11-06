/*
 * StellaOS - network.c
 * Network stack stub and network interface implementation
 */

#include "types.h"
#include "network.h"
#include "string.h"
#include "serial.h"

static net_interface_t default_interface;
static bool network_initialized = false;

static void uint8_to_dec(uint8_t val, char *out, size_t *pos, size_t max_len) {
    char temp[4];
    int tpos = 0;
    if (val == 0) {
        temp[tpos++] = '0';
    } else {
        uint8_t v = val;
        while (v > 0) {
            temp[tpos++] = (char)('0' + (v % 10));
            v /= 10;
        }
    }
    for (int i = tpos - 1; i >= 0; i--) {
        if (*pos + 1 < max_len) {
            out[(*pos)++] = temp[i];
        }
    }
}

static void uint8_to_hex(uint8_t val, char *out, size_t *pos, size_t max_len) {
    const char hex_chars[] = "0123456789ABCDEF";
    if (*pos + 2 < max_len) {
        out[(*pos)++] = hex_chars[(val >> 4) & 0x0F];
        out[(*pos)++] = hex_chars[val & 0x0F];
    }
}

void network_init(void) {
    memset(&default_interface, 0, sizeof(default_interface));

    strncpy(default_interface.name, "eth0", sizeof(default_interface.name) - 1);

    /* Standard default virtual MAC (52:54:00:12:34:56) */
    default_interface.mac[0] = 0x52;
    default_interface.mac[1] = 0x54;
    default_interface.mac[2] = 0x00;
    default_interface.mac[3] = 0x12;
    default_interface.mac[4] = 0x34;
    default_interface.mac[5] = 0x56;

    /* Default IPv4 configuration */
    default_interface.ip[0] = 192;
    default_interface.ip[1] = 168;
    default_interface.ip[2] = 1;
    default_interface.ip[3] = 100;

    default_interface.netmask[0] = 255;
    default_interface.netmask[1] = 255;
    default_interface.netmask[2] = 255;
    default_interface.netmask[3] = 0;

    default_interface.gateway[0] = 192;
    default_interface.gateway[1] = 168;
    default_interface.gateway[2] = 1;
    default_interface.gateway[3] = 1;

    default_interface.status = NET_STATUS_UP;
    network_initialized = true;

    char ip_str[32];
    char mac_str[32];
    network_get_ip_string(ip_str, sizeof(ip_str));
    network_get_mac_string(mac_str, sizeof(mac_str));

    serial_printf("Network: Initialized stub interface '%s' (MAC: %s, IP: %s)\n",
                  default_interface.name, mac_str, ip_str);
}

bool network_is_available(void) {
    return (network_initialized && default_interface.status == NET_STATUS_UP);
}

void network_get_ip_string(char *buf, size_t len) {
    if (!buf || len == 0) return;

    size_t pos = 0;
    for (int i = 0; i < 4; i++) {
        uint8_to_dec(default_interface.ip[i], buf, &pos, len);
        if (i < 3 && pos + 1 < len) {
            buf[pos++] = '.';
        }
    }
    buf[pos] = '\0';
}

void network_get_mac_string(char *buf, size_t len) {
    if (!buf || len == 0) return;

    size_t pos = 0;
    for (int i = 0; i < 6; i++) {
        uint8_to_hex(default_interface.mac[i], buf, &pos, len);
        if (i < 5 && pos + 1 < len) {
            buf[pos++] = ':';
        }
    }
    buf[pos] = '\0';
}

net_interface_t *network_get_interface(void) {
    return &default_interface;
}
