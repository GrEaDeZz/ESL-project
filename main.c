#include <stdbool.h>
#include "nrf_delay.h"
#include "nrf_gpio.h"

#define LED_1_Y_PIN     6
#define LED_2_R_PIN     8
#define LED_2_G_PIN     41
#define LED_2_B_PIN     12
#define BUTTON_1_PIN    38

#define BLINK_ON_MS     250
#define BLINK_OFF_MS    250

static const int id_digits[4] = { 6, 6, 0, 6 };

static const uint32_t led_pins[4] = {
    LED_1_Y_PIN,
    LED_2_R_PIN,
    LED_2_G_PIN,
    LED_2_B_PIN
};

static int current_index = 0;   // индекс активного светодиода
static int current_blink = 0;   // количество завершённых миганий текущего светодиода

void leds_init(void)
{
    nrf_gpio_cfg_output(LED_1_Y_PIN);
    nrf_gpio_cfg_output(LED_2_R_PIN);
    nrf_gpio_cfg_output(LED_2_G_PIN);
    nrf_gpio_cfg_output(LED_2_B_PIN);

    nrf_gpio_pin_set(LED_1_Y_PIN);
    nrf_gpio_pin_set(LED_2_R_PIN);
    nrf_gpio_pin_set(LED_2_G_PIN);
    nrf_gpio_pin_set(LED_2_B_PIN);
}

void button_init(void)
{
    nrf_gpio_cfg_input(BUTTON_1_PIN, NRF_GPIO_PIN_PULLUP);
}

void led_on(uint32_t pin)
{
    nrf_gpio_pin_clear(pin);
}

void led_off(uint32_t pin)
{
    nrf_gpio_pin_set(pin);
}

int main(void)
{
    leds_init();
    button_init();

    while (true)
    {
        if (nrf_gpio_pin_read(BUTTON_1_PIN) == 0)
        {
            uint32_t pin = led_pins[current_index];
            int total_blinks = id_digits[current_index];

            while (current_blink < total_blinks)
            {
                if (nrf_gpio_pin_read(BUTTON_1_PIN) != 0)
                    break;

                led_on(pin);
                nrf_delay_ms(BLINK_ON_MS);

                if (nrf_gpio_pin_read(BUTTON_1_PIN) != 0)
                    break;

                led_off(pin);
                nrf_delay_ms(BLINK_OFF_MS);

                current_blink++;
            }

            if (current_blink >= total_blinks)
            {
                current_blink = 0;
                current_index = (current_index + 1) % 4;
            }
        }
        else
        {
            // Кнопка не нажата — делаем другую работу
        }
    }
}