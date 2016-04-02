#define DD_MOSI 2
#define DD_SCK 1
#define SPE 6
#define MSTR 4
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

    /* Enable SPI, Master, set clock rate fck / 2 */     
    SPCR = (1 << SPE) | (1 << MSTR); 

    // double the SPI frequency
    SPSR |= 0x01;
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
 * SPI_SendMessage - 
 *  Send multiple bytes via SPI, return all responses.
 */
void SPI_SendMessage(uint8_t *send, uint8_t *receive, uint8_t len) {
    for(uint8_t i = 0; i < len; i++) {
        receive[i] = SPI_SendByte(send[i]);
    }
}


