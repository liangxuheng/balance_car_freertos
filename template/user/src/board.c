/**
 ******************************************************************************
 * @file    board.c
 * @brief   板级硬件描述：所有外设的引脚/参数宏定义和全局句柄实例化
 * @author  平衡车项目
 *
 * @details 这里集中定义：定时器/ADC/串口/I2C/编码器/PWM 的硬件参数，
 *          通过 xxx_desc.h 宏展开为静态结构体，再实例化为全局句柄。
 *
 ******************************************************************************
 */

#include "board.h"
#include "timer_desc.h"
#include "adc_desc.h"
#include "my_usart_desc.h"
#include "bat_led.h"
#include "bat_led_desc.h"
#include "my_button_desc.h"
#include "pwm_desc.h"
#include "app_encoder_desc.h"
#include "soft_i2c_desc.h"
#include "app_mpu6050_desc.h"
#include "pid_desc.h"
#include "app_motor_desc.h"
#include "app_control_desc.h"
#include "hardware_i2c_desc.h"
#include "app_rc_desc.h"
#include <stddef.h>
#define G 9.8f   /**< 重力加速度，用于MPU6050数据校准 */


	/* 1. 时基单元 */
#define TIMER_BASE_DESC(x, TIM_X, PRESCALER, COUNTER_MODE, PERIOD, CLOCK_DIV, REP_COUNTER)  \
static struct timer_base __TIM_BASE##x = {                          \
    .__tim_x                 = TIM_X,                               \
    .__TIM_Prescaler         = (PRESCALER),                         \
    .__TIM_CounterMode       = COUNTER_MODE,                        \
    .__TIM_Period            = (PERIOD),                            \
    .__TIM_ClockDivision     = CLOCK_DIV,                           \
    .__TIM_RepetitionCounter = (REP_COUNTER),                       \
};
 
/* 2. 输出比较 */
#define TIMER_OC_DESC(x, OC_MODE, OUT_STATE, OUTN_STATE, PULSE, OC_POLARITY, OCN_POLARITY, OC_IDLE, OCN_IDLE)  \
static struct timer_oc __TIM_OC##x = {                              \
    .__TIM_OCMode         = OC_MODE,                                \
    .__TIM_OutputState    = OUT_STATE,                              \
    .__TIM_OutputNState   = OUTN_STATE,                             \
    .__TIM_Pulse          = PULSE,                                  \
    .__TIM_OCPolarity     = OC_POLARITY,                            \
    .__TIM_OCNPolarity    = OCN_POLARITY,                           \
    .__TIM_OCIdleState    = OC_IDLE,                                \
    .__TIM_OCNIdleState   = OCN_IDLE,                               \
};
 
/* 3. 顶层：自动挂上同号的 base 和 oc */
#define TIMER_DESC(x, TRGO_SOURCE,NVIC_HANDLER)                                  \
static struct timer_struct __TIM##x = {                             \
    .__timer_base        = &__TIM_BASE##x,                          \
    .__timer_trgo_source = TRGO_SOURCE,                             \
    .__timer_oc          = &__TIM_OC##x,                            \
		.__nvic              =NVIC_HANDLER,															\
};
 
/* 4. 不需要输出比较的定时器（如电池模块的 TIM2） */
#define TIMER_DESC_NO_OC(x, TRGO_SOURCE,NVIC_HANDLER)                            \
static struct timer_struct __TIM##x = {                             \
    .__timer_base        = &__TIM_BASE##x,                          \
    .__timer_trgo_source = TRGO_SOURCE,                             \
    .__timer_oc          = NULL,      /* 无输出比较 */                  \
		.__nvic              =NVIC_HANDLER,                             \
};

#define TIMER_NVIC_DESC(x,IRQN)\
static struct timer_nvic __TIM_NVIC##x={\
		.nvic_irqn =(IRQN),	\
};


	#define GPIO_PWM_DESC(x, GPIOx, PIN, SPEED, MODE)   \
/* 定义静态的 GPIO_PWM_struct 对象 */                \
static struct GPIO_PWM_struct __GPIO_PWM##x = {      \
    .__GPIOx      = GPIOx,                           \
    .__GPIO_Pin   = PIN,                             \
    .__GPIO_Speed = SPEED,                           \
    .__GPIO_Mode  = MODE,                            \
};
         

	#define STBY_DESC(x, GPIOx, PIN, SPEED, MODE)                       \
