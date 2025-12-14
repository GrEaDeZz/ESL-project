#include "app_logic.h"
#include "pwm_handler.h"
#include "app_timer.h"
#include "nrf_log.h"
#include "nrfx_nvmc.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

// Параметры обновления
#define VALUE_UPDATE_INTERVAL_MS    15
#define HUE_STEP                    1
#define SAT_STEP                    1
#define VAL_STEP                    1

// Адрес страницы для сохранения настроек.
#define FLASH_SAVE_ADDR             0x7F000

// Режимы работы
typedef enum
{
    INPUT_MODE_NONE = 0,
    INPUT_MODE_HUE,
    INPUT_MODE_SAT,
    INPUT_MODE_VAL,
    INPUT_MODE_COUNT
} input_mode_t;

// Структура данных во Flash
typedef struct
{
    app_logic_hsv_t current_color;
    uint32_t count;
    saved_color_entry_t list[MAX_SAVED_COLORS];
} app_flash_data_t;

// Локальные переменные
static app_flash_data_t m_app_data;          
static input_mode_t     m_current_mode = INPUT_MODE_NONE;
static bool             m_is_holding = false;

// Направление: 1 = вверх, -1 = вниз
static int8_t       m_sat_direction = -1;
static int8_t       m_val_direction = -1;

APP_TIMER_DEF(m_update_timer);

// Сохранение всех данных в Flash
static void save_all_data_to_flash(void)
{
    nrfx_nvmc_page_erase(FLASH_SAVE_ADDR);
    nrfx_nvmc_words_write(FLASH_SAVE_ADDR, (uint32_t *)&m_app_data, sizeof(m_app_data) / 4);
    while (nrfx_nvmc_write_done_check() == false);
}

// Конвертация RGB -> HSV
static void rgb_to_hsv(uint16_t r, uint16_t g, uint16_t b, app_logic_hsv_t *hsv)
{
    float R = r / 1000.0f;
    float G = g / 1000.0f;
    float B = b / 1000.0f;

    float cmax = MAX(R, MAX(G, B));
    float cmin = MIN(R, MIN(G, B));
    float delta = cmax - cmin;

    if (delta == 0) {
        hsv->h = 0;
    } else if (cmax == R) {
        float mod_val = fmodf(((G - B) / delta), 6);
        if (mod_val < 0)
            mod_val += 6.0f;
        hsv->h = (uint16_t)(60 * mod_val);
    } else if (cmax == G) {
        hsv->h = (uint16_t)(60 * (((B - R) / delta) + 2));
    } else {
        hsv->h = (uint16_t)(60 * (((R - G) / delta) + 4));
    }

    if (cmax == 0) {
        hsv->s = 0;
    } else {
        hsv->s = (uint8_t)((delta / cmax) * 100);
    }

    hsv->v = (uint8_t)(cmax * 100);
}

// Конвертация HSV -> RGB
static void hsv_to_rgb(app_logic_hsv_t hsv, uint16_t *r, uint16_t *g, uint16_t *b)
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
    hsv_to_rgb(m_app_data.current_color, &r, &g, &b);
    pwm_handler_set_rgb(r, g, b);
}

