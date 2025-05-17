#ifndef JOYSTICK_H
#define JOYSTICK_H

#include <stdint.h>
#include <stdbool.h>


void joystick_init(void);
uint16_t joystick_read_x(void);
uint16_t joystick_read_y(void);
uint8_t joystick_adc_to_percent(uint16_t raw_value);
bool joystick_button_pressed();


#endif // JOYSTICK_H