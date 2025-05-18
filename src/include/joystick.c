#include "joystick.h"
#include "config.h" // For pin defines, ADC_MAX_VALUE, etc.
#include "hardware/adc.h"
#include "hardware/gpio.h"

void joystick_init() {
    adc_init();
    // Make sure GPIO is high-impedance, no pullups etc
    adc_gpio_init(JOYSTICK_ADC_X_PIN);
    adc_gpio_init(JOYSTICK_ADC_Y_PIN);

    // Initialize joystick button pin as input with pull-up
    gpio_init(JOYSTICK_BTN_PIN);
    gpio_set_dir(JOYSTICK_BTN_PIN, GPIO_IN);
    gpio_pull_up(JOYSTICK_BTN_PIN);
    printf("Joystick ADC and Button Initialized.\n");
}

uint16_t joystick_read_x() {
    adc_select_input(JOYSTICK_ADC_X_CHAN);
    return adc_read();
}

uint16_t joystick_read_y() {
    adc_select_input(JOYSTICK_ADC_Y_CHAN);
    return adc_read();
}

bool joystick_button_is_pressed() {
    return !gpio_get(JOYSTICK_BTN_PIN); // Active low due to pull-up
}