// Смена режима
static void set_mode(input_mode_t new_mode)
{
    if (new_mode == INPUT_MODE_NONE && m_current_mode != INPUT_MODE_NONE)
        save_all_data_to_flash();

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
            m_app_data.current_color.h = (m_app_data.current_color.h + HUE_STEP) % 360;
            break;

        case INPUT_MODE_SAT:
            // Логика маятника для Saturation
            {
                int16_t new_sat = m_app_data.current_color.s + (m_sat_direction * SAT_STEP);
                
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
                m_app_data.current_color.s = (uint8_t)new_sat;
            }
            break;

        case INPUT_MODE_VAL:
            // Логика маятника для Value (Яркость)
            {
                int16_t new_val = m_app_data.current_color.v + (m_val_direction * VAL_STEP);
                
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
                m_app_data.current_color.v = (uint8_t)new_val;
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
    app_flash_data_t * p_flash = (app_flash_data_t *)FLASH_SAVE_ADDR;

    if (p_flash->count > MAX_SAVED_COLORS)
    {
        memset(&m_app_data, 0, sizeof(app_flash_data_t));
        
        int last_two_digits = id_digits[2] * 10 + id_digits[3];
        m_app_data.current_color.h = (uint16_t)(360 * (last_two_digits / 100.0f));
        m_app_data.current_color.s = 100;
        m_app_data.current_color.v = 100;
        m_app_data.count = 0;
        
        save_all_data_to_flash();
    }
    else
    {
        memcpy(&m_app_data, p_flash, sizeof(app_flash_data_t));
        if (m_app_data.current_color.h > 360) m_app_data.current_color.h = 0;
        if (m_app_data.current_color.s > 100) m_app_data.current_color.s = 100;
        if (m_app_data.current_color.v > 100) m_app_data.current_color.v = 100;
    }

    m_sat_direction = -1;
    m_val_direction = -1;

    app_timer_create(&m_update_timer, APP_TIMER_MODE_REPEATED, update_timer_handler);

    set_mode(INPUT_MODE_NONE);
    update_leds();
}

// Установка HSV
void app_logic_set_hsv(uint16_t h, uint8_t s, uint8_t v)
{
    m_app_data.current_color.h = (h > 360) ? 360 : h;
    m_app_data.current_color.s = (s > 100) ? 100 : s;
    m_app_data.current_color.v = (v > 100) ? 100 : v;

    set_mode(INPUT_MODE_NONE);
    update_leds();
    save_all_data_to_flash();
}

// Установка RGB
void app_logic_set_rgb(uint16_t r, uint16_t g, uint16_t b)
{
    if (r > 1000) r = 1000;
    if (g > 1000) g = 1000;
    if (b > 1000) b = 1000;

    set_mode(INPUT_MODE_NONE); 
    rgb_to_hsv(r, g, b, &m_app_data.current_color);
    update_leds();
    save_all_data_to_flash();
}


bool app_logic_save_color_hsv(uint16_t h, uint8_t s, uint8_t v, const char * name)
{
    if (m_app_data.count >= MAX_SAVED_COLORS)
        return false;

    for (uint32_t i = 0; i < m_app_data.count; i++)
    {
        if (strcmp(m_app_data.list[i].name, name) == 0)
        {
            // Имя занято
            return false; 
        }
    }
    uint32_t id = m_app_data.count;
    
    strncpy(m_app_data.list[id].name, name, COLOR_NAME_LEN - 1);
    m_app_data.list[id].name[COLOR_NAME_LEN - 1] = '\0';

    m_app_data.list[id].color.h = h;
    m_app_data.list[id].color.s = s;
    m_app_data.list[id].color.v = v;

    m_app_data.count++;
    save_all_data_to_flash();
    
    return true;
}

bool app_logic_save_color_rgb(uint16_t r, uint16_t g, uint16_t b, const char * name)
{
    app_logic_hsv_t temp_hsv;
    rgb_to_hsv(r, g, b, &temp_hsv);
    return app_logic_save_color_hsv(temp_hsv.h, temp_hsv.s, temp_hsv.v, name);
}

bool app_logic_save_current_color(const char * name)
{
    return app_logic_save_color_hsv(m_app_data.current_color.h, 
                                    m_app_data.current_color.s, 
                                    m_app_data.current_color.v, 
                                    name);
}

bool app_logic_del_color(const char * name)
{
    for (uint32_t i = 0; i < m_app_data.count; i++)
    {
        if (strcmp(m_app_data.list[i].name, name) == 0)
        {
            // Сдвигаем массив
            for (uint32_t j = i; j < m_app_data.count - 1; j++)
            {
                m_app_data.list[j] = m_app_data.list[j+1];
            }
            m_app_data.count--;
            save_all_data_to_flash();
            return true;
        }
    }
    return false;
}

bool app_logic_apply_color(const char * name)
{
    for (uint32_t i = 0; i < m_app_data.count; i++)
    {
        if (strcmp(m_app_data.list[i].name, name) == 0)
        {
            app_logic_set_hsv(m_app_data.list[i].color.h,
                              m_app_data.list[i].color.s,
                              m_app_data.list[i].color.v);
            return true;
        }
    }
    return false;
}

const saved_color_entry_t * app_logic_get_list(uint8_t * count)
{
    *count = (uint8_t)m_app_data.count;
    return m_app_data.list;
}