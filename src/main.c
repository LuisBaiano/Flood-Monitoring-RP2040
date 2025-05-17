#include "config.h"
#include <stdio.h>
#include <string.h> 
#include "display.h"
#include "led_matrix.h" 

// --- Global Variables & Queues ---
ssd1306_t display_oled;

QueueHandle_t xSensorDataQueue;
QueueHandle_t xAlertStatusQueue;

// --- Task Forward Declarations ---
void vJoystickReadTask(void *pvParameters);
void vDataProcessingTask(void *pvParameters);
void vRgbLedAlertTask(void *pvParameters);
void vLedMatrixAlertTask(void *pvParameters);
void vBuzzerAlertTask(void *pvParameters);
void vDisplayInfoTask(void *pvParameters);
// void vButtonMonitorTask(void *pvParameters);

// --- System Initialization ---
void init_system_flood_alert() {
    stdio_init_all();
    sleep_ms(1000);
    printf("Initializing Flood Alert System...\n");

    joystick_init();
    buttons_init();
    buzzer_init();
    led_matrix_init(); // Certifique-se que led_matrix.h está incluído via hardware_config.h

    gpio_init(LED_RED_PIN); gpio_set_dir(LED_RED_PIN, GPIO_OUT); gpio_put(LED_RED_PIN, 0);
    gpio_init(LED_GREEN_PIN); gpio_set_dir(LED_GREEN_PIN, GPIO_OUT); gpio_put(LED_GREEN_PIN, 1);
    gpio_init(LED_BLUE_PIN); gpio_set_dir(LED_BLUE_PIN, GPIO_OUT); gpio_put(LED_BLUE_PIN, 0);

    // display_init() deve estar em display.c e display.h incluído via hardware_config.h
    display_init(&display_oled);

    printf("System Hardware Initialized.\n");
}

