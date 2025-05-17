#include "led_matrix.h"
#include "config.h"
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "led_matrix.pio.h"

#include <math.h>   // For fmaxf, fminf
#include <string.h> // For memset if used

// --- Internal Definitions ---
static PIO pio_instance = MATRIX_PIO_INSTANCE; // From hardware_config.h
static uint pio_sm = MATRIX_PIO_SM;           // From hardware_config.h
static uint32_t pixel_buffer[MATRIX_SIZE];    // Framebuffer for the matrix

// Brightness control for the matrix
#define MATRIX_GLOBAL_BRIGHTNESS 0.15f // Adjust as needed (0.0 to 1.0)

// Color structure
typedef struct {
    float r;
    float g;
    float b;
} ws2812b_color_t;

// Predefined colors
static const ws2812b_color_t COLOR_BLACK     = {0.0f, 0.0f, 0.0f};
static const ws2812b_color_t COLOR_RED_ALERT = {1.0f, 0.0f, 0.0f}; // Intense Red for high alert
static const ws2812b_color_t COLOR_YELLOW_WATER= {0.8f, 0.8f, 0.0f}; // Yellowish for water level
static const ws2812b_color_t COLOR_BLUE_RAIN = {0.0f, 0.3f, 1.0f}; // Blueish for rain
static const ws2812b_color_t COLOR_ORANGE_BOTH = {1.0f, 0.4f, 0.0f}; // Orange for both alerts
static const ws2812b_color_t COLOR_GREEN_NORMAL= {0.0f, 1.0f, 0.0f}; // Green for normal status

// Physical LED mapping (ensure this matches your BitDogLab matrix)
static const uint8_t Leds_Matrix_position[MATRIX_DIM][MATRIX_DIM] = {
    {24, 23, 22, 21, 20},
    {15, 16, 17, 18, 19},
    {14, 13, 12, 11, 10},
    {5,  6,  7,  8,  9 },
    {4,  3,  2,  1,  0 }
};

// --- Static Helper Functions ---

// Get logical index from 1-based row/col
static inline int get_pixel_index(int row, int col) {
    if (row >= 1 && row <= MATRIX_DIM && col >= 1 && col <= MATRIX_DIM) {
        return Leds_Matrix_position[row - 1][col - 1];
    }
    return -1; // Invalid coordinate
}

// Convert color to PIO format with brightness
static inline uint32_t color_to_pio_grb_format(ws2812b_color_t color, float brightness) {
    brightness = fmaxf(0.0f, fminf(1.0f, brightness));
    float r = fmaxf(0.0f, fminf(1.0f, color.r * brightness));
    float g = fmaxf(0.0f, fminf(1.0f, color.g * brightness));
    float b = fmaxf(0.0f, fminf(1.0f, color.b * brightness));

    uint8_t R_val = (uint8_t)(r * 255.0f + 0.5f);
    uint8_t G_val = (uint8_t)(g * 255.0f + 0.5f);
    uint8_t B_val = (uint8_t)(b * 255.0f + 0.5f);

    return ((uint32_t)(G_val) << 16) | ((uint32_t)(R_val) << 8) | B_val;
}

// Update the physical matrix with the contents of pixel_buffer
static void matrix_render() {
    for (int i = 0; i < MATRIX_SIZE; ++i) {
        pio_sm_put_blocking(pio_instance, pio_sm, pixel_buffer[i]);
    }
    busy_wait_us(50); // WS2812 reset time
}

// Set a pixel in the buffer using 1-based row/col
static void led_activate_position_at(int row, int col, ws2812b_color_t color, float brightness_mod) {
    int index = get_pixel_index(row, col);
    if (index != -1) {
        pixel_buffer[index] = color_to_pio_grb_format(color, MATRIX_GLOBAL_BRIGHTNESS * brightness_mod);
    }
}
// Overload for default brightness
static void led_activate_position(int row, int col, ws2812b_color_t color) {
    led_activate_position_at(row, col, color, 1.0f);
}


// --- Public API Functions ---

void led_matrix_init() {
    uint offset = pio_add_program(pio_instance, &led_matrix_program);
    led_matrix_program_init(pio_instance, pio_sm, offset, MATRIX_WS2812_PIN);
    led_matrix_clear();
    printf("LED Matrix Initialized (Pin: %d, PIO: %d, SM: %d)\n", MATRIX_WS2812_PIN, pio_get_index(pio_instance), pio_sm);
}