static struct stby_struct __STBY##x = {                             \
    .__GPIOx      = GPIOx,                                          \
    .__GPIO_Pin   = PIN,                                            \
    .__GPIO_Speed = SPEED,                                          \
    .__GPIO_Mode  = MODE,                                           \
};
 
#define GPIO_IN_DESC(x, GPIOx1, PIN1, SPEED1, MODE1, GPIOx2, PIN2, SPEED2, MODE2)  \
static struct GPIO_IN_struct __GPIO_IN##x = {                       \
    .__GPIOx_1     = GPIOx1, .__GPIO_Pin_1   = PIN1,                \
    .__GPIO_Speed_1 = SPEED1, .__GPIO_Mode_1 = MODE1,               \
    .__GPIOx_2     = GPIOx2, .__GPIO_Pin_2   = PIN2,                \
    .__GPIO_Speed_2 = SPEED2, .__GPIO_Mode_2 = MODE2,               \
};
 
	#define PWM_DESC(x, STBY_HANDLER, TIMER_HANDLER, GPIO_IN_HANDLER, GPIO_PWM_HANDLER)  \
/* 定义静态的 pwm_struct 对象，把四部分挂上去 */                                 \
static struct pwm_struct __PWM##x = {                                          \
    .stby_handler_pwm = STBY_HANDLER,                                          \
    .timer            = TIMER_HANDLER,                                         \
    .GPIO_IN          = GPIO_IN_HANDLER,                                       \
    .GPIO_PWM         = GPIO_PWM_HANDLER,                                      \
};

/* ================= 左电机：TIM1 ================= */
TIMER_BASE_DESC(1, TIM1, 0, TIM_CounterMode_Up, 999, TIM_CKD_DIV1, 0)
TIMER_OC_DESC(1, TIM_OCMode_PWM1, TIM_OutputState_Enable, TIM_OutputNState_Disable, 0,
              TIM_OCPolarity_High, TIM_OCNPolarity_High,
              TIM_OCIdleState_Reset, TIM_OCNIdleState_Reset)
TIMER_DESC(1, TIM_TRGOSource_Reset,NULL)      /* 电机不需要 TRGO */
 
/* ================= 右电机：TIM4 ================= */
TIMER_BASE_DESC(4, TIM4, 0, TIM_CounterMode_Up, 999, TIM_CKD_DIV1, 0)
TIMER_OC_DESC(4, TIM_OCMode_PWM1, TIM_OutputState_Enable, TIM_OutputNState_Disable, 0,
              TIM_OCPolarity_High, TIM_OCNPolarity_High,
              TIM_OCIdleState_Reset, TIM_OCNIdleState_Reset)
TIMER_DESC(4, TIM_TRGOSource_Reset,NULL)
 
/* ========= 电池模块的 TIM2（顺带展示拆分的好处，不需要 oc）========= */
TIMER_BASE_DESC(2, TIM2, 71, TIM_CounterMode_Up, 9999, TIM_CKD_DIV1, 0)
TIMER_DESC_NO_OC(2, TIM_TRGOSource_Update,NULL)

/* ===== 微秒时基 TIM3: 72MHz/72=1MHz, ARR=65535, 无OC无TRGO ===== */ 
TIMER_BASE_DESC( 3 , TIM3, 71 , TIM_CounterMode_Up, 1000-1 , TIM_CKD_DIV1, 0 )
TIMER_NVIC_DESC(3,TIM3_IRQn)
TIMER_DESC_NO_OC( 3 , TIM_TRGOSource_Reset,&__TIM_NVIC3)

timer_handler timer_us=&__TIM3;
 
/* ================= TB6612 ================= */
STBY_DESC(1, GPIOA, GPIO_Pin_1, GPIO_Speed_2MHz, GPIO_Mode_Out_PP)   /* PA1 - Out_PP */
 