// --- Main Function ---
int main() {
    init_system_flood_alert();
    display_startup_screen(&display_oled);
    printf("Startup screen finished.\n");

    xSensorDataQueue = xQueueCreate(5, sizeof(SensorData_t));
    xAlertStatusQueue = xQueueCreate(3, sizeof(AlertStatus_t));

    if (xSensorDataQueue == NULL || xAlertStatusQueue == NULL) {
        printf("FATAL: Failed to create queues!\n");
        while(1);
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

    printf("Scheduler returned. Critical Error!\n");
    while(1);
}

// --- Task Implementations ---

void vJoystickReadTask(void *pvParameters) {
    SensorData_t current_data; // Variável local para os dados do joystick
    printf("Tarefa do joystick\n");
    while (true) {
        current_data.water_level_raw = joystick_read_x();
        current_data.rain_volume_raw = joystick_read_y();

        current_data.nivel_agua = joystick_adc_to_percent(current_data.water_level_raw);
        current_data.volume_chuva = joystick_adc_to_percent(current_data.rain_volume_raw);

    }
}

//tarefa pra processar os dados
void vDataProcessingTask(void *pvParameters) {
    SensorData_t dados_recebidos;
    AlertStatus_t status_alerta;
    status_alerta.is_alert_active = false;
    status_alerta.level = ALERT_NONE;

    printf("Tarefa que muda os estados\n");
    while (true) {
        if (xQueueReceive(xSensorDataQueue, &dados_recebidos, portMAX_DELAY)) {
            status_alerta.nivel_agua = dados_recebidos.nivel_agua;
            status_alerta.volume_chuva = dados_recebidos.volume_chuva;

            bool water_alert = (dados_recebidos.nivel_agua >= WATER_LEVEL_ALERT_THRESHOLD);
            bool rain_alert = (dados_recebidos.volume_chuva >= RAIN_VOLUME_ALERT_THRESHOLD);

            if (water_alert && rain_alert) {
                status_alerta.level = ALERT_BOTH_HIGH;
                status_alerta.is_alert_active = true;
            } else if (water_alert) {
                status_alerta.level = ALERT_WATER_HIGH;
                status_alerta.is_alert_active = true;
            } else if (rain_alert) {
                status_alerta.level = ALERT_RAIN_HIGH;
                status_alerta.is_alert_active = true;
            } else {
                status_alerta.level = ALERT_NONE;
                status_alerta.is_alert_active = false;
            }
            //aqui ta funcionando normal

        }
    }
}

void vRgbLedAlertTask(void *pvParameters) {
    AlertStatus_t alerta_atual; // Variável local para o status de alerta
    printf("Tarefa do rgb.\n");
    while (true) {
        if (xQueueReceive(xAlertStatusQueue, &alerta_atual, portMAX_DELAY)) {
            if (alerta_atual.is_alert_active) {
                gpio_put(LED_RED_PIN, 1);
                gpio_put(LED_GREEN_PIN, 0);
                gpio_put(LED_BLUE_PIN, 0);
            } else {
                gpio_put(LED_RED_PIN, 0);
                gpio_put(LED_GREEN_PIN, 1);
                gpio_put(LED_BLUE_PIN, 0);
            }
        }
        // talvez adicionar o delay
        // vTaskDelay(pdMS_TO_TICKS(RGB_LED_TASK_DELAY_MS));
    }
}

void vLedMatrixAlertTask(void *pvParameters) {
    AlertStatus_t current_status_alerta; // Variável local
    SensorData_t current_sensor_data;  // Variável local

    memset(&current_sensor_data, 0, sizeof(SensorData_t)); // Inicializa
    current_status_alerta.is_alert_active = false;
    current_status_alerta.level = ALERT_NONE;

    printf("tarefa da matriz de leds.\n");
    while (true) {
        xQueuePeek(xSensorDataQueue, &current_sensor_data, 0);
        if (xQueueReceive(xAlertStatusQueue, &current_status_alerta, portMAX_DELAY)) {
            if (current_status_alerta.is_alert_active) {
                led_matrix_display_alert(
                    current_status_alerta.level,
                    current_sensor_data.nivel_agua,
                    current_sensor_data.volume_chuva
                );
            } else {
                led_matrix_display_normal_status();
            }
        }
        // vTaskDelay(pdMS_TO_TICKS(MATRIX_TASK_DELAY_MS));
    }
}

void vBuzzerAlertTask(void *pvParameters) {
    AlertStatus_t alerta_atual;
    bool buzzer_active = false;

    printf("Task BuzzerAlert started.\n");
    while (true) {
        // Usar alerta_atual
        if (xQueueReceive(xAlertStatusQueue, &alerta_atual, pdMS_TO_TICKS(10))) {
            buzzer_active = alerta_atual.is_alert_active && (alerta_atual.level != ALERT_NONE);
        }

        if (buzzer_active) {
            switch (alerta_atual.level) {
                case ALERT_WATER_HIGH:
                    buzzer_play_tone(BUZZER_ALERT_WATER_FREQ, BUZZER_ALERT_WATER_ON_MS);
                    vTaskDelay(pdMS_TO_TICKS(BUZZER_ALERT_WATER_OFF_MS));
                    break;
                case ALERT_RAIN_HIGH:
                    buzzer_play_tone(BUZZER_ALERT_RAIN_FREQ, BUZZER_ALERT_RAIN_ON_MS);
                    vTaskDelay(pdMS_TO_TICKS(BUZZER_ALERT_RAIN_OFF_MS));
                    break;
                case ALERT_BOTH_HIGH:
                    buzzer_play_tone(BUZZER_ALERT_BOTH_FREQ, BUZZER_ALERT_BOTH_ON_MS);
                    vTaskDelay(pdMS_TO_TICKS(BUZZER_ALERT_BOTH_OFF_MS));
                    break;
                default:
                    buzzer_play_tone(0, 0);
                    vTaskDelay(pdMS_TO_TICKS(BUZZER_TASK_DELAY_MS));
                    break;
            }
        } else {
            buzzer_play_tone(0, 0);
            vTaskDelay(pdMS_TO_TICKS(BUZZER_TASK_DELAY_MS));
        }
    }
}


void vDisplayInfoTask(void *pvParameters) {
    ssd1306_t *ssd = (ssd1306_t *)pvParameters;
    SensorData_t sensor_data_for_display; // Dados lidos diretamente por esta tarefa
    AlertStatus_t status_alerta_for_display = {ALERT_NONE, 0, 0, false}; // Último status de alerta recebido
    char line1[30];
    char line2[30];
    char line3[30];

    printf("Task DisplayInfo started (ADC Direct Read Mode).\n");
    if (!ssd) {
        printf("DisplayInfoTask: NULL display pointer!\n");
        vTaskDelete(NULL);
        return;
    }

    while (true) {
        // 1. LER DADOS DO JOYSTICK DIRETAMENTE (como em vJoystickReadTask)
        sensor_data_for_display.water_level_raw = joystick_read_x();
        sensor_data_for_display.rain_volume_raw = joystick_read_y();
        sensor_data_for_display.nivel_agua = joystick_adc_to_percent(sensor_data_for_display.water_level_raw);
        sensor_data_for_display.volume_chuva = joystick_adc_to_percent(sensor_data_for_display.rain_volume_raw);


        if (xQueueReceive(xAlertStatusQueue, &status_alerta_for_display, pdMS_TO_TICKS(10)) != pdPASS) {

            xQueuePeek(xAlertStatusQueue, &status_alerta_for_display, 0); // Pega o último sem remover
        }


        // ----- DEBUG PRINT DENTRO DA TAREFA DE DISPLAY -----
        printf("DisplayTask - DirectRead Sensor: Agua:%u%%, Chuva:%u%% | Rcvd Alert Active: %d Level: %d\n",
               sensor_data_for_display.nivel_agua,
               sensor_data_for_display.volume_chuva,
               status_alerta_for_display.is_alert_active,
               status_alerta_for_display.level);
        // ----- FIM DEBUG PRINT -----

        ssd1306_fill(ssd, false);

        sprintf(line1, "Agua:%3u%%     Chuva:%3u%%",
                sensor_data_for_display.nivel_agua,
                sensor_data_for_display.volume_chuva);
        ssd1306_draw_string(ssd, line1, 1, 5);

        //não está mudando, corrigir depois
        if (status_alerta_for_display.is_alert_active) {
            strcpy(line2, "!!! ALERTA !!!");
            switch(status_alerta_for_display.level) {
                case ALERT_WATER_HIGH: sprintf(line3, "Nivel Agua Alto!"); break;
                case ALERT_RAIN_HIGH:  sprintf(line3, "Chuva Intensa!");   break;
                case ALERT_BOTH_HIGH:  sprintf(line3, "PERIGO MAXIMO!");   break;
                default:               sprintf(line3, "Alerta Ativo");    break;
            }
            ssd1306_draw_string(ssd, line2, (ssd->width / 2) - (strlen(line2) * 8 / 2), 20);
            ssd1306_draw_string(ssd, line3, (ssd->width / 2) - (strlen(line3) * 8 / 2), 35);
        } else {
            strcpy(line2, "Status: Normal");
            ssd1306_draw_string(ssd, line2, (ssd->width / 2) - (strlen(line2) * 8 / 2), 25);
        }

        ssd1306_send_data(ssd);
        vTaskDelay(pdMS_TO_TICKS(DISPLAY_UPDATE_DELAY_MS)); // Controla a taxa de atualização do display
    }
}
