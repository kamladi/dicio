/**
 * 18-748 Wireless Sensor Networks
 * Spring 2016
 * Dicio - A Smart Outlet Mesh Network
 * assembler.h
 * Kedar Amladi // kamladi. Daniel Santoro // ddsantor. Adam Selevan // aselevan.
 */
 
#ifndef __assembler_h	
#define __assembler_h

#include <type_defs.h>

void assemble_serv_packet(uint8_t *tx_buf, packet *tx);
uint8_t assemble_packet(uint8_t *tx_buf, packet *tx);

#endif