GPIO_IN_DESC(1, GPIOA, GPIO_Pin_9,  GPIO_Speed_2MHz, GPIO_Mode_Out_PP,
                GPIOA, GPIO_Pin_10, GPIO_Speed_2MHz, GPIO_Mode_Out_PP)   /* 左：AIN1 / AIN2 */
GPIO_IN_DESC(2, GPIOB, GPIO_Pin_5,  GPIO_Speed_2MHz, GPIO_Mode_Out_PP,
                GPIOB, GPIO_Pin_7,  GPIO_Speed_2MHz, GPIO_Mode_Out_PP)   /* 右：BIN1 / BIN2 */
 
GPIO_PWM_DESC(1, GPIOA, GPIO_Pin_8, GPIO_Speed_2MHz, GPIO_Mode_AF_PP)    /* 左：PA8 = TIM1_CH1 */
GPIO_PWM_DESC(2, GPIOB, GPIO_Pin_6, GPIO_Speed_2MHz, GPIO_Mode_AF_PP)    /* 右：PB6 = TIM4_CH1 */
 
PWM_DESC(1, &__STBY1, &__TIM1, &__GPIO_IN1, &__GPIO_PWM1)   /* 左电机 */
PWM_DESC(2, &__STBY1, &__TIM4, &__GPIO_IN2, &__GPIO_PWM2)   /* 右电机，STBY 共用 */


timer_handler timer_1=&__TIM2;
pwm_handler pwm_l_handler=&__PWM1;
pwm_handler pwm_2_handler=&__PWM2;


	#define ADC_DESC(x, ADC_X, INJ_CMD, ADC_MODE, SCAN_MODE, \
CONT_MODE, EXT_TRIG, DATA_ALIGN, NBR_OF_CH, CHANNEL, RANK, SAMPLE_TIME,\
EXT_TRIG_INJ, IS_USE_TRIG, INJ_CHANNEL, INJ_RANK, INJ_SAMPLE_TIME, GPIO_X,\
GPIO_PIN, GPIO_SPEED, GPIO_MODE,IS_USE_TRGI_BASE,IS_JEOC,IS_EOC)  \
/* 1. 定义静态的 adc_base_struct 对象（规则组） */                          \
static struct adc_base_struct __ADC_BASE##x = {                            \
    .__ADC_Mode               = ADC_MODE,                                  \
    .__ADC_ScanConvMode       = SCAN_MODE,                                 \
    .__ADC_ContinuousConvMode = CONT_MODE,                                 \
    .__ADC_ExternalTrigConv   = EXT_TRIG,                                  \
    .__ADC_DataAlign          = DATA_ALIGN,                                \
    .__ADC_NbrOfChannel       = (NBR_OF_CH),                               \
    .__ADC_Channel            = (CHANNEL),                                 \
    .__Rank                   = (RANK),                                    \
    .__ADC_SampleTime         = (SAMPLE_TIME),                             \
		.__is_use_trig          	= (IS_USE_TRGI_BASE),                                 \
};                                                                         \
/* 2. 定义静态的 adc_injected_struct 对象（注入组） */                       \
static struct adc_injected_struct __ADC_INJ##x = {                         \
    .__ADC_ExternalTrigInjecConv = EXT_TRIG_INJ,                           \
    .__is_use_trig               = IS_USE_TRIG,                            \
    .__ADC_Channel               = (INJ_CHANNEL),                          \
    .__Rank                      = (INJ_RANK),                             \
    .__ADC_SampleTime            = (INJ_SAMPLE_TIME),                      \
};                                                                         \
/* 3. 定义静态的 adc_struct 对象，把 base / injected / GPIO 都挂上 */        \
static struct adc_struct __ADC##x = {                                      \
    .adc_base          = &__ADC_BASE##x,                                   \
    .__adc_x           = ADC_X,                                            \
    .__InjectedConvCmd = INJ_CMD,                                          \
    .adc_injected      = &__ADC_INJ##x,                                    \
    .__GPIO_X          = GPIO_X,                                           \
    .__GPIO_Pin        = GPIO_PIN,                                         \
    .__GPIO_Speed      = GPIO_SPEED,                                       \
    .__GPIO_Mode       = GPIO_MODE,                                        \
		.__is_jeoc         = IS_JEOC,																						\
		.__is_eoc          = IS_EOC,																					\
};



