/**
 * 18-748 Wireless Sensor Networks
 * Spring 2016
 * Dicio - A Smart Outlet Mesh Network
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
