#ifndef __BAT_LED_H_
#define __BAT_LED_H_
#include <stdbool.h>
typedef enum{
	LED_LOW,
	LED_MID,
	LED_TOP
}led_stat_t;

struct bat_led_struct;
typedef struct bat_led_struct* bat_led_handler;
struct bat_led_init_struct;
typedef struct bat_led_init_struct* bat_led_init_handler;

void bat_led_init(bat_led_handler bat_led_x);
void bat_led_init_s(bat_led_init_handler led_init_handler);
void bat_led_set(bat_led_handler led_x,bool is_on);
bat_led_handler bat_led_get_handler(bat_led_init_handler led_init_handler,led_stat_t state);
#endif
