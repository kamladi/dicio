/**
 * 18-748 Wireless Sensor Networks
 * Spring 2016
 * Lab 3: Multi-Hop Communication
 * parser.h
 * Kedar Amladi // kamladi. Daniel Santoro // ddsantor. Adam Selevan // aselevan.
 */

#ifndef __parser_h
#define __parser_h

/*** INCLUDE STATEMENTS ***/
#include <type_defs.h>

void print_packet(packet *p);
void parse_msg(packet *parsed_packet, uint8_t *src, uint8_t len);

#endif
