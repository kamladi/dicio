/**
 * 18-748 Wireless Sensor Networks
 * Spring 2016
 * Dicio - A Smart Outlet Mesh Network
 * power_sensor.c
 * Kedar Amladi // kamladi. Daniel Santoro // ddsantor. Adam Selevan // aselevan.
 */

#include <power_sensor.h>
#include <dicio_spi.h>

// pwr_init - initialize power sensor
void pwr_init() {
    uint8_t Iscale[3];
    uint8_t Vscale[3];
    uint8_t Pscale[3];
    uint8_t Zeros[3];

    // current scaling factor
    Iscale[0] = GET_TOP_BYTE(IMAX);
    Iscale[1] = GET_MID_BYTE(IMAX);
    Iscale[2] = GET_LOW_BYTE(IMAX);
    pwr_write(ISCALE, &Iscale);

    // voltage scaling factor: 120V -> 0x01D4C0
    Vscale[0] = GET_TOP_BYTE(VMAX);
    Vscale[1] = GET_MID_BYTE(VMAX);
    Vscale[2] = GET_LOW_BYTE(VMAX);
    pwr_write(VSCALE, &Vscale);

    // power scaling factor: 1200VA -> 0x124F80
    Pscale[0] = GET_TOP_BYTE(PMAX);
    Pscale[1] = GET_MID_BYTE(PMAX);
    Pscale[2] = GET_LOW_BYTE(PMAX);
    pwr_write(PSCALE, &Pscale);

    // sticky reg
    Zeros[0] = 0x00;
    Zeros[1] = 0x00;
    Zeros[2] = 0x00;
    pwr_write(STICKY, Zeros);
    pwr_write(ALARM_MASK1, Zeros);
    pwr_write(ALARM_MASK2, Zeros);
}

// pwr_read - read from the power sensor
void pwr_read(uint16_t reg, uint8_t *read_buf) {
    // initialize the message
    uint8_t send_receive_buf[5];
    send_receive_buf[0] = 0x01;             // header
    send_receive_buf[1] = GET_REG_ADDR(reg);    // address
    send_receive_buf[2] = 0x00;             // padding for reading
    send_receive_buf[3] = 0x00;             // padding for reading
    send_receive_buf[4] = 0x00;             // padding for reading

    // send message
    SPI_SendMessage(&send_receive_buf, &send_receive_buf, PWR_MSG_LEN, PWR_CS);

    // get the return value
    read_buf[0] = send_receive_buf[2];
    read_buf[1] = send_receive_buf[3];
    read_buf[2] = send_receive_buf[3];
}

// pwr_write - write to the power sensor
void pwr_write(uint16_t reg, uint8_t *write_buf) {
    // initialize the message
    uint8_t send_receive_buf[5];
    send_receive_buf[0] = 0x01;
    send_receive_buf[1] = GET_REG_ADDR(reg);
    send_receive_buf[2] = write_buf[0];
    send_receive_buf[3] = write_buf[1];
    send_receive_buf[4] = write_buf[2];

    // send the message
    SPI_SendMessage(&send_receive_buf, &send_receive_buf, PWR_MSG_LEN, PWR_CS);
}

// transform_pwr - change power sensor reading to a fixed point value
int16_t transform_pwr(int16_t counts) {
    int16_t integer = (counts / PWR_MULT) * PWR_SCALE;
    int16_t decimal = ((counts % PWR_MULT) * PWR_SCALE) / PWR_MULT;
    return integer + decimal;
}