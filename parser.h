#ifndef PARSER_H
#define PARSER_H

#include "capture.h"
#include <time.h>

int parse_packet(const u_char *packet, int length, ParsedPacket *parsed);
const char *protocol_to_string(uint8_t protocol);
const char *tcp_flags_to_string(uint8_t flags);

#endif
