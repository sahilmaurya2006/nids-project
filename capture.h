#ifndef CAPTURE_H
#define CAPTURE_H

#include <pcap.h>
#include <stdint.h>

#define MAX_DEVICES 32

typedef struct {
    char name[256];
    char description[1024];
} NetworkDevice;

typedef struct {
    uint8_t dst_mac[6];
    uint8_t src_mac[6];
    uint16_t ethertype;
} EthernetHeader;

typedef struct {
    uint8_t version_ihl;
    uint8_t tos;
    uint16_t length;
    uint16_t id;
    uint16_t flags_offset;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint32_t src_ip;
    uint32_t dst_ip;
} IPv4Header;

typedef struct {
    uint16_t src_port;
    uint16_t dst_port;
    uint32_t seq_num;
    uint32_t ack_num;
    uint8_t offset;
    uint8_t flags;
    uint16_t window;
    uint16_t checksum;
    uint16_t urgent;
} TCPHeader;

typedef struct {
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t length;
    uint16_t checksum;
} UDPHeader;

typedef struct {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t id;
    uint16_t seq;
} ICMPHeader;

typedef struct {
    uint64_t timestamp;
    uint8_t protocol;
    uint32_t src_ip;
    uint32_t dst_ip;
    uint16_t src_port;
    uint16_t dst_port;
    uint8_t tcp_flags;
    uint32_t packet_length;
} ParsedPacket;

int list_devices(NetworkDevice *devices, int *count);
pcap_t *open_interface(const char *interface_name, char *errbuf);
void packet_handler(u_char *user_data, const struct pcap_pkthdr *h, const u_char *bytes);

#endif
