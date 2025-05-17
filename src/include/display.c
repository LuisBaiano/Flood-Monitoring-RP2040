#include "display.h"
#include "config.h"
#include <string.h>
#include <stdio.h>
#include "pico/stdlib.h"


/**
  * @brief Inicializa a comunicação I2C e o display OLED SSD1306.
  *        Configura os pinos SDA e SCL, inicializa o periférico I2C e
  *        envia os comandos de configuração para o display.
  *
  * @param ssd Ponteiro para a estrutura de controle do display SSD1306 a ser inicializada.
  */
 void display_init(ssd1306_t *ssd) {
     // Inicializa I2C na porta e velocidade definidas
     i2c_init(I2C_PORT, 400 * 1000);
     // Configura os pinos GPIO para a função I2C
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
     // Habilita resistores de pull-up internos para os pinos I2C
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
     // Inicializa a estrutura do driver SSD1306 com os parâmetros do display
    ssd1306_init(ssd, WIDTH, HEIGHT, false, DISPLAY_ADDR, I2C_PORT);
     // Envia a sequência de comandos de configuração para o display
    ssd1306_config(ssd);
    ssd1306_fill(ssd, false);
    ssd1306_send_data(ssd);
    printf("Display inicializado.\n");
}

/**
  * @brief Exibe uma tela de inicialização no display OLED.
  *        Mostra um texto de título e o ícone do semáforo por alguns segundos.
  *
  * @param ssd Ponteiro para a estrutura de controle do display SSD1306.
  */
 void display_startup_screen(ssd1306_t *ssd) {
     // Limpa o display
    ssd1306_fill(ssd, false);
     // Calcula posições aproximadas para centralizar o texto
    uint8_t center_x_approx = ssd->width / 2;
    uint8_t start_y = 8;
     uint8_t line_height = 10; // Espaçamento vertical entre linhas

     // Define as strings a serem exibidas
     const char *line1 = "EMBARCATECH";
     const char *line2 = "PROJETO";
     const char *line3 = "MONITORAMENTO RTOS";
    ssd1306_draw_string(ssd, line1, center_x_approx - (strlen(line1)*8)/2, start_y);
    ssd1306_draw_string(ssd, line2, center_x_approx - (strlen(line2)*8)/2, start_y + line_height);
    ssd1306_draw_string(ssd, line3, center_x_approx - (strlen(line3)*8)/2, start_y + 2*line_height);
    ssd1306_send_data(ssd);
    // Mantém a tela visível por um tempo
    sleep_ms(2500);
    // Limpa o display após a tela de inicialização
    ssd1306_fill(ssd, false);
    ssd1306_send_data(ssd);
}
