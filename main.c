#include <stdbool.h>
#include <stdint.h>
#include "nrf_delay.h"
#include "boards.h"

// Настройки мигания
#define BLINK_ON_MS       250  // Как долго светодиод горит
#define BLINK_OFF_MS      250  // Пауза между миганиями
#define PAUSE_DIGIT_MS    500  // Пауза после завершения мигания одного светодиода
#define PAUSE_LOOP_MS     1500 // Пауза в конце всего цикла

// ID платы: 6606
static const int id_digits[LEDS_NUMBER] = {6, 6, 0, 6};


void blink_led(uint8_t led_index, int blink_count)
{

    for (int i = 0; i < blink_count; i++)
    {
        bsp_board_led_on(led_index);
        nrf_delay_ms(BLINK_ON_MS);
        
        bsp_board_led_off(led_index);
        nrf_delay_ms(BLINK_OFF_MS);
    }
}

int main(void)
{

    bsp_board_init(BSP_INIT_LEDS);

    bsp_board_leds_off();

    while (true)
    {
        for (int i = 0; i < LEDS_NUMBER; i++)
        {
            int num_blinks  = id_digits[i];

            blink_led(i, num_blinks);

            nrf_delay_ms(PAUSE_DIGIT_MS);
        }
        nrf_delay_ms(PAUSE_LOOP_MS);
    }
}