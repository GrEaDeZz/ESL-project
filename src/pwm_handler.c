#include "pwm_handler.h"
#include "nrfx_pwm.h"
#include "nrf_pwm.h"
#include "app_timer.h"

#define PWM_TOP_VALUE       1000
#define BLINK_SLOW_MS       500
#define BLINK_FAST_MS       100

APP_TIMER_DEF(m_blink_timer);

static nrfx_pwm_t m_pwm_instance = NRFX_PWM_INSTANCE(0);
static nrf_pwm_values_individual_t m_seq_values;

static pwm_indicator_mode_t m_indicator_mode = PWM_INDICATOR_OFF;
static bool m_blink_state = false;

// Таймер мигания
static void blink_timer_handler(void *p_context)
{
    // Инвертируем состояние для мигания
    m_blink_state = !m_blink_state;
    m_seq_values.channel_0 = m_blink_state ? PWM_TOP_VALUE : 0;
}

// Инициализация ШИМ
void pwm_handler_init(const uint32_t *led_pins)
{
    nrfx_pwm_config_t config = NRFX_PWM_DEFAULT_CONFIG;
    
    // Настройка пинов
    for (int i = 0; i < 4; i++) {
        config.output_pins[i] = (led_pins[i] != NRF_PWM_PIN_NOT_CONNECTED) ? led_pins[i] : NRFX_PWM_PIN_NOT_USED;
    }

    config.top_value = PWM_TOP_VALUE;
    config.load_mode = NRF_PWM_LOAD_INDIVIDUAL; 
    config.step_mode = NRF_PWM_STEP_AUTO;

    nrfx_pwm_init(&m_pwm_instance, &config, NULL);

    // Сброс значений каналов
    m_seq_values.channel_0 = 0;
    m_seq_values.channel_1 = 0;
    m_seq_values.channel_2 = 0;
    m_seq_values.channel_3 = 0;

    app_timer_create(&m_blink_timer, APP_TIMER_MODE_REPEATED, blink_timer_handler);
    
    nrf_pwm_sequence_t seq;
    seq.values.p_individual = &m_seq_values;
    seq.length              = 4;
    seq.repeats             = 0;
    seq.end_delay           = 0;
    // Запуск циклического воспроизведения
    nrfx_pwm_simple_playback(&m_pwm_instance, &seq, 1, NRFX_PWM_FLAG_LOOP);
}

// Установка RGB
void pwm_handler_set_rgb(uint16_t r, uint16_t g, uint16_t b)
{
    m_seq_values.channel_1 = (r > PWM_TOP_VALUE) ? PWM_TOP_VALUE : r;
    m_seq_values.channel_2 = (g > PWM_TOP_VALUE) ? PWM_TOP_VALUE : g;
    m_seq_values.channel_3 = (b > PWM_TOP_VALUE) ? PWM_TOP_VALUE : b;
}

// Устанавливает режим работы индикатора (мигание/постоянный)
void pwm_handler_set_indicator_mode(pwm_indicator_mode_t mode)
{
    m_indicator_mode = mode;
    app_timer_stop(m_blink_timer);

    switch (mode)
    {
        case PWM_INDICATOR_OFF:
            m_seq_values.channel_0 = 0;
            break;
        case PWM_INDICATOR_ON:
            m_seq_values.channel_0 = PWM_TOP_VALUE;
            break;
        case PWM_INDICATOR_BLINK_SLOW:
            app_timer_start(m_blink_timer, APP_TIMER_TICKS(BLINK_SLOW_MS), NULL);
            break;
        case PWM_INDICATOR_BLINK_FAST:
            app_timer_start(m_blink_timer, APP_TIMER_TICKS(BLINK_FAST_MS), NULL);
            break;
    }
}