#ifndef APP_LOGIC_H
#define APP_LOGIC_H

#include <stdint.h>
#include <stdbool.h>
#include "button_handler.h"

// Максимальное количество сохраненных цветов
#define MAX_SAVED_COLORS 10
// Длина имени цвета
#define COLOR_NAME_LEN   12

// Структура цвета HSV
typedef struct
{
    uint16_t h; // 0-360
    uint8_t  s; // 0-100
    uint8_t  v; // 0-100
} app_logic_hsv_t;

// Структура записи сохраненного цвета
typedef struct
{
    char name[COLOR_NAME_LEN];
    app_logic_hsv_t color;
} saved_color_entry_t;

// Инициализация логики приложения
void app_logic_init(const int *id_digits);

// Обработчик событий от кнопки (вызывается из button_handler)
void app_logic_on_button_event(button_event_t event);

// Установка цвета в формате RGB
void app_logic_set_rgb(uint16_t r, uint16_t g, uint16_t b);

// Установка цвета в формате HSV
void app_logic_set_hsv(uint16_t h, uint8_t s, uint8_t v);

// Сохранить HSV цвет в список
bool app_logic_save_color_hsv(uint16_t h, uint8_t s, uint8_t v, const char * name);

// Сохранить RGB цвет в список
bool app_logic_save_color_rgb(uint16_t r, uint16_t g, uint16_t b, const char * name);

// Сохранить текущий активный цвет в список
bool app_logic_save_current_color(const char * name);

// Удалить цвет по имени
bool app_logic_del_color(const char * name);

// Применить цвет из списка
bool app_logic_apply_color(const char * name);

// Получить список цветов
const saved_color_entry_t * app_logic_get_list(uint8_t * count);

#endif