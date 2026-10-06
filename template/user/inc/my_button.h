#ifndef __MY_BUTTON_DESC_H_
#define __MY_BUTTON_DESC_H_
#include <stdint.h>

struct my_button_struct;
typedef struct my_button_struct* my_button_handler;

void my_button_init(my_button_handler button_x,uint8_t (*__button_usr_read)(void*));
int8_t my_button_get_state(my_button_handler button_x);
void my_button_ClickIntervalConfig(my_button_handler button_x
	,uint32_t click_interval);
void my_button_LongPressIntervalConfig(my_button_handler button_x
	,uint32_t long_press_interval );
void my_button_LongPressThresholdConfig(my_button_handler button_x
	,uint32_t long_press_threshold);
void my_button_set_longpresscb(my_button_handler button_x
	,void (*LongPressCb)(void*,uint8_t ticks));
void my_button_set_presscb(my_button_handler button_x,void (*PressCb)(void*));
void my_button_set_releasecb(my_button_handler button_x,void (*ReleaseCb)(void*));
void my_button_set_clickcb(my_button_handler button_x
	,void (*ClickCb)(void*,uint8_t clicks));
void my_button_set_usrread(my_button_handler button_x
	,uint8_t (*button_usr_read)(void*));
void my_button_proc(my_button_handler button_x);
#endif
