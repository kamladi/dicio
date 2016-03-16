/**
 * 18-748 Wireless Sensor Networks
 * Spring 2016
 * Lab 3: Multi-Hop Communication
 * sample_sensor.c
 * Kedar Amladi // kamladi. Daniel Santoro // ddsantor. Adam Selevan // aselevan.
 */
 
#include <sample_sensor.h>

uint16_t sample_light(){
    uint8_t fd;
    int8_t val;
    uint16_t light;
    
    // Open ADC device as read 
    fd=nrk_open(FIREFLY_3_SENSOR_BASIC,READ);
    if(fd==NRK_ERROR) nrk_kprintf(PSTR("Failed to open sensor driver\r\n"));
    
    // state actions
    val = nrk_set_status(fd,SENSOR_SELECT,LIGHT);
    val = nrk_read(fd,&light,2);
    //printf("...light/threshold=%d/%d\r\n",light, light_threshold);
    
    nrk_close(fd); // power down sensor for power savings
    
    return light;
}