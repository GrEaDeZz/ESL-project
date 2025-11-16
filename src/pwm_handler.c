#include "pwm_handler.h"
#include "nrfx_pwm.h"
#include "nrf_pwm.h"
#include "app_timer.h"

#define PWM_CHANNELS       4       // Кол-во каналов
#define PWM_TOP_VALUE      1000    // Максимальное значение ШИМ
#define FADE_INTERVAL_MS   5       // Интервал обновления яркости (мс)
#define FADE_STEP          10      // Шаг изменения яркости

APP_TIMER_DEF(m_pwm_timer);

static nrfx_pwm_t m_pwm_instance = NRFX_PWM_INSTANCE(0);

static nrf_pwm_values_individual_t m_seq_values;

static int      m_led_counts[PWM_CHANNELS];
static uint32_t m_num_leds = PWM_CHANNELS;

static volatile bool m_running             = false;
static bool          m_fading_in           = true;
static uint16_t      m_current_duty_cycle  = 0;
static int           m_current_led_index   = 0;
static int           m_current_blink_count = 0;



static void pwm_timer_handler(void *p_context)
{
    (void)p_context;

    if (!m_running)
        return; 

    if (m_fading_in)
    {
        m_current_duty_cycle += FADE_STEP;
        if (m_current_duty_cycle >= PWM_TOP_VALUE)
        {
            m_current_duty_cycle = PWM_TOP_VALUE;
            m_fading_in = false;
        }
    }
    else
    {
        if (m_current_duty_cycle <= FADE_STEP)
        {
            m_current_duty_cycle = 0;
            m_fading_in = true;

            m_current_blink_count++;

            if (m_current_blink_count >= m_led_counts[m_current_led_index])
            {
                m_current_blink_count = 0;

                do {
                    m_current_led_index = (m_current_led_index + 1) % m_num_leds;
                } while (m_led_counts[m_current_led_index] == 0);
            }
        }
        else
        {
            m_current_duty_cycle -= FADE_STEP;
        }
    }

    m_seq_values.channel_0 = 0;
    m_seq_values.channel_1 = 0;
    m_seq_values.channel_2 = 0;
    m_seq_values.channel_3 = 0;

    uint16_t active_value = m_current_duty_cycle;
    switch (m_current_led_index)
    {
        case 0: m_seq_values.channel_0 = active_value; break;
        case 1: m_seq_values.channel_1 = active_value; break;
        case 2: m_seq_values.channel_2 = active_value; break;
        case 3: m_seq_values.channel_3 = active_value; break;
    }

    nrf_pwm_sequence_t seq;
    seq.values.p_individual = &m_seq_values; 
    seq.length              = PWM_CHANNELS;
    seq.repeats             = 0;
    seq.end_delay           = 0;

    nrfx_pwm_simple_playback(&m_pwm_instance, &seq, 1, 0);
}

void pwm_handler_init(const uint32_t *led_pins, const int *id_digits)
{
    nrfx_pwm_config_t config = NRFX_PWM_DEFAULT_CONFIG;

    for (int i = 0; i < PWM_CHANNELS; ++i)
    {
        config.output_pins[i] = (led_pins && led_pins[i] != (uint32_t)-1) ? led_pins[i] : NRFX_PWM_PIN_NOT_USED;
    }

    config.base_clock = NRF_PWM_CLK_1MHz;
    config.count_mode = NRF_PWM_MODE_UP;
    config.top_value  = PWM_TOP_VALUE;
    config.load_mode  = NRF_PWM_LOAD_INDIVIDUAL;
    config.step_mode  = NRF_PWM_STEP_AUTO;

    if (nrfx_pwm_init(&m_pwm_instance, &config, NULL) != NRFX_SUCCESS)
        return;

    for (int i = 0; i < m_num_leds; i++)
        m_led_counts[i] = id_digits ? id_digits[i] : 0;

    m_current_led_index = 0;
    while (m_current_led_index < (int)m_num_leds && m_led_counts[m_current_led_index] == 0)
        m_current_led_index++;

    app_timer_create(&m_pwm_timer, APP_TIMER_MODE_REPEATED, pwm_timer_handler);

    m_seq_values.channel_0 = 0;
    m_seq_values.channel_1 = 0;
    m_seq_values.channel_2 = 0;
    m_seq_values.channel_3 = 0;
    
    nrf_pwm_sequence_t seq;
    seq.values.p_individual = &m_seq_values;
    seq.length              = PWM_CHANNELS;
    seq.repeats             = 0;
    seq.end_delay           = 0;
    
    nrfx_pwm_simple_playback(&m_pwm_instance, &seq, 1, 0);
}

void pwm_handler_toggle_playback(void)
{
    m_running = !m_running;

    if (m_running)
    {
        m_current_duty_cycle = 0;
        m_fading_in = true;
        m_current_blink_count = 0;

        m_current_led_index = 0;
        while (m_current_led_index < (int)m_num_leds && m_led_counts[m_current_led_index] == 0)
            ++m_current_led_index;
            
        app_timer_start(m_pwm_timer, APP_TIMER_TICKS(FADE_INTERVAL_MS), NULL);
    }
    else
    {
        app_timer_stop(m_pwm_timer);

        m_seq_values.channel_0 = 0;
        m_seq_values.channel_1 = 0;
        m_seq_values.channel_2 = 0;
        m_seq_values.channel_3 = 0;

        nrf_pwm_sequence_t seq;
        seq.values.p_individual = &m_seq_values;
        seq.length              = PWM_CHANNELS;
        seq.repeats             = 0;
        seq.end_delay           = 0;
        
        nrfx_pwm_simple_playback(&m_pwm_instance, &seq, 1, 0);
    }
}