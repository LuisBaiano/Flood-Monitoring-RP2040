#include "debouncer.h"
#include "pico/stdlib.h"

bool check_debounce(uint32_t *last_event_time_us, uint32_t debounce_interval_us) {
    uint32_t current_time_us = time_us_32();
    if (current_time_us - *last_event_time_us > debounce_interval_us) {
        *last_event_time_us = current_time_us;
        return true;
    }
    return false;
}