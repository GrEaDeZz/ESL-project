#include "app_timer.h"
#include "nrf_drv_clock.h"
#include "nrf_drv_power.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_log_backend_usb.h"

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

static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}

int main(void)
{
    ret_code_t err_code;

    err_code = nrf_drv_clock_init();
    APP_ERROR_CHECK(err_code);
    
    nrf_drv_clock_lfclk_request(NULL);

    while (!nrf_drv_clock_lfclk_is_running());

    err_code = nrf_drv_power_init(NULL);
    APP_ERROR_CHECK(err_code);

    log_init();
    app_timer_init();
    button_handler_init(BUTTON_1_PIN);
    pwm_handler_init(led_pins, id_digits);

    while (1)
    {
        LOG_BACKEND_USB_PROCESS();

        if (NRF_LOG_PROCESS() == false)
        {
            __WFE();
        }
    }
}