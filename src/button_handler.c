#include "button_handler.h"
#include "app_logic.h"
#include "nrfx_gpiote.h"
#include "app_timer.h"
#include "nrf_gpio.h"

// Настройки таймингов
#define DEBOUNCE_MS         50
#define DOUBLE_CLICK_MS     400

APP_TIMER_DEF(m_debounce_timer);
APP_TIMER_DEF(m_double_click_timer);

static uint32_t m_button_pin;
static bool     m_wait_for_double_click = false;

// Сброс флага двойного клика
static void double_click_timer_handler(void * p_context)
{
    m_wait_for_double_click = false;
}

// Обработка нажатия после антидребезга
static void debounce_timer_handler(void * p_context)
{
    if (!nrfx_gpiote_is_init()) return;

    // Читаем состояние (0 = нажата)
    bool is_pressed = !nrfx_gpiote_in_is_set(m_button_pin);

    if (is_pressed)
    {
        // Сообщаем логике о нажатии
        app_logic_on_button_event(BUTTON_EVENT_PRESSED);

        if (m_wait_for_double_click)
        {
            // Второе нажатие -> двойной клик
            app_logic_on_button_event(BUTTON_EVENT_DOUBLE_CLICK);
            
            m_wait_for_double_click = false;
            app_timer_stop(m_double_click_timer);
        }
        else
        {
            // Первое нажатие -> ждем второе
            m_wait_for_double_click = true;
            app_timer_start(m_double_click_timer, APP_TIMER_TICKS(DOUBLE_CLICK_MS), NULL);
        }
    }
    else
    {
        // Сообщаем логике об отпускании
        app_logic_on_button_event(BUTTON_EVENT_RELEASED);
    }
}

// Обработчик прерывания GPIO
static void button_gpiote_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    // Запускаем таймер антидребезга
    app_timer_start(m_debounce_timer, APP_TIMER_TICKS(DEBOUNCE_MS), NULL);
}

// Инициализация кнопки
void button_handler_init(uint32_t button_pin)
{
    m_button_pin = button_pin;

    if (!nrfx_gpiote_is_init()) {
        nrfx_gpiote_init();
    }

    // Настройка на любой фронт
    nrfx_gpiote_in_config_t in_config = NRFX_GPIOTE_CONFIG_IN_SENSE_TOGGLE(true);
    in_config.pull = NRF_GPIO_PIN_PULLUP;
    
    nrfx_gpiote_in_init(m_button_pin, &in_config, button_gpiote_handler);
    nrfx_gpiote_in_event_enable(m_button_pin, true);

    // Создаем таймеры
    app_timer_create(&m_debounce_timer, APP_TIMER_MODE_SINGLE_SHOT, debounce_timer_handler);
    app_timer_create(&m_double_click_timer, APP_TIMER_MODE_SINGLE_SHOT, double_click_timer_handler);
}