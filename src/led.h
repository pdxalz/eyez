#ifndef LED_H__
#define LED_H__

void led_init();
void led_update();
void set_color_string(const uint8_t *str);
void set_color_pattern(int index);
void next_color_pattern();

#endif // LED_H__