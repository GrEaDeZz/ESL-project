#ifndef APP_LOGIC_H
#define APP_LOGIC_H

#include <stdint.h>
#include "button_handler.h"

// Инициализация логики приложения
void app_logic_init(const int *id_digits);

// Обработчик событий от кнопки (вызывается из button_handler)
void app_logic_on_button_event(button_event_t event);

#endif