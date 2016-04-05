/**
 * 18-748 Wireless Sensor Networks
 * Spring 2016
 * Dicio - A Smart Outlet Mesh Network
 * dicio_spi.c
 * Kedar Amladi // kamladi. Daniel Santoro // ddsantor. Adam Selevan // aselevan.
 */

#define DD_MOSI 2
#define DD_SCK 1
#define SPE 6
#define MSTR 4
#define CPOL 3
#define CPHA 2
#define SPR0 0
#define SPIF 7

#include <dicio_spi.h>

/**
 * SPI_MasterInit - 
 *  Initialize a node as a SPI Master.
 */
void SPI_MasterInit() {
    // set power reduction register accordingly 
    PRR0 &= 0xFD; // Clear bit 2 to enable SPI

    /* Set MOSI and SCK output, all others input */     
    DDRB = (1 << DD_MOSI) | (1 << DD_SCK);

    /* Enable SPI, Master, set clock rate fck / 4 */     
    SPCR = (1 << SPE) | (1 << MSTR) | (1 << CPOL) | (1 << CPHA) | 0x01; 

    // dont double the SPI frequency
    SPSR |= 0x00;
}

/**
 * SPI_SlaveInit - 
 *  Initialize a port for use as a SPI chip select.
 */
void SPI_SlaveInit(uint8_t port) {
  nrk_gpio_direction(port, NRK_PIN_OUTPUT);
  nrk_gpio_set(port);
}

/**
 * SPI_SendByte - 
 *  send a byte via SPI, return response.
 */
uint8_t SPI_SendByte(uint8_t send) {
    uint8_t receive;
    SPDR = send;
    while (!(SPSR & (1 << SPIF)));
    receive = SPDR;
    while (SPSR & (1 << SPIF));
    return receive;
}

/**
 * SPI_SendBuffer - 
 *  Send multiple bytes via SPI, return all responses.
 */
void SPI_SendBuffer(uint8_t *send, uint8_t *receive, uint8_t len) {
    for(uint8_t i = 0; i < len; i++) {
        receive[i] = SPI_SendByte(send[i]);
    }
}

/**
 * SPI_SendMessage -
 *  Send a message to a particular external chip.
 */
void SPI_SendMessage(uint8_t *send, uint8_t *receive, uint8_t len, uint8_t CS) {
    nrk_gpio_clr(CS);
    SPI_SendBuffer(send, receive, len);
    nrk_gpio_set(CS);
}






