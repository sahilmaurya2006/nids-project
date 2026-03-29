#include "detection.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <winsock2.h>

#define TCP_SYN 0x02
#define TCP_ACK 0x10

static int find_tracker(DetectionEngine *engine, uint32_t ip) {
    for (int i = 0; i < engine->count; i++) {
        if (engine->trackers[i].ip == ip) {
            return i;
        }
    }
    return -1;
}

static int create_tracker(DetectionEngine *engine, uint32_t ip) {
    if (engine->count >= MAX_TRACKED_IPS) {
        /* Reuse oldest entry */
        int oldest = 0;
        time_t oldest_time = engine->trackers[0].last_activity;
        
        for (int i = 1; i < engine->count; i++) {
            if (engine->trackers[i].last_activity < oldest_time) {
                oldest_time = engine->trackers[i].last_activity;
                oldest = i;
            }
        }
        
        memset(&engine->trackers[oldest], 0, sizeof(IPTracker));
        engine->trackers[oldest].ip = ip;
        engine->trackers[oldest].last_activity = time(NULL);
        return oldest;
    }
    
    int idx = engine->count;
    memset(&engine->trackers[idx], 0, sizeof(IPTracker));
    engine->trackers[idx].ip = ip;
    engine->trackers[idx].last_activity = time(NULL);
    engine->count++;
    return idx;
}

static int is_port_already_tracked(IPTracker *tracker, uint16_t port) {
    for (int i = 0; i < tracker->port_count; i++) {
        if (tracker->ports[i] == port) {
            return 1;
        }
    }
    return 0;
}

DetectionEngine *detection_engine_create(int port_threshold, int syn_threshold, int icmp_threshold) {
    DetectionEngine *engine = (DetectionEngine *)malloc(sizeof(DetectionEngine));
    if (engine == NULL) {
        return NULL;
    }
    
    memset(engine, 0, sizeof(DetectionEngine));
    engine->port_scan_threshold = port_threshold > 0 ? port_threshold : DEFAULT_PORT_SCAN_THRESHOLD;
    engine->syn_flood_threshold = syn_threshold > 0 ? syn_threshold : DEFAULT_SYN_FLOOD_THRESHOLD;
    engine->icmp_flood_threshold = icmp_threshold > 0 ? icmp_threshold : DEFAULT_ICMP_FLOOD_THRESHOLD;
    
    return engine;
}

void detection_engine_destroy(DetectionEngine *engine) {
    if (engine) {
        free(engine);
    }
}

int detection_process_packet(DetectionEngine *engine, ParsedPacket *packet, Alert *alert) {
    if (engine == NULL || packet == NULL || alert == NULL) {
        return -1;
    }
    
    memset(alert, 0, sizeof(Alert));
    
    int idx = find_tracker(engine, packet->src_ip);
    if (idx == -1) {
        idx = create_tracker(engine, packet->src_ip);
    }
    
    IPTracker *tracker = &engine->trackers[idx];
    tracker->last_activity = time(NULL);
    
    /* Port Scan Detection */
    if (packet->protocol == 6) { /* TCP */
        /* Track unique destination ports */
        if (!is_port_already_tracked(tracker, packet->dst_port)) {
            if (tracker->port_count < MAX_TRACKED_IPS - 1) {
                tracker->ports[tracker->port_count] = packet->dst_port;
                tracker->port_count++;
            }
        }
        
        /* SYN Flood Detection */
        if ((packet->tcp_flags & TCP_SYN) && !(packet->tcp_flags & TCP_ACK)) {
            tracker->syn_count++;
        } else if (packet->tcp_flags & TCP_ACK) {
            /* Reset SYN count if we see ACK */
            if (tracker->syn_count > 0) {
                tracker->syn_count--;
            }
        }
        
        /* Check for port scan alert */
        if (tracker->port_count >= engine->port_scan_threshold) {
            alert->type = ALERT_PORT_SCAN;
            alert->src_ip = packet->src_ip;
            alert->dst_ip = packet->dst_ip;
            alert->timestamp = packet->timestamp;
            snprintf(alert->description, sizeof(alert->description),
                    "Port scan detected: %d unique ports scanned", tracker->port_count);
            
            /* Reset port tracking after alert */
            tracker->port_count = 0;
            memset(tracker->ports, 0, sizeof(tracker->ports));
            
            return 1; /* Alert generated */
        }
        
        /* Check for SYN flood alert */
        if (tracker->syn_count >= engine->syn_flood_threshold) {
            alert->type = ALERT_SYN_FLOOD;
            alert->src_ip = packet->src_ip;
            alert->dst_ip = packet->dst_ip;
            alert->timestamp = packet->timestamp;
            snprintf(alert->description, sizeof(alert->description),
                    "SYN flood detected: %d SYN packets without ACK", tracker->syn_count);
            
            /* Reset SYN count after alert */
            tracker->syn_count = 0;
            
            return 1; /* Alert generated */
        }
    }
    
    /* ICMP Flood Detection */
    if (packet->protocol == 1) { /* ICMP */
        tracker->icmp_count++;
        
        if (tracker->icmp_count >= engine->icmp_flood_threshold) {
            alert->type = ALERT_ICMP_FLOOD;
            alert->src_ip = packet->src_ip;
            alert->dst_ip = packet->dst_ip;
            alert->timestamp = packet->timestamp;
            snprintf(alert->description, sizeof(alert->description),
                    "ICMP flood detected: %d ICMP packets", tracker->icmp_count);
            
            /* Reset ICMP count after alert */
            tracker->icmp_count = 0;
            
            return 1; /* Alert generated */
        }
    }
    
    return 0; /* No alert */
}

void detection_reset_stale_entries(DetectionEngine *engine) {
    if (engine == NULL) {
        return;
    }
    
    time_t current_time = time(NULL);
    
    for (int i = 0; i < engine->count; i++) {
        if (current_time - engine->trackers[i].last_activity > SLIDING_WINDOW_SECONDS) {
            memset(&engine->trackers[i], 0, sizeof(IPTracker));
        }
    }
}
