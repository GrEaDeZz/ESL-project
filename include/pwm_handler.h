#ifndef PWM_HANDLER_H
#define PWM_HANDLER_H

#include <stdint.h>

void pwm_handler_init(const uint32_t *led_pins, const int *id_digits);

void pwm_handler_toggle_playback(void);

#endif
