// Includes como antes
#include "config.h" // Certifique-se que este é o correto e inclui tudo
#include "display.h"         // Para display_init e display_flood_startup_screen
#include "led_matrix.h"      // Para led_matrix_init, led_matrix_display_alert, led_matrix_display_normal_status
#include "pico/stdlib.h"     // Para stdio_init_all, gpio_init, etc. (assumindo que joystick_init, etc. precisem)
#include "FreeRTOS.h"        // Para FreeRTOS
#include "task.h"            // Para xTaskCreate, vTaskStartScheduler, vTaskDelay
#include "queue.h"           // Para QueueHandle_t, xQueueCreate, xQueueSend, xQueueReceive
#include <stdio.h>           // Para printf
#include <string.h>          // Para memset, strcpy, sprintf

// Trecho para modo BOOTSEL com botão B
#include "pico/bootrom.h"
#define botaoB 6 // Defina o pino do botão B se não estiver em config.h
void gpio_irq_handler(uint gpio, uint32_t events) {
    reset_usb_boot(0, 0);
}
//-------------------

// --- Global Variables & Queues ---
ssd1306_t display_oled;

QueueHandle_t xSensorDataQueue;

// Filas de Alerta Individuais para Fan-Out
QueueHandle_t xDisplayAlertQueue;
QueueHandle_t xRgbLedAlertQueue;
QueueHandle_t xLedMatrixAlertQueue;
QueueHandle_t xBuzzerAlertQueue;

// --- Task Forward Declarations ---
void vJoystickReadTask(void *pvParameters);
void vDataProcessingTask(void *pvParameters);
void vRgbLedAlertTask(void *pvParameters);
void vLedMatrixAlertTask(void *pvParameters);
void vBuzzerAlertTask(void *pvParameters);
void vDisplayInfoTask(void *pvParameters);

