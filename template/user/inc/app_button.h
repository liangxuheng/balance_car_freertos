#ifndef __APP_BUTTON_H_
#define __APP_BUTTON_H_
#include "my_button.h"
void app_button_init(my_button_handler button_x
	,uint8_t (*button_usr_read)(void*),void (*ClickCb)(void*,uint8_t clicks));
//void app_button_proc(my_button_handler button_x);
#endif
