
#include <power_sensor.h>
#include <dicio_spi.h>

void pwr_read(uint16_t reg, uint8_t *read_buf) {
    uint8_t send_receive_buf[5];

    send_receive_buf[0] = 0x01;             // header
    send_receive_buf[1] = GET_ADDR(reg);    // address
    send_receive_buf[2] = 0x00;             // padding for reading
    send_receive_buf[3] = 0x00;             // padding for reading
    send_receive_buf[4] = 0x00;             // padding for reading

    SPI_SendMessage(&send_receive_buf, &send_receive_buf, PWR_MSG_LEN, PWR_CS);
    read_buf[0] = send_receive_buf[2];
    read_buf[1] = send_receive_buf[3];
    read_buf[2] = send_receive_buf[4];
}


void pwr_write(uint16_t reg, uint8_t *write_buf) {
    uint8_t send_receive_buf[5];
    send_receive_buf[0] = 0x01;
    send_receive_buf[1] = GET_ADDR(reg);
    send_receive_buf[2] = write_buf[0];
    send_receive_buf[3] = write_buf[1];
    send_receive_buf[4] = write_buf[2];

    SPI_SendMessage(&send_receive_buf, &send_receive_buf, PWR_MSG_LEN, PWR_CS);
}

void pwr_init() {
    uint8_t Iscale[3];
    uint8_t Vscale[3];
    uint8_t Pscale[3];
    uint8_t Zeros[3];

    // current scaling factor: 10A -> 0x002710
    Iscale[0] = 0x00;
    Iscale[1] = 0x27;
    Iscale[2] = 0x10;
    pwr_write(ISCALE, &Iscale);

    // voltage scaling factor: 120V -> 0x01D4C0
    Vscale[0] = 0x01;
    Vscale[1] = 0xD4;
    Vscale[2] = 0xC0;
    pwr_write(VSCALE, &Vscale);

    // power scaling factor: 1200VA -> 0x124F80
    Pscale[0] = 0x12;
    Pscale[1] = 0x4F;
    Pscale[2] = 0x80;
    pwr_write(PSCALE, &Pscale);

    // sticky reg
    Zeros[0] = 0x00;
    Zeros[1] = 0x00;
    Zeros[2] = 0x00;
    pwr_write(STICKY, Zeros);
    pwr_write(ALARM_MASK1, Zeros);
    pwr_write(ALARM_MASK2, Zeros);
}