#include "button_handler.h"
#include "nrfx_gpiote.h"
#include "app_timer.h"
#include "nrf_gpio.h"
#include "pwm_handler.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_log_backend_usb.h"

#define DEBOUNCE_MS         200
#define DOUBLE_CLICK_MS     600

APP_TIMER_DEF(debounce_timer);
APP_TIMER_DEF(double_click_timer);

static uint32_t m_button_pin;
static bool m_first_click_detected = false;
static bool m_button_blocked = false;

static void debounce_timer_handler(void * p_context)
{
    m_button_blocked = false;
}

static void double_click_timer_handler(void * p_context)
{
    m_first_click_detected = false;
}

static void button_gpiote_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    // Если дребезг — выходим
    if (m_button_blocked)
        return;

    // Запуск таймера дребезга
    m_button_blocked = true;
    app_timer_start(debounce_timer, APP_TIMER_TICKS(DEBOUNCE_MS), NULL);

    // Логика двойного клика
    if (!m_first_click_detected)
    {
        NRF_LOG_INFO("Обнаружено одно нажатие.");
        m_first_click_detected = true;
        app_timer_start(double_click_timer, APP_TIMER_TICKS(DOUBLE_CLICK_MS), NULL);
    }
    else
    {
        NRF_LOG_INFO("Обнаружено двойное нажатие.");
        m_first_click_detected = false;
        app_timer_stop(double_click_timer);
        pwm_handler_toggle_playback();
    }
}

void button_handler_init(uint32_t button_pin)
{
    m_button_pin = button_pin;

    if (!nrfx_gpiote_is_init()) {
        nrfx_gpiote_init();
    }

    nrfx_gpiote_in_config_t in_config = NRFX_GPIOTE_CONFIG_IN_SENSE_HITOLO(true);
    in_config.pull = NRF_GPIO_PIN_PULLUP;
    nrfx_gpiote_in_init(m_button_pin, &in_config, button_gpiote_handler);
    nrfx_gpiote_in_event_enable(m_button_pin, true);

    app_timer_create(&debounce_timer, APP_TIMER_MODE_SINGLE_SHOT, debounce_timer_handler);
    app_timer_create(&double_click_timer, APP_TIMER_MODE_SINGLE_SHOT, double_click_timer_handler);
}