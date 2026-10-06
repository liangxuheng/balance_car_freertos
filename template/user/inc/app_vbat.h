#ifndef __APP_VBAT_H_
#define __APP_VBAT_H_

struct adc_struct;
typedef struct adc_struct* adc_handler;
struct timer_struct;
typedef struct timer_struct* timer_handler;
struct bat_led_init_struct;
typedef struct bat_led_init_struct* bat_led_init_handler;


void app_vbat_init(timer_handler timer_x,adc_handler adc_x,bat_led_init_handler led_init_handler);
float app_vbat_get(adc_handler adc_x);
//void app_vbat_proc(bat_led_init_handler led_init_handler,adc_handler adc_x);

#endif
