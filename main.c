#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <winsock2.h>
#include <process.h>
#include "capture.h"
#include "parser.h"
#include "detection.h"
#include "logger.h"

#define PACKET_QUEUE_SIZE 1000

typedef struct {
    uint8_t data[65536];
    int length;
    time_t timestamp;
} QueuedPacket;

typedef struct {
    QueuedPacket packets[PACKET_QUEUE_SIZE];
    int head;
    int tail;
    int count;
    HANDLE mutex;
    HANDLE not_empty;
} PacketQueue;

typedef struct {
    pcap_t *handle;
    PacketQueue *queue;
    volatile int running;
} CaptureThreadArgs;

typedef struct {
    PacketQueue *queue;
    DetectionEngine *engine;
    Logger *logger;
    volatile int running;
} ProcessThreadArgs;

static PacketQueue *queue_create(void) {
    PacketQueue *q = (PacketQueue *)malloc(sizeof(PacketQueue));
    if (q == NULL) {
        return NULL;
    }
    
    memset(q, 0, sizeof(PacketQueue));
    q->mutex = CreateMutex(NULL, FALSE, NULL);
    q->not_empty = CreateEvent(NULL, FALSE, FALSE, NULL);
    
    if (q->mutex == NULL || q->not_empty == NULL) {
        if (q->mutex) CloseHandle(q->mutex);
        if (q->not_empty) CloseHandle(q->not_empty);
        free(q);
        return NULL;
    }
    
    return q;
}

static void queue_destroy(PacketQueue *q) {
    if (q) {
        if (q->mutex) CloseHandle(q->mutex);
        if (q->not_empty) CloseHandle(q->not_empty);
        free(q);
    }
}

static int queue_enqueue(PacketQueue *q, const uint8_t *data, int length) {
    if (WaitForSingleObject(q->mutex, INFINITE) != WAIT_OBJECT_0) {
        return -1;
    }
    
    if (q->count >= PACKET_QUEUE_SIZE) {
        ReleaseMutex(q->mutex);
        return -1; /* Queue full */
    }
    
    if (length > 65536) {
        ReleaseMutex(q->mutex);
        return -1; /* Packet too large */
    }
    
    QueuedPacket *pkt = &q->packets[q->tail];
    memcpy(pkt->data, data, length);
    pkt->length = length;
    pkt->timestamp = time(NULL);
    
    q->tail = (q->tail + 1) % PACKET_QUEUE_SIZE;
    q->count++;
    
    SetEvent(q->not_empty);
    ReleaseMutex(q->mutex);
    
    return 0;
}

static int queue_dequeue(PacketQueue *q, QueuedPacket *pkt) {
    if (WaitForSingleObject(q->mutex, 100) != WAIT_OBJECT_0) {
        return -1; /* Timeout or error */
    }
    
    if (q->count == 0) {
        ReleaseMutex(q->mutex);
        return -1; /* Queue empty */
    }
    
    memcpy(pkt, &q->packets[q->head], sizeof(QueuedPacket));
    q->head = (q->head + 1) % PACKET_QUEUE_SIZE;
    q->count--;
    
    ReleaseMutex(q->mutex);
    
    return 0;
}

static unsigned int __stdcall capture_thread(void *arg) {
    CaptureThreadArgs *args = (CaptureThreadArgs *)arg;
    int ret;
    char errbuf[PCAP_ERRBUF_SIZE];
    
    printf("[INFO] Capture thread started\n");
    
    if (pcap_datalink(args->handle) != DLT_EN10MB) {
        printf("[ERROR] Unsupported data link type\n");
        args->running = 0;
        return 1;
    }
    
    while (args->running) {
        struct pcap_pkthdr header;
        const u_char *packet_data = pcap_next(args->handle, &header);
        
        if (packet_data != NULL) {
            int result = queue_enqueue(args->queue, packet_data, header.len);
            if (result != 0) {
                /* Queue full, drop packet */
            }
        }
    }
    
    printf("[INFO] Capture thread stopped\n");
    return 0;
}

static unsigned int __stdcall process_thread(void *arg) {
    ProcessThreadArgs *args = (ProcessThreadArgs *)arg;
    QueuedPacket pkt;
    ParsedPacket parsed;
    Alert alert;
    
    printf("[INFO] Process thread started\n");
    
    while (args->running) {
        int ret = queue_dequeue(args->queue, &pkt);
        
        if (ret == 0) {
            /* Process packet */
            if (parse_packet(pkt.data, pkt.length, &parsed) == 0) {
                if (detection_process_packet(args->engine, &parsed, &alert) > 0) {
                    logger_log_alert(args->logger, &alert);
                }
            }
        } else {
            /* No packet, sleep briefly */
            Sleep(10);
        }
        
        /* Periodically reset stale entries */
        static unsigned int packet_count = 0;
        if (++packet_count % 100 == 0) {
            detection_reset_stale_entries(args->engine);
        }
    }
    
    printf("[INFO] Process thread stopped\n");
    return 0;
}

static void print_devices(NetworkDevice *devices, int count) {
    printf("\nAvailable network interfaces:\n");
    printf("===================================\n");
    for (int i = 0; i < count; i++) {
        printf("[%d] %s\n", i, devices[i].name);
        printf("    Description: %s\n", devices[i].description);
    }
    printf("===================================\n");
}

