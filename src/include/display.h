#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>
#include <stdbool.h>
#include "config.h"
#include "lib/ssd1306/ssd1306.h"

void display_init(ssd1306_t *ssd); 
void display_startup_screen(ssd1306_t *ssd);

#endif // DISPLAY_H