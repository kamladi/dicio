/**
 * 18-748 Wireless Sensor Networks
 * Spring 2016
 * Dicio - A Smart Outlet Mesh Network
 * power_sensor.h
 * Kedar Amladi // kamladi. Daniel Santoro // ddsantor. Adam Selevan // aselevan.
 */

#ifndef __power_sensor_h 
#define __power_sensor_h

#include <type_defs.h>


#define PWR_CS NRK_PORTB_5
//#define PWR_CS NRK_PORTF_2
#define PWR_MSG_LEN 5

// OUTPUT REGISTERS 
#define COMMAND     0x00 // command register
#define FWVERSION   0x03 // firmware version
#define TEMPERATURE 0x12 // chip temperature
#define VA          0x15 // apparent power
#define VAR         0x18 // reactive power
#define VRMS        0x1B // RMS voltage
#define IRMS        0x1E // RMS current
#define WATT        0x21 // active power
#define PA_AVERAGE  0x24 // active power over 30s
#define PF          0x27 // power factor
#define FREQ        0x2A // line frequency
#define ALARMS      0x30 // alarm status registers
#define VFUND       0x33 // RMS voltage (fundamental)
#define IFUND       0x36 // RMS current (fundamental)
#define PFUND       0x39 // active power (fundamental)
#define QFUND       0x3C // reactive power (fundamental)
#define VHARM       0x42 // RMS voltage (harmonic)
#define IHARM       0x45 // RMS current (harmonic)
#define PHARM       0x48 // active power (harmonic)
#define QHARM       0x4B // reactive power (harmonic)
#define VAHARM      0x4E // apparent power (harmonic)
#define ILOW        0x57 // min RMS current
#define IHIGH       0x5A // max RMS current
#define IPEAK       0x5D // highest current
#define VLOW        0x60 // min RMS voltage
#define VHIGH       0x63 // max RMS current
#define VPEAK       0x66 // highest voltage
#define EXTTEMP1    0xD5 // external temp 1
#define EXTTEMP2    0xD8 // external temp2
#define DIVISOR     0x10E // accumulation interval
#define SIOState    0x147 // state of DIO outputs
#define IRMS1       0x156 // unscaled RMS current
#define VRMS1       0x159 // unscaled RMS voltage
#define POWER1      0x15C // unscaled active power
#define VAR1        0x15F // unscaled reactive power
#define VA1         0x162 // unscaled apparent power
#define PF1         0x165 // unscaled power factor
#define FREQ1       0x168 // unscaled frequency

// INPUT REGISTERS
#define PHASECOMP   0x06 // phase compensation
#define ALARM_MASK1 0x09 // alarm mask bits for AC FAULT
#define ALARM_SET   0x0C // sets corresponding alarm bits
#define ALARM_RESET 0x0F // clears corresponding alarm bits
#define DEVADDR     0x2D // UART and I2C address
#define ALARM_MASK2 0x51 // alarm mark bits for ACCRIT
#define STICKY      0x54 // alarm bits to control auto-reset
#define IROFF       0xA5 // RMD current offset adjust
#define VROFF       0xA8 // RMS voltage offset adjust
#define POFF        0xAB // Power offset adjust
#define ACCUMCYC    0xCC // number of line cycles for low rate
#define PTARGET     0xCF // current gain calibration, power target
#define BAUD        0xD2 // UART baud rate
#define TEMP1GAIN   0xDB // external temperature gain
#define TEMP2GAIN   0xDE // external temperature gain
#define HPFCOEEFI   0xE1 // current offset removal settling time
#define HPFCOEFFV   0xE4 // voltage offset remobal settling time
#define ITARGET     0xE7 // current gain calibration, current target
#define VTARGET     0XEA // voltage gain calibration, voltage target
#define CALCYC      0xED // number of calibration cycles to average
#define IGAIN       0xF0 // current gain setting
#define VGAIN       0xF3 // voltage gain setting
#define IOFF        0xF6 // current offset
#define VOFF        0xF9 // voltage offset 
#define TGAIN       0xFC // die temperature gain setting
#define TOFFS       0xFF // die temperature offset
#define SYSGAIN     0x102 // temperatre compensated system gain
#define HARM        0x105 // harmonic selector
#define SACCUM      0x108 // accumlation interval for Sag and Surge
#define ACCUM       0x10B // accumlation interval for calculation
#define FRAME       0x111 // low rate frame number counter
#define XYCOMP      0x117 // line filters capacitive compensation
#define RCOMP       0x11A // resistive I/R drop compensation
#define ISCALE      0x11D // current scaling register
#define VSCALE      0x120 // voltage scaling register
#define PSCALE      0x123 // power scaling register
#define PFSCALE     0x126 // power factor scaling register
#define FSCALE      0x129 // frequency scaling register
#define TSCALE      0x12C // temperature scaling register
#define TC1         0x14A // temperature compensation
#define TC2         0x14D // temperature compensation 

// scale constants
#define PWR_MULT  55
#define PWR_SCALE 10
#define IMAX        0x002710 // 10 A
#define VMAX        0x01D4C0 // 120 V
#define PMAX        0x124F80 // 1200 VA
#define GET_LOW_BYTE(x) (uint8_t)(x & 0xFF)
#define GET_MID_BYTE(x) (uint8_t)((x >> 8) & 0xFF)
#define GET_TOP_BYTE(x) (uint8_t)((x >> 16) & 0xFF)

#define GET_REG_ADDR(x) (((uint8_t)((uint16_t)x / 3)) & 0x3F) << 2

void pwr_init();
void pwr_read(uint16_t reg, uint8_t *read_buf);
void pwr_write(uint16_t reg, uint8_t *write_buf);
int16_t transform_pwr(int16_t counts);

#endif
