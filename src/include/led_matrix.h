#ifndef LED_MATRIX_H
#define LED_MATRIX_H

#include <stdint.h>
#include <stdbool.h>
#include "config.h"

void led_matrix_init();
void led_matrix_clear();
void led_matrix_display_alert(AlertLevel_t level, uint8_t water_percent, uint8_t rain_percent);
void led_matrix_display_normal_status();

#endif // LED_MATRIX_H