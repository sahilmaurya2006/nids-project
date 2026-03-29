#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <winsock2.h>

Logger *logger_create(const char *log_path) {
    Logger *logger = (Logger *)malloc(sizeof(Logger));
    if (logger == NULL) {
        return NULL;
    }
    
    logger->log_file = fopen(log_path, "a");
    if (logger->log_file == NULL) {
        printf("Warning: Could not open log file: %s\n", log_path);
        /* Continue without file logging */
    }
    
    return logger;
}

void logger_destroy(Logger *logger) {
    if (logger) {
        if (logger->log_file) {
            fclose(logger->log_file);
        }
        free(logger);
    }
}

static const char *alert_type_to_string(AlertType type) {
    switch (type) {
        case ALERT_PORT_SCAN:
            return "PORT_SCAN";
        case ALERT_SYN_FLOOD:
            return "SYN_FLOOD";
        case ALERT_ICMP_FLOOD:
            return "ICMP_FLOOD";
        default:
            return "UNKNOWN";
    }
}

static void ip_to_string(uint32_t ip, char *buffer, size_t size) {
    unsigned char *bytes = (unsigned char *)&ip;
    snprintf(buffer, size, "%d.%d.%d.%d", bytes[0], bytes[1], bytes[2], bytes[3]);
}

void logger_log_alert(Logger *logger, Alert *alert) {
    if (alert == NULL) {
        return;
    }
    
    char src_ip[20];
    char dst_ip[20];
    char timestamp_str[100];
    
    ip_to_string(alert->src_ip, src_ip, sizeof(src_ip));
    ip_to_string(alert->dst_ip, dst_ip, sizeof(dst_ip));
    
    time_t raw_time = alert->timestamp;
    struct tm *timeinfo = localtime(&raw_time);
    strftime(timestamp_str, sizeof(timestamp_str), "%Y-%m-%d %H:%M:%S", timeinfo);
    
    /* Print to console */
    printf("[%s] ALERT: %s | Source: %s | Destination: %s | %s\n",
           timestamp_str, alert_type_to_string(alert->type), src_ip, dst_ip, alert->description);
    
    /* Log to file */
    if (logger && logger->log_file) {
        fprintf(logger->log_file, "[%s] ALERT: %s | Source: %s | Destination: %s | %s\n",
                timestamp_str, alert_type_to_string(alert->type), src_ip, dst_ip, alert->description);
        fflush(logger->log_file);
    }
}

void logger_log_message(Logger *logger, const char *message) {
    if (message == NULL) {
        return;
    }
    
    char timestamp_str[100];
    time_t raw_time = time(NULL);
    struct tm *timeinfo = localtime(&raw_time);
    strftime(timestamp_str, sizeof(timestamp_str), "%Y-%m-%d %H:%M:%S", timeinfo);
    
    /* Print to console */
    printf("[%s] %s\n", timestamp_str, message);
    
    /* Log to file */
    if (logger && logger->log_file) {
        fprintf(logger->log_file, "[%s] %s\n", timestamp_str, message);
        fflush(logger->log_file);
    }
}
