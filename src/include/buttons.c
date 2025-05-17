#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "pico/stdlib.h"
#include "buttons.h"
#include "config.h"
#include "debouncer.h"
#include "pico/bootrom.h"

static volatile bool flag_button_a = false; //Flag volátil indicando se o botão A foi pressionado
//static volatile bool flag_button_b = false; //Flag volátil indicando se o botão B foi pressionado
static uint32_t last_press_time_a = 0;
static uint32_t last_press_time_b = 0;

 /**  * @brief Callback da interrupção dos pinos dos botões.
  *        Chamada quando ocorre uma borda de descida (GPIO_IRQ_EDGE_FALL).
  *        Verifica qual botão gerou a interrupção, aplica debounce e
  *        ativa a flag correspondente. O botão B também aciona o reset para bootloader USB.
  *
  * @param gpio O número do pino GPIO que gerou a interrupção.
  * @param events Máscara de bits indicando os eventos que ocorreram (ex: GPIO_IRQ_EDGE_FALL).
  */
 static void buttons_irq_callback(uint gpio, uint32_t events) {
     // Verifica se o evento foi uma borda de descida
     if (events & GPIO_IRQ_EDGE_FALL) {
         switch (gpio) {
             case BUTTON_A_PIN:
                 // Aplica debounce e ativa a flag do botão A se válido
                 if (check_debounce(&last_press_time_a, DEBOUNCE_TIME_US)) {
                     flag_button_a = true;
                 }
                 break;
             default:
                 // Ignora outros pinos, se houver
                 break;
         }
     }
 }
 
 /**
  * @brief Inicializa os pinos GPIO dos botões A e B.
  *        Configura os pinos como entrada com resistores de pull-up internos.
  *        Habilita as interrupções por borda de descida para ambos os botões,
  *        associando a função de callback `buttons_irq_callback`.
  */
 void buttons_init() {
     // Configura botão A
     gpio_init(BUTTON_A_PIN);
     gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
     gpio_pull_up(BUTTON_A_PIN);

 
     // Habilita interrupções por borda de descida para ambos os botões
     gpio_set_irq_enabled_with_callback(BUTTON_A_PIN, GPIO_IRQ_EDGE_FALL, true, &buttons_irq_callback);
     gpio_set_irq_enabled_with_callback(BUTTON_B_PIN, GPIO_IRQ_EDGE_FALL, true, &buttons_irq_callback);
 }
 
 /**
  * @brief Verifica se o botão A foi pressionado desde a última chamada.
  *        Lê a flag `flag_button_a` (que é ativada pela ISR) e a reseta
  *        se estiver ativa, para indicar que o pressionamento foi consumido.
  *
  * @return true Se o botão A foi pressionado e ainda não foi consumido.
  * @return false Caso contrário.
  */
 bool button_a_pressed() {
     if (flag_button_a) {
         flag_button_a = false; // Reseta a flag (consome o evento)
         return true;
     }
     return false;
 }

