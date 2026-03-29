#ifndef DETECTION_H
#define DETECTION_H

#include "capture.h"
#include <time.h>

#define MAX_TRACKED_IPS 10000
#define DEFAULT_PORT_SCAN_THRESHOLD 10
#define DEFAULT_SYN_FLOOD_THRESHOLD 50
#define DEFAULT_ICMP_FLOOD_THRESHOLD 100
#define SLIDING_WINDOW_SECONDS 10

typedef enum {
    ALERT_PORT_SCAN,
    ALERT_SYN_FLOOD,
    ALERT_ICMP_FLOOD
} AlertType;

typedef struct {
    AlertType type;
    uint32_t src_ip;
    uint32_t dst_ip;
    time_t timestamp;
    char description[256];
} Alert;

typedef struct {
    uint32_t ip;
    time_t last_activity;
    int port_count;
    int syn_count;
    int icmp_count;
    uint16_t ports[MAX_TRACKED_IPS];
} IPTracker;

typedef struct {
    IPTracker trackers[MAX_TRACKED_IPS];
    int count;
    int port_scan_threshold;
    int syn_flood_threshold;
    int icmp_flood_threshold;
} DetectionEngine;

DetectionEngine *detection_engine_create(int port_threshold, int syn_threshold, int icmp_threshold);
void detection_engine_destroy(DetectionEngine *engine);
int detection_process_packet(DetectionEngine *engine, ParsedPacket *packet, Alert *alert);
void detection_reset_stale_entries(DetectionEngine *engine);

#endif
