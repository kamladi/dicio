#ifndef __dicio_spi_h   
#define __dicio_spi_h

#include <type_defs.h>
#include <hal.h>

void SPI_MasterInit(void);
uint8_t SPI_SendByte(uint8_t send);

#endif