ADC_DESC(1, ADC1, true,
         ADC_Mode_Independent, DISABLE, DISABLE,
         ADC_ExternalTrigConv_None, ADC_DataAlign_Right,
         0, 0, 0, 0,                                  /* 规则组未使用，NbrOfChannel 填 0 */
         ADC_ExternalTrigInjecConv_T2_TRGO, true,
         ADC_Channel_8, 1, ADC_SampleTime_7Cycles5,
         GPIOB, GPIO_Pin_0, GPIO_Speed_50MHz, GPIO_Mode_AIN,false,true,false);
adc_handler adc_1=&__ADC1;
				 
#define USART_DESC(x, USART_X, GPIO_X, BAUDRATE, WORD_LENGTH, STOP_BITS, PARITY, MODE, \
				 HW_FLOW, TX_PIN, TX_SPEED, TX_MODE, RX_PIN, RX_SPEED, RX_MODE)  \
/* 定义静态的 usart_struct 对象 */                                          \
static struct usart_struct __USART##x = {                                  \
    .__usart_x                    = USART_X,                               \
    .__GPIO_X                     = GPIO_X,                                \
    .__USART_BaudRate             = BAUDRATE,                              \
    .__USART_WordLength           = WORD_LENGTH,                           \
    .__USART_StopBits             = STOP_BITS,                             \
    .__USART_Parity               = PARITY,                                \
    .__USART_Mode                 = MODE,                                  \
    .__USART_HardwareFlowControl  = HW_FLOW,                               \
    .__GPIO_Tx_Pin                = TX_PIN,                                \
    .__GPIO_Tx_Speed              = TX_SPEED,                              \
    .__GPIO_Tx_Mode               = TX_MODE,                               \
    .__GPIO_Rx_Pin                = RX_PIN,                                \
    .__GPIO_Rx_Speed              = RX_SPEED,                              \
    .__GPIO_Rx_Mode               = RX_MODE,                               \
};				 
				 
USART_DESC(1, USART2, GPIOA,
           921600, USART_WordLength_8b, USART_StopBits_1, USART_Parity_No,
           USART_Mode_Tx | USART_Mode_Rx, USART_HardwareFlowControl_None,
           GPIO_Pin_2, GPIO_Speed_2MHz, GPIO_Mode_AF_PP,
           GPIO_Pin_3, GPIO_Speed_2MHz, GPIO_Mode_IPU);	
usart_handler usart_1=&__USART1;

#define BAT_LED_DESC(x, GPIO_X, PIN, SPEED, MODE)   \
/* 定义静态的 bat_led_struct 对象 */                 \
static struct bat_led_struct __BAT_LED##x = {        \
    .__GPIO_X     = GPIO_X,                          \
    .__GPIO_Pin   = PIN,                             \
    .__GPIO_Speed = SPEED,                           \
    .__GPIO_Mode  = MODE,                            \
};

BAT_LED_DESC(1, GPIOA, GPIO_Pin_4, GPIO_Speed_2MHz, GPIO_Mode_Out_PP);   /* 第1颗：满电/75%/50% 亮 */
BAT_LED_DESC(2, GPIOA, GPIO_Pin_5, GPIO_Speed_2MHz, GPIO_Mode_Out_PP);   /* 第2颗：满电/75% 亮 */
BAT_LED_DESC(3, GPIOA, GPIO_Pin_6, GPIO_Speed_2MHz, GPIO_Mode_Out_PP);   /* 第3颗：仅满电亮 */

bat_led_handler bat_led_low=&__BAT_LED1;
bat_led_handler bat_led_mid=&__BAT_LED2;
bat_led_handler bat_led_top=&__BAT_LED3;


#define BAT_LED_INIT_DESC(x,LED_TOP,LED_MID,LED_LOW)\
static struct bat_led_init_struct __BAT_LED_INIT##x={\
		.__led_top=(LED_TOP),\
		.__led_mid=(LED_MID),\
		.__led_low=(LED_LOW),\
};

BAT_LED_INIT_DESC(1,&__BAT_LED3,&__BAT_LED2,&__BAT_LED1);
bat_led_init_handler __bat_led_init=&__BAT_LED_INIT1;

