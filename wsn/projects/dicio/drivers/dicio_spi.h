/**
 * 18-748 Wireless Sensor Networks
 * Spring 2016
 * Dicio - A Smart Outlet Mesh Network
 * dicio_spi.h
 * Kedar Amladi // kamladi. Daniel Santoro // ddsantor. Adam Selevan // aselevan.
 */

#ifndef __dicio_spi_h   
#define __dicio_spi_h

#include <type_defs.h>
#include <hal.h>

void SPI_MasterInit(void);
uint8_t SPI_SendByte(uint8_t send);
void SPI_SendBuffer(uint8_t *send, uint8_t *receive, uint8_t len);
void SPI_SendMessage(uint8_t *send, uint8_t *receive, uint8_t len, uint8_t CS);

#endif