void led_matrix_clear() {
    uint32_t pio_black = color_to_pio_grb_format(COLOR_BLACK, 1.0f);
    for (int i = 0; i < MATRIX_SIZE; ++i) {
        pixel_buffer[i] = pio_black;
    }
    matrix_render();
}

void led_matrix_display_normal_status() {

    led_activate_position(1, 3, COLOR_GREEN_NORMAL);
    led_activate_position(2, 2, COLOR_GREEN_NORMAL);
    led_activate_position(3, 1, COLOR_GREEN_NORMAL);
    led_activate_position(3, 2, COLOR_GREEN_NORMAL);
    led_activate_position(3, 3, COLOR_GREEN_NORMAL);
    led_activate_position(4, 4, COLOR_GREEN_NORMAL);
    led_activate_position(5, 5, COLOR_GREEN_NORMAL);

    matrix_render();
}

void led_matrix_display_alert(AlertLevel_t level, uint8_t water_percent, uint8_t rain_percent) {

    // 1. General Alert Icon (e.g., Exclamation Mark !)
    if (level != ALERT_NONE) {
        ws2812b_color_t alert_icon_color = COLOR_RED_ALERT;
        if (level == ALERT_WATER_HIGH) alert_icon_color = COLOR_YELLOW_WATER;
        else if (level == ALERT_RAIN_HIGH) alert_icon_color = COLOR_BLUE_RAIN;
        else if (level == ALERT_BOTH_HIGH) alert_icon_color = COLOR_ORANGE_BOTH;

        led_activate_position(1, 3, alert_icon_color);
        led_activate_position(2, 3, alert_icon_color);
        led_activate_position(3, 3, alert_icon_color);
        // led_activate_position(4, 3, COLOR_BLACK); // Gap
        led_activate_position(5, 3, alert_icon_color); // Dot
    }

    // 2. Water Level Bar (Left side, e.g., column 1)
    // Scale percent to number of rows (MATRIX_DIM = 5)
    int water_rows_lit = (water_percent * MATRIX_DIM) / 100;
    if (water_rows_lit > MATRIX_DIM) water_rows_lit = MATRIX_DIM;
    if (water_rows_lit < 0) water_rows_lit = 0;

    for (int r = 1; r <= MATRIX_DIM; ++r) {
        if ((MATRIX_DIM - r + 1) <= water_rows_lit) { // Fill from bottom up
            led_activate_position(r, 1, COLOR_BLUE_RAIN); // Use blue for water
        } else {
            // led_activate_position(r, 1, COLOR_BLACK); // Ensure other parts are off if not covered by icon
        }
    }

    // 3. Rain Indicator (Right side, e.g., column 5, simple on/off for heavy rain)
    if (rain_percent >= RAIN_VOLUME_ALERT_THRESHOLD) {
        // Simple blinking "drops" or a static pattern
        led_activate_position(1, 5, COLOR_BLUE_RAIN);
        led_activate_position(3, 5, COLOR_BLUE_RAIN);
        led_activate_position(5, 5, COLOR_BLUE_RAIN);
    } else if (rain_percent > 40) { // Moderate rain
        led_activate_position(2, 5, COLOR_BLUE_RAIN);
        led_activate_position(4, 5, COLOR_BLUE_RAIN);
    }


    // If no specific icons and it's an alert, make the whole matrix flash a color
    if (level != ALERT_NONE && water_rows_lit == 0 && rain_percent < 40) {
        ws2812b_color_t flash_color = COLOR_RED_ALERT;
        if (level == ALERT_WATER_HIGH) flash_color = COLOR_YELLOW_WATER;
        else if (level == ALERT_RAIN_HIGH) flash_color = COLOR_BLUE_RAIN;
        else if (level == ALERT_BOTH_HIGH) flash_color = COLOR_ORANGE_BOTH;

        for (int r = 1; r <= MATRIX_DIM; ++r) {
            for (int c = 1; c <= MATRIX_DIM; ++c) {
                led_activate_position(r, c, flash_color);
            }
        }
    }

    matrix_render();
}