#define MY_BUTTON_DESC(x, GPIOx, PIN, SPEED, MODE, PRESS_LEVEL, LONG_PRESS_THRESHOLD, LONG_PRESS_TICK_INTERVAL, CLICK_INTERVAL, PRESSED_CB, RELEASED_CB, CLICKED_CB, LONG_PRESSED_CB, USR_READ)  \
/* 定义静态的 my_button_struct 对象（未列出的成员自动清零） */                 \
static struct my_button_struct __BUTTON##x = {                                \
    /* ---------- 初始化参数 ---------- */                                    \
    .__GPIOx                   = GPIOx,                                       \
    .__GPIO_Pin                = PIN,                                         \
    .__GPIO_Speed              = SPEED,                                       \
    .__GPIO_Mode               = MODE,                                        \
    .__press_level             = PRESS_LEVEL,                                 \
    .__LongPressThreshold       = LONG_PRESS_THRESHOLD,      /* 单下划线 */     \
    .__LongPressTickInterval   = LONG_PRESS_TICK_INTERVAL,                    \
    .__ClickInterval           = CLICK_INTERVAL,                              \
    .__button_pressed_cb       = PRESSED_CB,                                  \
    .__button_released_cb      = RELEASED_CB,                                 \
    .__button_clicked_cb       = CLICKED_CB,                                  \
    .__button_long_pressed_cb  = LONG_PRESSED_CB,                             \
    .__button_usr_read         = USR_READ,                                    \
};

MY_BUTTON_DESC(1,
               GPIOA, GPIO_Pin_11,          /* ← 代码里没出现按键引脚，按你板子实际接线改 */
               GPIO_Speed_2MHz,            /* 原代码没赋值（见说明 2） */
               GPIO_Mode_IPU,              /* gpio_init_struct.GPIO_Mode = GPIO_Mode_IPU */
               0,                          /* 低电平按下 */
               0,                       /* BUTTON_LONG_PRESS_THRESHOLD */
               0,                        /* BUTTON_LONG_PRESS_TICK_INTERNVAL */
               0,                        /* BUTTON_CLICK_INTERVAL */
							 NULL, NULL, NULL, NULL,                 /* 四个回调，先用 Set*Cb 注册 */
               NULL)                          /* __button_usr_read，暂不使用 */

//用于控制向standby引脚写电平
my_button_handler my_button_1=&__BUTTON1;

#define STBY_DESC(x, GPIOx, PIN, SPEED, MODE)   \
/* 定义静态的 stby_struct 对象 */                \
static struct stby_struct __STBY##x = {          \
    .__GPIOx      = GPIOx,                       \
    .__GPIO_Pin   = PIN,                         \
    .__GPIO_Speed = SPEED,                       \
    .__GPIO_Mode  = MODE,                        \
};
 

	#define ENCODER_GPIO_DESC(x, idx, GPIOx, PIN, SPEED, MODE)      \
/* 定义静态的 encoder_gpio_struct 对象（idx: 1-A相, 2-B相） */  \
static struct encoder_gpio_struct __ENC_GPIO##x##_##idx = {     \
    .__GPIOx      = GPIOx,                                       \
    .__GPIO_Pin   = PIN,                                         \
    .__GPIO_Speed = SPEED,                                       \
    .__GPIO_Mode  = MODE,                                        \
};
 
#define ENCODER_EXTI_DESC(x, EXTI_LINE, EXTI_MODE, EXTI_TRIGGER, LINE_CMD)  \
static struct encoder_exti_struct __ENC_EXTI##x = {              \
    .__EXTI_Line    = EXTI_LINE,                                 \
    .__EXTI_Mode    = EXTI_MODE,                                 \
    .__EXTI_Trigger = EXTI_TRIGGER,                              \
    .__EXTI_LineCmd = LINE_CMD,                                  \
};
 
#define ENCODER_NVIC_DESC(x, IRQ_CHANNEL)                        \
static struct encoder_nvic_struct __ENC_NVIC##x = {              \
    .__NVIC_IRQChannel = IRQ_CHANNEL,                            \
};
 