static void show_usage(const char *program) {
    printf("Usage: %s [OPTIONS]\n\n", program);
    printf("Options:\n");
    printf("  --interface <name>    Network interface name (e.g., \\Device\\NPF_{GUID})\n");
    printf("  --port-threshold <n>  Port scan threshold (default: 10)\n");
    printf("  --syn-threshold <n>   SYN flood threshold (default: 50)\n");
    printf("  --icmp-threshold <n>  ICMP flood threshold (default: 100)\n");
    printf("  --help                Show this help message\n\n");
    printf("If no interface is specified, available interfaces will be displayed.\n");
}

int main(int argc, char *argv[]) {
    NetworkDevice devices[MAX_DEVICES];
    int device_count = 0;
    char *interface = NULL;
    int port_threshold = DEFAULT_PORT_SCAN_THRESHOLD;
    int syn_threshold = DEFAULT_SYN_FLOOD_THRESHOLD;
    int icmp_threshold = DEFAULT_ICMP_FLOOD_THRESHOLD;
    char errbuf[PCAP_ERRBUF_SIZE];
    
    printf("====================================\n");
    printf("Network Intrusion Detection System\n");
    printf("Windows (Npcap/WinPcap)\n");
    printf("====================================\n\n");
    
    /* Parse command-line arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            show_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--interface") == 0 && i + 1 < argc) {
            interface = argv[++i];
        } else if (strcmp(argv[i], "--port-threshold") == 0 && i + 1 < argc) {
            port_threshold = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--syn-threshold") == 0 && i + 1 < argc) {
            syn_threshold = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--icmp-threshold") == 0 && i + 1 < argc) {
            icmp_threshold = atoi(argv[++i]);
        }
    }
    
    /* List available devices */
    if (list_devices(devices, &device_count) != 0) {
        printf("[ERROR] Failed to list network devices\n");
        return 1;
    }
    
    if (device_count == 0) {
        printf("[ERROR] No network devices found\n");
        return 1;
    }
    
    /* Select interface */
    if (interface == NULL) {
        print_devices(devices, device_count);
        printf("\nEnter interface number (0-%d): ", device_count - 1);
        
        int choice;
        if (scanf("%d", &choice) != 1 || choice < 0 || choice >= device_count) {
            printf("[ERROR] Invalid interface selection\n");
            return 1;
        }
        
        interface = devices[choice].name;
    }
    
    printf("\n[INFO] Selected interface: %s\n", interface);
    printf("[INFO] Thresholds - Port: %d, SYN: %d, ICMP: %d\n",
           port_threshold, syn_threshold, icmp_threshold);
    
    /* Open network interface */
    pcap_t *handle = open_interface(interface, errbuf);
    if (handle == NULL) {
        printf("[ERROR] Could not open interface: %s\n", errbuf);
        return 1;
    }
    
    printf("[INFO] Interface opened successfully\n");
    
    /* Create logger */
    Logger *logger = logger_create("alerts.log");
    if (logger == NULL) {
        printf("[ERROR] Could not create logger\n");
        pcap_close(handle);
        return 1;
    }
    
    logger_log_message(logger, "NIDS started");
    
    /* Create detection engine */
    DetectionEngine *engine = detection_engine_create(port_threshold, syn_threshold, icmp_threshold);
    if (engine == NULL) {
        printf("[ERROR] Could not create detection engine\n");
        logger_log_message(logger, "Failed to create detection engine");
        logger_destroy(logger);
        pcap_close(handle);
        return 1;
    }
    
    /* Create packet queue */
    PacketQueue *queue = queue_create();
    if (queue == NULL) {
        printf("[ERROR] Could not create packet queue\n");
        logger_log_message(logger, "Failed to create packet queue");
        detection_engine_destroy(engine);
        logger_destroy(logger);
        pcap_close(handle);
        return 1;
    }
    
    /* Create capture thread arguments */
    CaptureThreadArgs capture_args;
    capture_args.handle = handle;
    capture_args.queue = queue;
    capture_args.running = 1;
    
    /* Create process thread arguments */
    ProcessThreadArgs process_args;
    process_args.queue = queue;
    process_args.engine = engine;
    process_args.logger = logger;
    process_args.running = 1;
    
    printf("[INFO] Starting capture and processing threads\n");
    
    /* Create threads */
    HANDLE capture_handle = (HANDLE)_beginthreadex(NULL, 0, capture_thread, &capture_args, 0, NULL);
    HANDLE process_handle = (HANDLE)_beginthreadex(NULL, 0, process_thread, &process_args, 0, NULL);
    
    if (capture_handle == NULL || process_handle == NULL) {
        printf("[ERROR] Could not create threads\n");
        logger_log_message(logger, "Failed to create threads");
        capture_args.running = 0;
        process_args.running = 0;
        queue_destroy(queue);
        detection_engine_destroy(engine);
        logger_destroy(logger);
        pcap_close(handle);
        return 1;
    }
    
    printf("[INFO] NIDS is running. Press Ctrl+C to stop.\n");
    logger_log_message(logger, "NIDS is monitoring traffic");
    
    /* Wait for user to stop */
    WaitForSingleObject(capture_handle, INFINITE);
    WaitForSingleObject(process_handle, INFINITE);
    
    /* Cleanup */
    printf("\n[INFO] Stopping NIDS...\n");
    capture_args.running = 0;
    process_args.running = 0;
    
    WaitForSingleObject(capture_handle, 2000);
    WaitForSingleObject(process_handle, 2000);
    
    CloseHandle(capture_handle);
    CloseHandle(process_handle);
    
    queue_destroy(queue);
    detection_engine_destroy(engine);
    logger_destroy(logger);
    pcap_close(handle);
    
    logger_log_message(logger, "NIDS stopped");
    
    printf("[INFO] NIDS stopped successfully\n");
    printf("[INFO] Alerts logged to: alerts.log\n");
    
    return 0;
}
