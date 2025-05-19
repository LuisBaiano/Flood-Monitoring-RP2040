#include "joystick.h"
#include "config.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"

void joystick_init() {
    adc_init();
    adc_gpio_init(JOYSTICK_ADC_X_PIN);
    adc_gpio_init(JOYSTICK_ADC_Y_PIN);
}

uint16_t joystick_read_x() {
    adc_select_input(JOYSTICK_ADC_X_CHAN);
    return adc_read();
}

uint16_t joystick_read_y() {
    adc_select_input(JOYSTICK_ADC_Y_CHAN);
    return adc_read();
}
