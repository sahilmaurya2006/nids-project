#include "capture.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

int list_devices(NetworkDevice *devices, int *count) {
    pcap_if_t *alldevs, *d;
    char errbuf[PCAP_ERRBUF_SIZE];
    int i = 0;

    if (pcap_findalldevs(&alldevs, errbuf) == -1) {
        printf("Error in pcap_findalldevs: %s\n", errbuf);
        return -1;
    }

    if (alldevs == NULL) {
        printf("No devices found! Make sure Npcap/WinPcap is installed.\n");
        return -1;
    }

    for (d = alldevs; d; d = d->next) {
        if (i >= MAX_DEVICES) break;
        
        strncpy(devices[i].name, d->name, sizeof(devices[i].name) - 1);
        if (d->description) {
            strncpy(devices[i].description, d->description, sizeof(devices[i].description) - 1);
        } else {
            strncpy(devices[i].description, "No description available", sizeof(devices[i].description) - 1);
        }
        i++;
    }

    *count = i;
    pcap_freealldevs(alldevs);
    return 0;
}

pcap_t *open_interface(const char *interface_name, char *errbuf) {
    pcap_t *handle;
    int snaplen = 65535;
    int promisc = 1;
    int to_ms = 1000;

    handle = pcap_open_live(interface_name, snaplen, promisc, to_ms, errbuf);
    
    if (handle == NULL) {
        return NULL;
    }

    return handle;
}

void packet_handler(u_char *user_data, const struct pcap_pkthdr *h, const u_char *bytes) {
    /* This callback is used within pcap_loop, actual processing is done elsewhere */
    (void)user_data;
    (void)h;
    (void)bytes;
}
