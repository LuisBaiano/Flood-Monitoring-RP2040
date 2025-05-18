#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "hardware/adc.h" 

#include "buttons.h"     
#include "joystick.h"     
#include "buzzer.h"   

// FreeRTOS includes
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h" // For FreeRTOS Queues
#include "FreeRTOSConfig.h" // User's FreeRTOS config


// --- Estruturas de Dados para Filas (Flood Alert Project) ---
typedef struct {
    uint16_t water_level_raw;     // Leitura crua do ADC para nível da água (0-4095)
    uint16_t rain_volume_raw;     // Leitura crua do ADC para volume de chuva (0-4095)
    uint8_t water_level_percent;  // Nível da água convertido para percentual (0-100)
    uint8_t rain_volume_percent;  // Volume de chuva convertido para percentual (0-100)
} SensorData_t;

typedef enum {
    ALERT_NONE,         // Sem alerta, tudo normal
    ALERT_WATER_HIGH,   // Alerta: nível da água alto
    ALERT_RAIN_HIGH,    // Alerta: volume de chuva alto
    ALERT_BOTH_HIGH     // Alerta: ambos os níveis altos (situação crítica)
} AlertLevel_t;

typedef struct {
    AlertLevel_t level;             // O tipo de alerta atual
    uint8_t water_level_percent;  // Percentual do nível da água no momento do alerta
    uint8_t rain_volume_percent;  // Percentual do volume de chuva no momento do alerta
    bool is_alert_active;         // Flag indicando se qualquer alerta está ativo
} AlertStatus_t;



// --- Definições de Pinos (conforme seu arquivo) ---
#define JOYSTICK_ADC_X_PIN  26  // Simula Nível Água
#define JOYSTICK_ADC_Y_PIN  27  // Simula Volume Chuva
#define JOYSTICK_BTN_PIN    22  // Pode ser usado para alguma função extra (ex: silenciar alerta temporariamente)
#define JOYSTICK_ADC_X_CHAN 0
#define JOYSTICK_ADC_Y_CHAN 1

#define BUTTON_A_PIN    5   // Usado para alternar modos (Normal/Alerta Manual - opcional) ou outra função
#define BUTTON_B_PIN    6   // Pode ser usado para reset BOOTSEL ou outra função

// LED RGB (usado para status geral de alerta)
#define LED_RED_PIN     13
#define LED_GREEN_PIN   11
#define LED_BLUE_PIN    12

// Matriz de LEDs
#define MATRIX_WS2812_PIN 7
#define MATRIX_SIZE       25
#define MATRIX_DIM        5
#define MATRIX_PIO_INSTANCE pio0
#define MATRIX_PIO_SM       0

// Buzzer
#define BUZZER_PIN_MAIN     10 // Renomeado de BUZZER_PIN1
// #define BUZZER_PIN2 21 // Não usado neste projeto

// Display OLED
#define I2C_PORT        i2c1
#define I2C_SDA_PIN     14
#define I2C_SCL_PIN     15
#define DISPLAY_ADDR    0x3C
#define DISPLAY_WIDTH   128
#define DISPLAY_HEIGHT  64

// --- Constantes ---
#define DEBOUNCE_TIME_US 50000 // Reduzido para melhor responsividade do botão A

#define ADC_MIN_VALUE      0
#define ADC_MAX_VALUE      4095
#define ADC_CENTER         ((ADC_MAX_VALUE + ADC_MIN_VALUE) / 2)
#define ADC_DEADZONE       50    

// Limiares para Alerta (Percentual)
#define WATER_LEVEL_ALERT_THRESHOLD 70 // 70%
#define RAIN_VOLUME_ALERT_THRESHOLD 80 // 80%

// --- Tempos do Buzzer para Alerta (ms) ---
#define BUZZER_ALERT_WATER_FREQ     880 // A5
#define BUZZER_ALERT_WATER_ON_MS    300
#define BUZZER_ALERT_WATER_OFF_MS   300

#define BUZZER_ALERT_RAIN_FREQ      1200 // D#6
#define BUZZER_ALERT_RAIN_ON_MS     200
#define BUZZER_ALERT_RAIN_OFF_MS    400

#define BUZZER_ALERT_BOTH_FREQ      1500 // F#6 (mais agudo e rápido)
#define BUZZER_ALERT_BOTH_ON_MS     150
#define BUZZER_ALERT_BOTH_OFF_MS    150

// --- Tempos de Delay das Tarefas (ms) ---
#define JOYSTICK_READ_DELAY_MS    200  // Frequência de leitura do joystick
#define DATA_PROCESS_DELAY_MS     50   // Pequeno delay se não houver dados na fila
#define BUTTON_TASK_DELAY_MS      20
#define DISPLAY_UPDATE_DELAY_MS   500
#define RGB_LED_TASK_DELAY_MS     100
#define MATRIX_TASK_DELAY_MS      200
#define BUZZER_TASK_DELAY_MS      50   // Pequeno delay base para a tarefa do buzzer

// --- Configuração de Tarefas FreeRTOS ---
// Prioridades
#define PRIORITY_JOYSTICK_READ    (tskIDLE_PRIORITY + 4) // Mais alta para entrada de dados
#define PRIORITY_DATA_PROCESSING  (tskIDLE_PRIORITY + 3)
#define PRIORITY_BUTTON_MONITOR   (tskIDLE_PRIORITY + 2) // Se usado para alternar modo manual de alerta
#define PRIORITY_RGB_LED_ALERT    (tskIDLE_PRIORITY + 1)
#define PRIORITY_MATRIX_ALERT     (tskIDLE_PRIORITY + 1)
#define PRIORITY_BUZZER_ALERT     (tskIDLE_PRIORITY + 1)
#define PRIORITY_DISPLAY_INFO     (tskIDLE_PRIORITY + 0) // Mais baixa

// Tamanho das Stacks
#define STACK_MULTIPLIER_DEFAULT  2
#define STACK_MULTIPLIER_DISPLAY  4 // Display pode precisar de mais para strings
#define STACK_SIZE_DEFAULT        (configMINIMAL_STACK_SIZE * STACK_MULTIPLIER_DEFAULT)
#define STACK_SIZE_DISPLAY        (configMINIMAL_STACK_SIZE * STACK_MULTIPLIER_DISPLAY)

#endif // HARDWARE_CONFIG_H