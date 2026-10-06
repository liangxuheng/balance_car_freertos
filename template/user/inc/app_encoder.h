#ifndef __APP_ENCODER_H_
#define __APP_ENCODER_H_
#include <stdint.h>
struct encoder_struct;
typedef struct encoder_struct* encoder_handler;
void app_encoder_init(encoder_handler encoder_x);
float encoder_get_pos(encoder_handler encoder_x);
float encoder_get_speed(encoder_handler encoder_x);
#endif
