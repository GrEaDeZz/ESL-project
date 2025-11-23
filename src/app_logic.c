#include "app_logic.h"
#include "pwm_handler.h"
#include "app_timer.h"
#include "nrf_log.h"
#include <math.h>

// Параметры обновления
#define VALUE_UPDATE_INTERVAL_MS    35
#define HUE_STEP                    1
#define SAT_STEP                    1
#define VAL_STEP                    1

// Режимы работы
typedef enum
{
    INPUT_MODE_NONE = 0,
    INPUT_MODE_HUE,
    INPUT_MODE_SAT,
    INPUT_MODE_VAL,
    INPUT_MODE_COUNT
} input_mode_t;

// Структура HSV
typedef struct
{
    uint16_t h; // 0-360
    uint8_t  s; // 0-100
    uint8_t  v; // 0-100
} hsv_t;

static hsv_t        m_current_hsv;
static input_mode_t m_current_mode = INPUT_MODE_NONE;
static bool         m_is_holding = false;

// Направление: 1 = вверх, -1 = вниз
static int8_t       m_sat_direction = -1;
static int8_t       m_val_direction = -1;

APP_TIMER_DEF(m_update_timer);

// Конвертация HSV -> RGB
static void hsv_to_rgb(hsv_t hsv, uint16_t *r, uint16_t *g, uint16_t *b)
{
    float H = hsv.h;
    float S = hsv.s / 100.0f;
    float V = hsv.v / 100.0f;
    
    float C = V * S;
    float X = C * (1 - fabsf(fmodf(H / 60.0f, 2) - 1));
    float m = V - C;
    
    float R_temp, G_temp, B_temp;
    
    if (H >= 0 && H < 60) {
        R_temp = C; G_temp = X; B_temp = 0;
    } else if (H >= 60 && H < 120) {
        R_temp = X; G_temp = C; B_temp = 0;
    } else if (H >= 120 && H < 180) {
        R_temp = 0; G_temp = C; B_temp = X;
    } else if (H >= 180 && H < 240) {
        R_temp = 0; G_temp = X; B_temp = C;
    } else if (H >= 240 && H < 300) {
        R_temp = X; G_temp = 0; B_temp = C;
    } else {
        R_temp = C; G_temp = 0; B_temp = X;
    }
    
    *r = (uint16_t)((R_temp + m) * 1000);
    *g = (uint16_t)((G_temp + m) * 1000);
    *b = (uint16_t)((B_temp + m) * 1000);
}

// Обновление LED
static void update_leds(void)
{
    uint16_t r, g, b;
    hsv_to_rgb(m_current_hsv, &r, &g, &b);
    pwm_handler_set_rgb(r, g, b);
}

// Смена режима
static void set_mode(input_mode_t new_mode)
{
    m_current_mode = new_mode;
    
    switch (m_current_mode)
    {
        case INPUT_MODE_NONE:
            pwm_handler_set_indicator_mode(PWM_INDICATOR_OFF);
            break;
        case INPUT_MODE_HUE:
            pwm_handler_set_indicator_mode(PWM_INDICATOR_BLINK_SLOW);
            break;
        case INPUT_MODE_SAT:
            pwm_handler_set_indicator_mode(PWM_INDICATOR_BLINK_FAST);
            break;
        case INPUT_MODE_VAL:
            pwm_handler_set_indicator_mode(PWM_INDICATOR_ON);
            break;
        default: break;
    }
}

// Таймер изменения значений
static void update_timer_handler(void * p_context)
{
    if (!m_is_holding || m_current_mode == INPUT_MODE_NONE) return;

    switch (m_current_mode)
    {
        case INPUT_MODE_HUE:
            // Hue (0-360)
            m_current_hsv.h = (m_current_hsv.h + HUE_STEP) % 360;
            break;

        case INPUT_MODE_SAT:
            // Логика маятника для Saturation
            {
                int16_t new_sat = m_current_hsv.s + (m_sat_direction * SAT_STEP);
                
                if (new_sat >= 100)
                {
                    new_sat = 100;
                    m_sat_direction = -1; // Разворачиваем вниз
                }
                else if (new_sat <= 0)
                {
                    new_sat = 0;
                    m_sat_direction = 1;  // Разворачиваем вверх
                }
                m_current_hsv.s = (uint8_t)new_sat;
            }
            break;

        case INPUT_MODE_VAL:
            // Логика маятника для Value (Яркость)
            {
                int16_t new_val = m_current_hsv.v + (m_val_direction * VAL_STEP);
                
                if (new_val >= 100)
                {
                    new_val = 100;
                    m_val_direction = -1; // Разворачиваем вниз
                }
                else if (new_val <= 0)
                {
                    new_val = 0;
                    m_val_direction = 1;  // Разворачиваем вверх
                }
                m_current_hsv.v = (uint8_t)new_val;
            }
            break;

        default: break;
    }
    update_leds();
}

// Обработка событий кнопки
void app_logic_on_button_event(button_event_t event)
{
    switch (event)
    {
        case BUTTON_EVENT_DOUBLE_CLICK:
            if (m_current_mode == INPUT_MODE_VAL) {
                NRF_LOG_INFO("Current Settings -> Hue: %d, Sat: %d, Val: %d", 
                             m_current_hsv.h, m_current_hsv.s, m_current_hsv.v);
            }
            set_mode((m_current_mode + 1) % INPUT_MODE_COUNT);
            break;

        case BUTTON_EVENT_PRESSED:
            m_is_holding = true;
            if (m_current_mode != INPUT_MODE_NONE) {
                app_timer_start(m_update_timer, APP_TIMER_TICKS(VALUE_UPDATE_INTERVAL_MS), NULL);
            }
            break;

        case BUTTON_EVENT_RELEASED:
            m_is_holding = false;
            app_timer_stop(m_update_timer);
            break;
    }
}

// Инициализация логики
void app_logic_init(const int *id_digits)
{
    int last_two_digits = id_digits[2] * 10 + id_digits[3];
    
    m_current_hsv.h = (uint16_t)(360 * (last_two_digits / 100.0f));
    m_current_hsv.s = 100;
    m_current_hsv.v = 100;
    
    m_sat_direction = -1;
    m_val_direction = -1;

    app_timer_create(&m_update_timer, APP_TIMER_MODE_REPEATED, update_timer_handler);

    set_mode(INPUT_MODE_NONE);
    update_leds();
}