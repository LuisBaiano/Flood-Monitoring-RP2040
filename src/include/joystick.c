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

uint8_t joystick_adc_to_percent(uint16_t raw_value) {
    // Alternative considering deadzone (if center is 0% and full deflection is 100%)
    
    int32_t relative_value = (int32_t)raw_value - ADC_CENTER;
    if (relative_value < ADC_DEADZONE) {
        return 0; // Or 50 if center is 50%
    }
    // Scale remaining range
    // This example assumes joystick resting at one end gives 0%, fully deflected gives 100%
    // Adjust if your joystick has a different physical behavior for min/max readings.
    if (raw_value > ADC_MAX_VALUE) raw_value = ADC_MAX_VALUE;
    float percent = ((float)raw_value / ADC_MAX_VALUE) * 100.0f;
    return (uint8_t)(percent > 100.0f ? 100.0f : (percent < 0.0f ? 0.0f : percent));
    
}

bool joystick_button_is_pressed() {
    return !gpio_get(JOYSTICK_BTN_PIN); // Active low due to pull-up
}