#define ENCODER_DESC(x, IS_INTERRUPT, FORWARD)                   \
/* 定义静态的 encoder_struct 对象，自动挂上同号的 gpio/exti/nvic */  \
static struct encoder_struct __ENCODER##x = {                    \
    .__gpio_1       = &__ENC_GPIO##x##_1,                        \
    .__gpio_2       = &__ENC_GPIO##x##_2,                        \
    .__exti         = &__ENC_EXTI##x,                            \
    .__is_interrupt = IS_INTERRUPT,                              \
    .__nvic         = &__ENC_NVIC##x,                            \
    .forward        = FORWARD,                                   \
};

	/* ================= 左编码器：A = PB14，B = PB15 ================= */
ENCODER_GPIO_DESC(1, 1, GPIOB, GPIO_Pin_14, GPIO_Speed_2MHz, GPIO_Mode_IPU)
ENCODER_GPIO_DESC(1, 2, GPIOB, GPIO_Pin_15, GPIO_Speed_2MHz, GPIO_Mode_IPU)
ENCODER_EXTI_DESC(1, EXTI_Line14, EXTI_Mode_Interrupt, EXTI_Trigger_Rising_Falling, ENABLE)
ENCODER_NVIC_DESC(1, EXTI15_10_IRQn)
ENCODER_DESC(1, true, EN_LEFT)
 
/* ================= 右编码器：A = PB3，B = PB4 ================= */
ENCODER_GPIO_DESC(2, 1, GPIOB, GPIO_Pin_3, GPIO_Speed_2MHz, GPIO_Mode_IPU)
ENCODER_GPIO_DESC(2, 2, GPIOB, GPIO_Pin_4, GPIO_Speed_2MHz, GPIO_Mode_IPU)
ENCODER_EXTI_DESC(2, EXTI_Line3, EXTI_Mode_Interrupt, EXTI_Trigger_Rising_Falling, ENABLE)
ENCODER_NVIC_DESC(2, EXTI3_IRQn)
ENCODER_DESC(2, true, EN_RIGHT)

encoder_handler encoder_left=&__ENCODER1;
encoder_handler encoder_right=&__ENCODER2;


#define SOFT_I2C_DESC(x, PORT_SCL, PIN_SCL, PORT_SDA, PIN_SDA)  \
/* 定义静态的 soft_i2c 对象 */                                   \
static struct soft_i2c __SOFT_I2C##x = {                         \
    .Port_scl = PORT_SCL,                                        \
    .Pin_scl  = PIN_SCL,                                         \
    .Port_sda = PORT_SDA,                                        \
    .Pin_sda  = PIN_SDA,                                         \
};

SOFT_I2C_DESC(1, GPIOB, GPIO_Pin_8, GPIOB, GPIO_Pin_9)   /* SCL = PB8，SDA = PB9 */         

	#define MPU6050_DESC(x, HARDWARE_I2C_HANDLER)       \
/* 定义静态的 mpu6050_struct 对象 */             \
static struct mpu6050_struct __MPU6050##x = {    \
					.__hard_i2c=HARDWARE_I2C_HANDLER,\
};



	#define PID_DESC(x, KP, KI, KD, SP, ULIMIT, LLIMIT,FORWARD)  \
/* 定义静态的 pid_Typedef 对象 */                    \
static struct pid_Typedef __PID##x = {               \
    /* ---------- 整定参数 ---------- */             \
    .Kp = KP,                                        \
    .Ki = KI,                                        \
    .Kd = KD,                                        \
    .Sp = SP,                                        \
    .uLimit = ULIMIT,                                \
    .lLimit = LLIMIT,                                \
    /* ---------- 运行时状态：静态对象自动清零，写出来只为对照 PID_Init ---------- */ \
    .last_tick    = 0,                               \
    .err_last     = 0.0f,                            \
    .err_last_int = 0.0f,                            \
		.forward      =FORWARD,														\
};

PID_DESC(1, 0.5f, 7.0f, 0.0f, 0.0f, +8.4f, -8.4f,PID_LEFT)   /* 左电机：PID_Init(&pid_motor_l, 0.5, 7, 0) + 限幅 ±8.4 */
PID_DESC(2, 0.5f, 7.0f, 0.0f, 0.0f, +8.4f, -8.4f,PID_RIGHT)   /* 右电机：同上 */

