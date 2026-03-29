#include "parser.h"
#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <time.h>

#define ETHERNET_HEADER_LEN 14
#define IPV4_HEADER_MIN_LEN 20
#define TCP_HEADER_MIN_LEN 20
#define UDP_HEADER_LEN 8
#define ICMP_HEADER_MIN_LEN 8

#define IPPROTO_ICMP 1
#define IPPROTO_TCP 6
#define IPPROTO_UDP 17

/* TCP Flags */
#define TCP_FIN 0x01
#define TCP_SYN 0x02
#define TCP_RST 0x04
#define TCP_PSH 0x08
#define TCP_ACK 0x10
#define TCP_URG 0x20

int parse_packet(const u_char *packet, int length, ParsedPacket *parsed) {
    EthernetHeader *eth = NULL;
    IPv4Header *ipv4 = NULL;
    int eth_offset = 0;
    int ip_offset = 0;
    int transport_offset = 0;
    
    if (length < ETHERNET_HEADER_LEN) {
        return -1;
    }

    /* Parse Ethernet Header */
    eth = (EthernetHeader *)(packet + eth_offset);
    uint16_t ethertype = ntohs(eth->ethertype);
    
    /* Only handle IPv4 */
    if (ethertype != 0x0800) {
        return -1;
    }

    ip_offset = ETHERNET_HEADER_LEN;
    
    if (length < ip_offset + IPV4_HEADER_MIN_LEN) {
        return -1;
    }

    /* Parse IPv4 Header */
    ipv4 = (IPv4Header *)(packet + ip_offset);
    
    uint8_t version = (ipv4->version_ihl >> 4) & 0x0F;
    uint8_t ihl = (ipv4->version_ihl & 0x0F) * 4;
    
    if (version != 4) {
        return -1;
    }
    
    if (ihl < IPV4_HEADER_MIN_LEN || length < ip_offset + ihl) {
        return -1;
    }

    parsed->src_ip = ipv4->src_ip;
    parsed->dst_ip = ipv4->dst_ip;
    parsed->protocol = ipv4->protocol;
    parsed->packet_length = ntohs(ipv4->length);
    parsed->timestamp = time(NULL);
    parsed->src_port = 0;
    parsed->dst_port = 0;
    parsed->tcp_flags = 0;
    
    transport_offset = ip_offset + ihl;

    /* Parse Transport Headers */
    switch (ipv4->protocol) {
        case IPPROTO_TCP:
            if (length < transport_offset + TCP_HEADER_MIN_LEN) {
                return -1;
            }
            {
                TCPHeader *tcp = (TCPHeader *)(packet + transport_offset);
                parsed->src_port = ntohs(tcp->src_port);
                parsed->dst_port = ntohs(tcp->dst_port);
                parsed->tcp_flags = tcp->flags;
            }
            break;

        case IPPROTO_UDP:
            if (length < transport_offset + UDP_HEADER_LEN) {
                return -1;
            }
            {
                UDPHeader *udp = (UDPHeader *)(packet + transport_offset);
                parsed->src_port = ntohs(udp->src_port);
                parsed->dst_port = ntohs(udp->dst_port);
            }
            break;

        case IPPROTO_ICMP:
            if (length < transport_offset + ICMP_HEADER_MIN_LEN) {
                return -1;
            }
            {
                ICMPHeader *icmp = (ICMPHeader *)(packet + transport_offset);
                parsed->src_port = ntohs(icmp->id);
                parsed->dst_port = icmp->type;
            }
            break;

        default:
            /* Unsupported protocol */
            return -1;
    }

    return 0;
}

const char *protocol_to_string(uint8_t protocol) {
    switch (protocol) {
        case IPPROTO_ICMP:
            return "ICMP";
        case IPPROTO_TCP:
            return "TCP";
        case IPPROTO_UDP:
            return "UDP";
        default:
            return "UNKNOWN";
    }
}

const char *tcp_flags_to_string(uint8_t flags) {
    static char buffer[256];
    buffer[0] = '\0';

    if (flags & TCP_SYN) strcat(buffer, "SYN ");
    if (flags & TCP_ACK) strcat(buffer, "ACK ");
    if (flags & TCP_FIN) strcat(buffer, "FIN ");
    if (flags & TCP_RST) strcat(buffer, "RST ");
    if (flags & TCP_PSH) strcat(buffer, "PSH ");
    if (flags & TCP_URG) strcat(buffer, "URG ");

    if (buffer[0] == '\0') {
        strcpy(buffer, "NONE");
    } else {
        buffer[strlen(buffer) - 1] = '\0'; /* Remove trailing space */
    }

    return buffer;
}
