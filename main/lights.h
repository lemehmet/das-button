#pragma once

#define LED_ON  (0)
#define LED_OFF (1)

#define COLOR_MAX       (255)
#define NORM_COLOR(c)   (COLOR_MAX - (c % (COLOR_MAX + 1)))

void lights_init();
void lights_basic_color(int r, int g, int b);
void lights_8bit_color(int r, int g, int b);
void lights_breathe(int cycles, int dur_ms);