// --- System Initialization ---
void init_system_flood_alert() {
    stdio_init_all();
    sleep_ms(1000); // Tempo para o terminal serial conectar
    printf("Initializing Flood Alert System...\n");

    joystick_init();
    buttons_init(); // Inicializa apenas BUTTON_A_PIN (5) e BUTTON_B_PIN (6)

    // Para ser utilizado o modo BOOTSEL com botão B
    gpio_init(botaoB);
    gpio_set_dir(botaoB, GPIO_IN);
    gpio_pull_up(botaoB);
    gpio_set_irq_enabled_with_callback(botaoB, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    buzzer_init();  // Usa BUZZER_PIN_MAIN (10)
    led_matrix_init();

    gpio_init(LED_RED_PIN); gpio_set_dir(LED_RED_PIN, GPIO_OUT); gpio_put(LED_RED_PIN, 0);
    gpio_init(LED_GREEN_PIN); gpio_set_dir(LED_GREEN_PIN, GPIO_OUT); gpio_put(LED_GREEN_PIN, 1); // Normal = Verde
    gpio_init(LED_BLUE_PIN); gpio_set_dir(LED_BLUE_PIN, GPIO_OUT); gpio_put(LED_BLUE_PIN, 0);

    display_init(&display_oled); // Função de display.c

    printf("System Hardware Initialized.\n");
}

// --- Main Function ---
int main() {
    init_system_flood_alert();
    display_startup_screen(&display_oled); // Função de display.c
    printf("Startup screen finished.\n");

    xSensorDataQueue = xQueueCreate(5, sizeof(SensorData_t));

    // Crie as filas de alerta individuais
    xDisplayAlertQueue   = xQueueCreate(3, sizeof(AlertStatus_t));
    xRgbLedAlertQueue    = xQueueCreate(3, sizeof(AlertStatus_t));
    xLedMatrixAlertQueue = xQueueCreate(3, sizeof(AlertStatus_t));
    xBuzzerAlertQueue    = xQueueCreate(3, sizeof(AlertStatus_t));

    if (xSensorDataQueue == NULL || xDisplayAlertQueue == NULL ||
        xRgbLedAlertQueue == NULL || xLedMatrixAlertQueue == NULL || xBuzzerAlertQueue == NULL) {
        printf("FATAL: Failed to create one or more queues!\n");
        while(1); // Parar aqui
    }
    printf("Queues created.\n");

    printf("Creating tasks...\n");
    xTaskCreate(vJoystickReadTask, "JoystickRead", STACK_SIZE_DEFAULT, NULL, PRIORITY_JOYSTICK_READ, NULL);
    xTaskCreate(vDataProcessingTask, "DataProcess", STACK_SIZE_DEFAULT, NULL, PRIORITY_DATA_PROCESSING, NULL);
    xTaskCreate(vRgbLedAlertTask, "RgbLedAlert", STACK_SIZE_DEFAULT, NULL, PRIORITY_RGB_LED_ALERT, NULL);
    xTaskCreate(vLedMatrixAlertTask, "MatrixAlert", STACK_SIZE_DEFAULT, NULL, PRIORITY_MATRIX_ALERT, NULL);
    xTaskCreate(vBuzzerAlertTask, "BuzzerAlert", STACK_SIZE_DEFAULT, NULL, PRIORITY_BUZZER_ALERT, NULL);
    xTaskCreate(vDisplayInfoTask, "DisplayInfo", STACK_SIZE_DISPLAY, &display_oled, PRIORITY_DISPLAY_INFO, NULL);

    printf("Starting FreeRTOS scheduler...\n");
    vTaskStartScheduler();

    // Não deve chegar aqui
    printf("Scheduler returned. Critical Error!\n");
    while(1);
    return 0; // Para satisfazer o compilador
}

// --- Task Implementations ---

void vDataProcessingTask(void *pvParameters) {
    SensorData_t received_data;
    AlertStatus_t alert_status;

    // Inicializa alert_status para um estado conhecido
    alert_status.is_alert_active = false;
    alert_status.level = ALERT_NONE;
    alert_status.water_level_percent = 0;
    alert_status.rain_volume_percent = 0;

    printf("Task DataProcessing started.\n");
    while (true) {
        if (xQueueReceive(xSensorDataQueue, &received_data, portMAX_DELAY)) {
            alert_status.water_level_percent = received_data.water_level_percent;
            alert_status.rain_volume_percent = received_data.rain_volume_percent;

            bool water_alert = (received_data.water_level_percent >= WATER_LEVEL_ALERT_THRESHOLD);
            bool rain_alert = (received_data.rain_volume_percent >= RAIN_VOLUME_ALERT_THRESHOLD);

            if (water_alert && rain_alert) {
                alert_status.level = ALERT_BOTH_HIGH;
                alert_status.is_alert_active = true;
            } else if (water_alert) {
                alert_status.level = ALERT_WATER_HIGH;
                alert_status.is_alert_active = true;
            } else if (rain_alert) {
                alert_status.level = ALERT_RAIN_HIGH;
                alert_status.is_alert_active = true;
            } else {
                alert_status.level = ALERT_NONE;
                alert_status.is_alert_active = false;
            }

            // DEBUG: Para verificar se o processamento e envio ocorrem
            printf("DataProc: Sending WL:%u%% RV:%u%% AlertActive:%d Level:%d\n",
                   alert_status.water_level_percent, alert_status.rain_volume_percent,
                   alert_status.is_alert_active, alert_status.level);

            // Envia para todas as filas de alerta consumidoras
            // Usar timeout 0 ou pequeno para não bloquear a DataProcessingTask
            // se uma fila de consumidor estiver cheia.
            if (xQueueSend(xDisplayAlertQueue, &alert_status, 0) != pdPASS) {
                 printf("DataProcessing: Failed to send to DisplayAlertQueue\n");
            }
            if (xQueueSend(xRgbLedAlertQueue, &alert_status, 0) != pdPASS) {
                 printf("DataProcessing: Failed to send to RgbLedAlertQueue\n");
            }
            if (xQueueSend(xLedMatrixAlertQueue, &alert_status, 0) != pdPASS) {
                 printf("DataProcessing: Failed to send to LedMatrixAlertQueue\n");
            }
            if (xQueueSend(xBuzzerAlertQueue, &alert_status, 0) != pdPASS) {
                 printf("DataProcessing: Failed to send to BuzzerAlertQueue\n");
            }
        }
    }
}

void vRgbLedAlertTask(void *pvParameters) {
    AlertStatus_t current_alert;
    printf("Task RgbLedAlert started.\n");
    while (true) {
        if (xQueueReceive(xRgbLedAlertQueue, &current_alert, portMAX_DELAY)) {
            if (current_alert.is_alert_active) {
                gpio_put(LED_RED_PIN, 1);
                gpio_put(LED_GREEN_PIN, 0);
                gpio_put(LED_BLUE_PIN, 0);
            } else {
                gpio_put(LED_RED_PIN, 0);
                gpio_put(LED_GREEN_PIN, 1);
                gpio_put(LED_BLUE_PIN, 0);
            }
        }
    }
}

void vLedMatrixAlertTask(void *pvParameters) {
    AlertStatus_t current_alert_status;
    // SensorData_t current_sensor_data; // Não é mais necessário aqui

    // Inicialize current_alert_status se desejar um estado padrão antes do primeiro receive
    memset(&current_alert_status, 0, sizeof(AlertStatus_t));
    current_alert_status.is_alert_active = false;
    current_alert_status.level = ALERT_NONE;

    printf("Task LedMatrixAlert started.\n");
    while (true) {
        // Não precisa mais do xQueuePeek(xSensorDataQueue, ...)

        if (xQueueReceive(xLedMatrixAlertQueue, &current_alert_status, portMAX_DELAY)) {
            if (current_alert_status.is_alert_active) {
                led_matrix_display_alert(
                    current_alert_status.level,
                    current_alert_status.water_level_percent, // Usa da struct de alerta recebida
                    current_alert_status.rain_volume_percent  // Usa da struct de alerta recebida
                );
            } else {
                led_matrix_display_normal_status();
            }
        }
    }
}

void vBuzzerAlertTask(void *pvParameters) {
    AlertStatus_t current_alert;
    bool buzzer_active = false; // Controla se o buzzer deve estar tocando intermitentemente

    // Inicialize current_alert se desejar um estado padrão antes do primeiro receive
    memset(&current_alert, 0, sizeof(AlertStatus_t));

    printf("Task BuzzerAlert started.\n");
    while (true) {
        // Tenta receber um novo status de alerta.
        // Usar um timeout aqui permite que a lógica de "buzzer_active" funcione mesmo sem novos dados.
        if (xQueueReceive(xBuzzerAlertQueue, &current_alert, pdMS_TO_TICKS(10))) {
            buzzer_active = current_alert.is_alert_active && (current_alert.level != ALERT_NONE);
            if (!buzzer_active) {
                 buzzer_play_tone(0, 0); // Desliga imediatamente se o alerta cessou
            }
             printf("BuzzerTask: Received alert. Active: %d, Level: %d\n", current_alert.is_alert_active, current_alert.level);
        }

        // Lógica de tocar o buzzer baseada no estado mais recente de 'buzzer_active'
        // e 'current_alert' (que foi atualizado pelo xQueueReceive)
        if (buzzer_active) {
            switch (current_alert.level) { // current_alert retém o último estado de alerta recebido
                case ALERT_WATER_HIGH:
                    buzzer_play_tone(BUZZER_ALERT_WATER_FREQ, BUZZER_ALERT_WATER_ON_MS);
                    vTaskDelay(pdMS_TO_TICKS(BUZZER_ALERT_WATER_OFF_MS + BUZZER_ALERT_WATER_ON_MS)); // Delay total do ciclo
                    break;
                case ALERT_RAIN_HIGH:
                    buzzer_play_tone(BUZZER_ALERT_RAIN_FREQ, BUZZER_ALERT_RAIN_ON_MS);
                    vTaskDelay(pdMS_TO_TICKS(BUZZER_ALERT_RAIN_OFF_MS + BUZZER_ALERT_RAIN_ON_MS));
                    break;
                case ALERT_BOTH_HIGH:
                    buzzer_play_tone(BUZZER_ALERT_BOTH_FREQ, BUZZER_ALERT_BOTH_ON_MS);
                    vTaskDelay(pdMS_TO_TICKS(BUZZER_ALERT_BOTH_OFF_MS + BUZZER_ALERT_BOTH_ON_MS));
                    break;
                default: // ALERT_NONE ou inesperado enquanto buzzer_active é true (não deveria acontecer)
                    buzzer_play_tone(0, 0); // Desliga
                    buzzer_active = false; // Corrige estado
                    vTaskDelay(pdMS_TO_TICKS(BUZZER_TASK_DELAY_MS)); // Delay padrão
                    break;
            }
        } else {
            // Se não há alerta ativo, garante que o buzzer esteja desligado e espera um pouco.
            buzzer_play_tone(0, 0);
            vTaskDelay(pdMS_TO_TICKS(BUZZER_TASK_DELAY_MS));
        }
    }
}


void vJoystickReadTask(void *pvParameters) {
    SensorData_t current_data;
    printf("Task JoystickRead started.\n");

    while (true) {
        current_data.water_level_raw = joystick_read_x();
        current_data.rain_volume_raw = joystick_read_y();

        // TODO: CALIBRAR ADC_MIN_VALUE e ADC_MAX_VALUE CORRETAMENTE!
        // Estes valores devem ser os limites REAIS do seu joystick para 0% e 100%
        // Exemplo: Se repouso (0%) = 1655 e máximo = 4000
        // ADC_MIN_VALUE_X = 1655; ADC_MAX_VALUE_X = 4000;
        // Se repouso (0%) = 0 e máximo = 4095 (ADC 12 bits completo)
        // ADC_MIN_VALUE = 0; ADC_MAX_VALUE = 4095;

        int32_t value_x = (int32_t)current_data.water_level_raw;
        int32_t value_y = (int32_t)current_data.rain_volume_raw;

        // Clamping para os valores definidos (importante se ADC_MIN/MAX não cobrem toda a faixa do ADC)
        if (value_x < ADC_MIN_VALUE) value_x = ADC_MIN_VALUE;
        if (value_x > ADC_MAX_VALUE) value_x = ADC_MAX_VALUE;

        if (value_y < ADC_MIN_VALUE) value_y = ADC_MIN_VALUE; // Assumindo mesma faixa para Y, ajuste se necessário
        if (value_y > ADC_MAX_VALUE) value_y = ADC_MAX_VALUE;

        // Conversão para porcentagem
        // Garante que o denominador não seja zero
        float percent_x = 0.0f;
        if ((ADC_MAX_VALUE - ADC_MIN_VALUE) != 0) {
            percent_x = ((float)(value_x - ADC_MIN_VALUE) / (ADC_MAX_VALUE - ADC_MIN_VALUE)) * 100.0f;
        }

        float percent_y = 0.0f;
        if ((ADC_MAX_VALUE - ADC_MIN_VALUE) != 0) { // Assumindo mesma faixa para Y
            percent_y = ((float)(value_y - ADC_MIN_VALUE) / (ADC_MAX_VALUE - ADC_MIN_VALUE)) * 100.0f;
        }
        
        current_data.water_level_percent = (uint8_t)percent_x;
        current_data.rain_volume_percent = (uint8_t)percent_y;

        // Limites de segurança (devem ser raramente atingidos se a calibração e o clamp acima estiverem corretos)
        if (current_data.water_level_percent > 100) current_data.water_level_percent = 100;
        if (percent_x < 0 && current_data.water_level_percent != 0) current_data.water_level_percent = 0; // Evita underflow de float negativo

        if (current_data.rain_volume_percent > 100) current_data.rain_volume_percent = 100;
        if (percent_y < 0 && current_data.rain_volume_percent != 0) current_data.rain_volume_percent = 0;

        // printf("Joystick: RawX=%d, PcntX=%d | RawY=%d, PcntY=%d\n",
        //        current_data.water_level_raw, current_data.water_level_percent,
        //        current_data.rain_volume_raw, current_data.rain_volume_percent);

        if (xQueueSend(xSensorDataQueue, &current_data, pdMS_TO_TICKS(10)) != pdPASS) {
            printf("JoystickRead: Failed to send to SensorDataQueue\n");
        }

        vTaskDelay(pdMS_TO_TICKS(JOYSTICK_READ_DELAY_MS));
    }
}


void vDisplayInfoTask(void *pvParameters) {
    ssd1306_t *ssd = (ssd1306_t *)pvParameters;
    AlertStatus_t current_alert_status;

    // Inicialize para um estado conhecido
    memset(&current_alert_status, 0, sizeof(AlertStatus_t));
    current_alert_status.level = ALERT_NONE;
    current_alert_status.is_alert_active = false;
    current_alert_status.water_level_percent = 0; // Começa mostrando 0%
    current_alert_status.rain_volume_percent = 0; // Começa mostrando 0%

    char line1[30];
    char line2[30];
    char line3[30];
    char line4[30];

    printf("Task DisplayInfo started.\n");
    if (!ssd) {
        printf("DisplayInfoTask: NULL display pointer!\n");
        vTaskDelete(NULL); // Encerra a task se o display não for válido
        return;
    }

    while (true) {
        // Tenta receber o status de alerta da sua fila dedicada.
        if (xQueueReceive(xDisplayAlertQueue, &current_alert_status, pdMS_TO_TICKS(DISPLAY_UPDATE_DELAY_MS / 2))) {
            printf("DisplayTask: AlertStatus RECEIVED! Agua: %u%%, Chuva: %u%%, Active: %d, Level: %d\n",
                   current_alert_status.water_level_percent,
                   current_alert_status.rain_volume_percent,
                   current_alert_status.is_alert_active,
                   current_alert_status.level);
        } else {
            // Timeout - nenhum novo dado de alerta recebido.
            // O display continuará mostrando o último estado válido de current_alert_status.
            printf("DisplayTask: DisplayAlertQueue Receive TIMEOUT.\n");
        }

        ssd1306_fill(ssd, false); // Limpa o display

        sprintf(line1, "Agua: %3u%%", current_alert_status.water_level_percent);
        ssd1306_draw_string(ssd, line1, 2, 0);

        sprintf(line2, "Chuva: %3u%%", current_alert_status.rain_volume_percent);
        ssd1306_draw_string(ssd, line2, 2, 15);

        if (current_alert_status.is_alert_active) {
            strcpy(line3, "!!! ALERTA !!!");
            switch(current_alert_status.level) {
                case ALERT_WATER_HIGH: sprintf(line4, "Nivel Agua Alto!"); break;
                case ALERT_RAIN_HIGH:  sprintf(line4, "Chuva Intensa!");   break;
                case ALERT_BOTH_HIGH:  sprintf(line4, "PERIGO MAXIMO!");   break;
                default:               sprintf(line4, "Alerta Ativo");     break;
            }
        } else {
            strcpy(line3, "Status: Normal");
            strcpy(line4, ""); // Linha vazia
        }

        ssd1306_draw_string(ssd, line3, 2, 30);
        ssd1306_draw_string(ssd, line4, 2, 45);

        ssd1306_send_data(ssd);
        vTaskDelay(pdMS_TO_TICKS(DISPLAY_UPDATE_DELAY_MS));
    }
}