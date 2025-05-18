#ifndef JOYSTICK_H
#define JOYSTICK_H

#include <stdint.h>
#include <stdbool.h>


void joystick_init(void);
uint16_t joystick_read_x(void);
uint16_t joystick_read_y(void);
bool joystick_button_pressed();


#endif // JOYSTICK_H