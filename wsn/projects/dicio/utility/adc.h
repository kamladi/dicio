/**
 * 18-748 Wireless Sensor Networks
 * Spring 2016
 * Dicio - A Smart Outlet Mesh Network
 * adc.h
 * Kedar Amladi // kamladi. Daniel Santoro // ddsantor. Adam Selevan // aselevan.
 */
 
#ifndef __adc_h	
#define __adc_h

#include <type_defs.h>

#define TEMP_MULT 100
#define TEMP_DIV  1023
#define TEMP_RANGE 16
#define TEMP_GAIN 20
#define TEMP_ZERO 256
#define TEMP_HUNS_MULT 100
#define TEMP_TENS_MULT 10
#define TEMP_TENS_DIV 102

int16_t transform_temp(uint16_t tempCounts);

#endif