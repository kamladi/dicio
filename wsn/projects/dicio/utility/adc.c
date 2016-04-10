/**
 * 18-748 Wireless Sensor Networks
 * Spring 2016
 * Dicio - A Smart Outlet Mesh Network
 * adc.c
 * Kedar Amladi // kamladi. Daniel Santoro // ddsantor. Adam Selevan // aselevan.
 */

#include <adc.h>

int16_t transform_temp(uint16_t temp_counts) {
    int16_t zero = temp_counts - TEMP_ZERO;
    int16_t scaled = zero * TEMP_RANGE;
    int16_t integer = (scaled / TEMP_DIV) * TEMP_HUNS_MULT;
    int16_t decimal = ((scaled % TEMP_DIV) * TEMP_TENS_MULT) / TEMP_TENS_DIV;
    return  integer + decimal;
}
