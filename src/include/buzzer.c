#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "pico/stdlib.h"
#include "buzzer.h"
#include "config.h"

/**
 * @brief Inicializa o pino GPIO conectado ao buzzer como saída.
 *        Opcionalmente, pode tocar uma melodia de inicialização.
 */
void buzzer_init() {
    gpio_init(BUZZER_PIN_MAIN);
    gpio_set_dir(BUZZER_PIN_MAIN, GPIO_OUT);
    gpio_put(BUZZER_PIN_MAIN, 0);
}

/**
 * @brief Toca um único tom no buzzer com a frequência e duração especificadas.
 *        Configura o PWM para gerar o tom. Se a duração for maior que 0,
 *        bloqueia a execução durante esse tempo (usar vTaskDelay em contexto FreeRTOS).
 *        Se freq for 0, apenas pausa pela duração_ms (silêncio).
 *
 * @param freq Frequência do tom em Hz (0 para silêncio).
 * @param duration_ms Duração do tom em milissegundos (0 para não bloquear,
 *                    apenas configurar o PWM e retornar).
 */
static void play_tone_internal(uint freq, uint duration_ms) {
    // Se a frequência for 0, representa silêncio.
    if (freq == 0) {
        // Se houver duração, pausa (atenção: sleep_ms bloqueia!)
        if (duration_ms > 0) {
            // IMPORTANTE: Se esta função for chamada de uma tarefa FreeRTOS,
            // substituir sleep_ms por vTaskDelay(pdMS_TO_TICKS(duration_ms));
            sleep_ms(duration_ms);
        }
        // Desliga o PWM explicitamente se estava ligado antes
        uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN_MAIN);
        pwm_set_enabled(slice_num, false);
        return; // Retorna após o silêncio
    }

    // Configura o pino GPIO para usar a função PWM.
    gpio_set_function(BUZZER_PIN_MAIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN_MAIN);
    uint channel = pwm_gpio_to_channel(BUZZER_PIN_MAIN);

    // Calcula os parâmetros do PWM (divisor de clock e valor de wrap)
    // para gerar a frequência desejada.
    uint32_t clock = clock_get_hz(clk_sys);
    uint32_t divider16 = clock * 16 / freq; // Cálculo inicial do divisor (vezes 16)
    uint32_t wrap_val = 65535; // Valor máximo de wrap inicial
    uint clk_div = 1; // Divisor de clock inicial

    // Ajusta o divisor de clock se necessário para caber no wrap_val
    while (divider16 >= 16 * wrap_val && clk_div < 256) {
        clk_div++;
        divider16 = clock * 16 / (freq * clk_div);
    }
    if (divider16 < 16) divider16 = 16; // Garante valor mínimo
    wrap_val = divider16 / 16; // Calcula o valor final de wrap

    // Configura o divisor de clock e o valor de wrap no hardware PWM.
    pwm_set_clkdiv_int_frac(slice_num, clk_div, 0);
    pwm_set_wrap(slice_num, wrap_val);
    // Define o nível do canal para 50% do valor de wrap (duty cycle de 50%).
    pwm_set_chan_level(slice_num, channel, wrap_val / 2);
    // Habilita o PWM.
    pwm_set_enabled(slice_num, true);

    // Se uma duração foi especificada, aguarda esse tempo.
    if (duration_ms > 0) {
        // IMPORTANTE: Substituir por vTaskDelay se em contexto FreeRTOS.
        sleep_ms(duration_ms);
        // Desabilita o PWM após a duração.
        pwm_set_enabled(slice_num, false);
    }
    // Se duration_ms for 0, o PWM permanece habilitado e a função retorna.
    // O chamador é responsável por desabilitá-lo depois, se necessário.
}

/**
 * @brief Toca um único tom no buzzer.
 *
 * @param freq Frequência do tom em Hz (0 para desligar o PWM).
 * @param duration_ms Duração em milissegundos.
 */
void buzzer_play_tone(uint freq, uint duration_ms) {
    // Chama a função interna que lida com a lógica do PWM.
    // A função interna já trata freq=0 e duration_ms=0/ >0.
    play_tone_internal(freq, duration_ms);
}