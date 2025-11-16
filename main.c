#include "app_timer.h"
#include "nrfx_clock.h"

#include "button_handler.h"
#include "pwm_handler.h"

#define LED_1_Y_PIN     6
#define LED_2_R_PIN     8
#define LED_2_G_PIN     41
#define LED_2_B_PIN     12
#define BUTTON_1_PIN    38

static const int id_digits[4] = { 6, 6, 0, 6 };

static const uint32_t led_pins[4] = {
    LED_1_Y_PIN,
    LED_2_R_PIN,
    LED_2_G_PIN,
    LED_2_B_PIN
};

int main(void)
{
    nrfx_clock_init(NULL);
    nrfx_clock_lfclk_start();

    while (!nrfx_clock_lfclk_is_running());

    app_timer_init();

    button_handler_init(BUTTON_1_PIN);
    pwm_handler_init(led_pins, id_digits);

    while (1)
    {        
        __WFE();
    }
}