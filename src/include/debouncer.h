#ifndef DEBOUNCER_H
#define DEBOUNCER_H

#include <stdbool.h>
#include <stdint.h>

bool check_debounce(uint32_t *last_event_time_us, uint32_t debounce_interval_us);

#endif // DEBOUNCER_H