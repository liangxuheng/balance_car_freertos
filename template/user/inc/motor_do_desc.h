#ifndef __MOTOR_DO_DESC_H_
#define __MOTOR_DO_DESC_H_
#include <stdbool.h>
typedef enum {
	MOTOR_DO_CMD,
	MOTOR_DO_SET,
}motor_do_type;
struct motor_do_TypeDef{
	motor_do_type type_t;
	union{
		struct {
			struct motor_Typedef*motor_x;
			bool cmd;
		}motor_do_cmd;
		struct{
			struct motor_Typedef*motor_x;
			float value;
		}motor_do_set;
	};
};
#endif
