#ifndef LOGGER_H
#define LOGGER_H

#include "detection.h"
#include <stdio.h>

typedef struct {
    FILE *log_file;
} Logger;

Logger *logger_create(const char *log_path);
void logger_destroy(Logger *logger);
void logger_log_alert(Logger *logger, Alert *alert);
void logger_log_message(Logger *logger, const char *message);

#endif
