/**
 * 18-748 Wireless Sensor Networks
 * Spring 2016
 * Dicio - A Smart Outlet Mesh Network
 * adc.c
 * Kedar Amladi // kamladi. Daniel Santoro // ddsantor. Adam Selevan // aselevan.
 */

#include <adc.h>

// transform_temp - transform the temperature to a fixed point number
int16_t transform_temp(uint16_t temp_counts) {
    // remove the zero
    int16_t zero = temp_counts - TEMP_ZERO;

    // scale the counts to the temp range
    int16_t scaled = zero * TEMP_RANGE;

    // get the integer part of the temperature
    int16_t integer = (scaled / TEMP_DIV) * TEMP_HUNS_MULT;

    // get the decimal part of the temperature
    int16_t decimal = ((scaled % TEMP_DIV) * TEMP_TENS_MULT) / TEMP_TENS_DIV;

    // combine the integer and decimal parts and return
    return  integer + decimal;
}