//pid_handler pid_handler_L=&__PID1;
//pid_handler pid_handler_R=&__PID2;

#define MOTOR_SIDE_DESC(x, PID_HANDLER, ENCODER_HANDLER, PWM_HANDLER)  \
/* 定义静态的 motor_side 对象：一路电机的 PID + 编码器 + PWM */        \
static struct motor_side __MOTOR_SIDE##x = {                           \
    .pid     = PID_HANDLER,                                            \
    .encoder = ENCODER_HANDLER,                                        \
    .pwm     = PWM_HANDLER,                                            \
};
 
#define MOTOR_DESC(x, LEFT_HANDLER, RIGHT_HANDLER,ADC)                     \
/* 定义静态的 motor_Typedef 对象，挂上左右两侧 */                       \
static struct motor_Typedef __MOTOR##x = {                             \
    .motor_left  = LEFT_HANDLER,                                       \
    .motor_right = RIGHT_HANDLER,                                      \
		.adc_vbat    = ADC,																									\
};

	/* ---------- 左右两侧 ---------- */
MOTOR_SIDE_DESC(1, &__PID1, &__ENCODER1,  &__PWM1)   /* 左：PID1 + 左编码器 + PWM1 */
MOTOR_SIDE_DESC(2, &__PID2, &__ENCODER2, 	&__PWM2)   /* 右：PID2 + 右编码器 + PWM2 */
 
/* ---------- 整车 ---------- */
MOTOR_DESC(1, &__MOTOR_SIDE1, &__MOTOR_SIDE2,&__ADC1)
motor_handler motor_handler_my=&__MOTOR1;


PID_DESC(3, 4.0f,  0.0f,  0.0f, 0.0f, +12.57f, -12.57f, PID_BOTH)   /* θ 环   */
PID_DESC(4, 10.0f, 10.0f, 0.0f, 0.0f, +125.7f, -125.7f, PID_BOTH)   /* θ̇ 环 */

pid_handler pid_theta=&__PID3;
pid_handler pid_theta_dot=&__PID4;

#define app_control_DESC(x,PID_THETA,PID_THETA_DOT,MPU,MOTOR,PID_VELOCITY,PID_TURN)\
static struct app_control_TypeDef __app_control##x={\
					.pid_theta			=PID_THETA,\
					.pid_theta_dot	=PID_THETA_DOT,\
					.mpu          	=MPU,\
					.motor   				=MOTOR,\
					.pid_velocity		=PID_VELOCITY,\
					.pid_turn       =PID_TURN,\
};\


PID_DESC(5, 10.0f, 1.0f, 0.0f, 0.0f, +0.5f*G, -0.5*G, PID_BOTH)
pid_handler pid_velocity=&__PID5;



	/* ---------- I2C 基本配置 ---------- */
#define I2C_BASE_DESC(x, I2C_X, CLOCK_SPEED, DUTY_CYCLE, MODE, OWN_ADDR1, ACK, ACK_ADDR) \
static struct i2c_base_struct __I2C_BASE##x = {                                          \
    .__I2C_x                   = I2C_X,                                                  \
    .__I2C_ClockSpeed          = CLOCK_SPEED,                                            \
    .__I2C_DutyCycle           = DUTY_CYCLE,                                             \
    .__I2C_Mode                = MODE,                                                   \
    .__I2C_OwnAddress1         = OWN_ADDR1,                                              \
    .__I2C_Ack                 = ACK,                                                    \
    .__I2C_AcknowledgedAddress = ACK_ADDR,                                               \
};
 
/* ---------- SCL/SDA 引脚 ---------- */
#define I2C_GPIO_DESC(x, SCL_PORT, SCL_PIN, SDA_PORT, SDA_PIN, SCL_SPEED, SCL_MODE, SDA_SPEED, SDA_MODE) \
static struct i2c_gpio_struct __I2C_GPIO##x = {                                                          \
    .__GPIOx_SCL      = SCL_PORT,                                                                        \
    .__GPIO_Pin_SCL   = SCL_PIN,                                                                         \
    .__GPIOx_SDA      = SDA_PORT,                                                                        \
    .__GPIO_Pin_SDA   = SDA_PIN,                                                                         \
    .__GPIO_Speed_Scl = SCL_SPEED,                                                                       \
    .__GPIO_Mode_Scl  = SCL_MODE,                                                                        \
    .__GPIO_Speed_Sda = SDA_SPEED,                                                                       \
    .__GPIO_Mode_Sda  = SDA_MODE,                                                                        \
};
 
