/**
 * 18-748 Wireless Sensor Networks
 * Spring 2016
 * Dicio - A Smart Outlet Mesh Network
 * adc.c
 * Kedar Amladi // kamladi. Daniel Santoro // ddsantor. Adam Selevan // aselevan.
 */

#include <adc.h>

int16_t transform_temp(uint16_t temp_counts) {
    int16_t zeroed = temp_counts - TEMP_ZERO;
    int16_t mult = zeroed * TEMP_RANGE;
    int16_t  huns = ((mult - (mult % TEMP_DIV)) / TEMP_DIV) * TEMP_HUNS_MULT;
    int16_t tens = (mult % TEMP_DIV) * TEMP_TENS_MULT / TEMP_TENS_DIV;
    int16_t final = huns + tens;
    return  final;
}