/* ---------- 顶层 ---------- */
#define HARD_I2C_DESC(x, BASE, GPIO, REMAP)      \
static struct hard_i2c_Typedef __HARD_I2C##x = { \
    .__i2c_base = BASE,                          \
    .__i2c_gpio = GPIO,                          \
    .__remap    = REMAP,                         \
};

I2C_BASE_DESC(1, I2C1, 400000, I2C_DutyCycle_2, I2C_Mode_I2C,
              0x0000, I2C_Ack_Enable, I2C_AcknowledgedAddress_7bit)
 
I2C_GPIO_DESC(1, GPIOB, GPIO_Pin_8, GPIOB, GPIO_Pin_9,
              GPIO_Speed_2MHz, GPIO_Mode_AF_OD,
              GPIO_Speed_2MHz, GPIO_Mode_AF_OD)
 
HARD_I2C_DESC(1, &__I2C_BASE1, &__I2C_GPIO1, 1)   /* 1 = 重映射到 PB8/PB9 */
 
MPU6050_DESC(1, &__HARD_I2C1);
mpu6050_handler mpu6050_1=&__MPU60501;


	/* ============ USART 的 NVIC 描述（对应 usart_nvic_struct） ============ */
#define USART_NVIC_DESC(x, IRQ_CHANNEL,USART_IT_TYPE)                     \
static struct usart_nvic_struct __USART_NVIC##x = {         \
    .irqn = (uint8_t)(IRQ_CHANNEL),                         \
		.usart_it_type =USART_IT_TYPE,													\
};
 
/* ============ 遥控顶层描述（对应 app_rc_TypeDef） ============ */
#define APP_RC_DESC(x, USART, USART_NVIC, PID_VELOCITY, PID_TURN)  \
static struct app_rc_TypeDef __APP_RC##x = {                       \
    .__usart        = (USART),                                     \
    .__usart_nvic   = (USART_NVIC),                                \
    .__pid_velocity = (PID_VELOCITY),                              \
    .__pid_turn     = (PID_TURN),                                  \
};

/* ================= 遥控串口：USART3 = PB10(TX)/PB11(RX) ================= */
USART_DESC(2, USART3, GPIOB,
           9600, USART_WordLength_8b, USART_StopBits_1, USART_Parity_No,
           USART_Mode_Tx | USART_Mode_Rx, USART_HardwareFlowControl_None,
           GPIO_Pin_10, GPIO_Speed_2MHz, GPIO_Mode_AF_PP,   /* TX */
           GPIO_Pin_11, GPIO_Speed_2MHz, GPIO_Mode_IPU);    /* RX */
usart_handler usart_rc = &__USART2;   /* 描述对象编号 2，实际外设是 USART3 */

USART_NVIC_DESC(1, USART3_IRQn,USART_IT_RXNE)
usart_nvic_handler usart_nvic_rc = &__USART_NVIC1;

/* ================= 转向环 PID ================= */
PID_DESC(6, 1.0f, 0.0f, 0.0f, 0.0f, +15.0f, -15.0f, PID_BOTH)
pid_handler pid_turn = &__PID6;

	/* ================= 遥控顶层 ================= */
APP_RC_DESC(1, &__USART2, &__USART_NVIC1, &__PID5, &__PID6);
app_rc_handler app_rc_1 = &__APP_RC1;

app_control_DESC(1,&__PID3,&__PID4,&__MPU60501,&__MOTOR1,&__PID5,&__PID6);
app_control_handler app_control_1=&__app_control1;



/**
 * @brief  板级初始化：开启所有外设时钟，配置中断优先级分组
 * @retval None
 */
void board_init(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);
	RCC_ADCCLKConfig(RCC_PCLK2_Div6);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1,ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1,ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3,ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3,ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1,ENABLE);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
}
