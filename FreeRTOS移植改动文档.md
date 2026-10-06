# 平衡车 FreeRTOS 移植 —— 完整改动文档（改前 / 改后对照）

> 本文档只描述改动，不改任何代码。按 **编译依赖顺序** 排列：改一个文件、编一次、过了再改下一个。
> 参考项目：天气时钟（分层 + work_queue + 软定时器模式）、OTA 项目（收指针设计）。

---

## 0. 总体架构

### 0.1 分层规则（照抄天气时钟）
```
编排层   main.c            —— 只负责建任务、启动调度器，全局只剩 state 变量
  ↓
应用层   app_motor / app_contorl / app_rc / app_mpu6050 /
        app_vbat / app_slow / work_queue      —— 每层只调用下层提供的接口
  ↓
驱动层   hardware_i2c / pid / app_encoder / app_pwm / my_usart /
        delay / timer / adc / my_button / bat_led      —— 零 FreeRTOS 依赖
```
- 驱动层 ISR 需要通知任务时，用**函数指针回调**上抛，由应用层回调内部调用 `FromISR` API。
- 队列/信号量句柄全部 `static` 在模块内部，**main.c 不出现任何句柄**。

### 0.2 任务划分表
| 任务 | 优先级 | 周期 | 职能 |
|---|---|---|---|
| init_task | 4 | 一次性 | Delay_Init 时基 → 各模块 init → 建队列/定时器 → vTaskDelete(NULL) |
| motor_gatekeeper_task | 4 | 1ms | **唯一**持有电机 PWM + 电机 PID；消费命令队列；速度环 |
| balance_task | 3 | 5ms | 直立/速度/转向控制律；pull 遥控目标 |
| rc_task | 2 | 事件（50ms 看门狗） | 解析 DMA+IDLE 收到的遥控行；超时清零自己的目标 |
| work_queue_task | 1 | 事件 | 消费软件定时器 push 的慢速工作（电池/按键） |
| 定时器守护任务 | 3（内核） | — | 运行软件定时器回调（只 push work_queue） |

优先级原则：周期最短/截止期最硬的最优先（gatekeeper 4 > balance 3 > rc 2 > work_queue 1）。

### 0.3 中断归属与优先级表
| 中断 | 归属模块 | 抢占优先级 | 说明 |
|---|---|---|---|
| EXTI3 / EXTI15_10 | app_encoder | **4** | 故意高于内核阈值 5，不调 FreeRTOS API |
| I2C1_EV / I2C1_ER | hardware_i2c | **5** | 状态机突发读 MPU；done 回调内 GiveFromISR |
| USART3（仅 IDLE） | app_rc | **5** | DMA 断帧；xQueueSendFromISR |
| TIM3 | delay.c | 15 | 1ms 时基（timer.c 已写死 15） |
| SVC / PendSV / SysTick | port.c | 内核 | 向量表直接指向内核实现，用户不得定义 |

> 优先级数值：数字越小越优先。内核屏蔽阈值 = configMAX_SYSCALL_INTERRUPT_PRIORITY = 5。
> 优先级 ≤ 5（即 5 及以上数值）的 ISR 才能安全调用 `*FromISR` 接口。
> 编码器 EXTI = 4（比 5 小，会打断内核临界区，但它不调用任何 FreeRTOS API，合法）。

### 0.4 数据流（一句话版）
```
rc_task(解析遥控) ──app_rc_get_target──> balance_task(控制律)
                                          ├─app_motor_req_set_omega/turn──> gatekeeper 命令队列
mpu6050_sample(I2C IT+信号量) ──GetPitch/GetGx/GetGz──> balance_task
app_slow(500ms) ──work_queue──> app_vbat_bat_work(更新电压缓存) ──get_cached──> gatekeeper
按键回调 ──app_motor_req_enable/disable──> gatekeeper 命令队列
```

---

## 1. `third_lib\freertos\portable\FreeRTOSConfig.h` —— 已就绪，无需改动

当前文件已经是最终配置，**不要动**。核对清单（确认值）：

| 宏 | 当前值 | 作用 |
|---|---|---|
| configCPU_CLOCK_HZ | (SystemCoreClock) | 72MHz |
| configTICK_RATE_HZ | 1000 | 1ms 一个 tick，vTaskDelayUntil 才能精确 1ms/5ms |
| configMAX_PRIORITIES | 5 | 任务优先级 0~4 |
| configTOTAL_HEAP_SIZE | (1024*12) | 12KB 堆 |
| configKERNEL_INTERRUPT_PRIORITY | (15<<4) | 内核中断 15 级 |
| configMAX_SYSCALL_INTERRUPT_PRIORITY | (5<<4) | 可调用 FromISR 的阈值=5 级 |
| configCHECK_FOR_STACK_OVERFLOW | 2 | 栈溢出检查 |
| configUSE_TIMERS | 1 | 软件定时器（app_slow 依赖） |
| configUSE_MALLOC_FAILED_HOOK | 1 | main.c 已有 hook |

---

## 2. `startup\startup_stm32f10x_md.s` —— 向量表 3 处指向内核

C8 为中等容量，工程实际用的启动文件若是其它密度版本（hd/xl…），行号略有差异，改动内容相同。

**改前**（第 74/77/78 行）：
```asm
                DCD     SVC_Handler                ; SVCall Handler
                DCD     PendSV_Handler             ; PendSV Handler
                DCD     SysTick_Handler            ; SysTick Handler
```

**改后**：
```asm
                DCD     vPortSVCHandler            ; SVCall Handler (FreeRTOS)
                DCD     xPortPendSVHandler         ; PendSV Handler (FreeRTOS)
                DCD     xPortSysTickHandler        ; SysTick Handler (FreeRTOS)
```

同时在文件顶部 IMPORT 区（原第 133~134 行 `IMPORT __main / IMPORT SystemInit` 附近）加三行：
```asm
    IMPORT  vPortSVCHandler
    IMPORT  xPortPendSVHandler
    IMPORT  xPortSysTickHandler
```

说明：
- 启动文件里 `SVC_Handler / PendSV_Handler / SysTick_Handler` 是 `[WEAK]` 空转死循环（`B .`）。
- 不改向量表的话，FreeRTOS 的 tick/切换永远不会触发，第一次 vTaskDelay 就卡死。
- 改完后这三个 WEAK 空 handler 已无人引用，可留可删（留着不占链接空间）。
- 这也是"用户自定义 SysTick/PendSV/SVC 处理函数会冲突"的根源：向量表只能有一个入口。

---

## 3. `user\stm32f10x_it.c` —— 删 3 个内核中断

**改前**（3 处，行号见右）：
```c
/* 第 108~111 行 */
#ifndef RTE_CMSIS_RTOS_RTX
void SVC_Handler(void)
{
}
#endif

/* 第 128~131 行 */
#ifndef RTE_CMSIS_RTOS_RTX
void PendSV_Handler(void)
{
}
#endif

/* 第 163~169 行 */
void SysTick_Handler(void)
{
//	if(SysTick->CTRL & SysTick_CTRL_COUNTFLAG)
//	{
//		ulTicks++;
//	}
}
```

**改后**：以上 3 段整体删除（第 138~144 行那段 SysTick_Handler 本来就是注释块，也删掉或不动都行）。

说明：向量表（第 2 步）已指向 port.c 的 `vPortSVCHandler / xPortPendSVHandler / xPortSysTickHandler`，这 3 个用户空壳函数留着没有任何作用，且 SVC/PendSV 空壳还会在调度器启动时踩坏上下文。

---

## 4. `user\src\delay.c` + `user\inc\delay.h` —— 修复时基

### 4.1 先确认板级 TIM3 配置（board.c 已就绪，不要动）
```c
/* board.c 第 122~125 行 */
/* ===== 微秒时基 TIM3: 72MHz/72=1MHz, ARR=65535, 无OC无TRGO ===== */   ← 注释是旧文案，别信
TIMER_BASE_DESC( 3 , TIM3, 71 , TIM_CounterMode_Up, 1000-1 , TIM_CKD_DIV1, 0 )
TIMER_NVIC_DESC(3,TIM3_IRQn)
TIMER_DESC_NO_OC( 3 , TIM_TRGOSource_Reset,&__TIM_NVIC3)
timer_handler timer_us=&__TIM3;
```
实际参数：PSC=71 → 1MHz 计数；ARR=999 → **每 1000 个计数 = 1ms 溢出一次**。
所以 `ulTicks` 的单位就是 **ms**。若你以后把 ARR 改成 65535，则溢出周期是 65536us，换算关系全变，本方案按 1ms 设计。

### 4.2 delay.c 改前（当前有 4 个 bug）
```c
__IO uint64_t ulTicks;

void Delay_Init(timer_handler timer_x)
{
	timer_init(timer_x);
}

void TIM3_IRQHandler (void)
{
	if(TIM_GetITStatus(TIM3,TIM_IT_Update)==SET)
	{
		ulTicks++;                          /* bug1: 单位正确（1ms），但见 bug2 */
		TIM_ClearITPendingBit(TIM3,TIM_IT_Update);
	}
}

uint64_t GetTick(void)
{
	return ulTicks;                         /* ms，OK */
}

uint64_t GetUs(void)
{
	uint64_t last_tick=0;
	uint64_t final_tick=0;
	do{
		last_tick=ulTicks;
		final_tick=ulTicks+TIM_GetCounter(TIM3);   /* bug2: ulTicks 是 ms，计数器是 0~999us，单位混了 */
	}while(last_tick!=ulTicks);             /* bug3: do-while 用了，但函数没有 return */
}

void DelayUs(uint64_t us)
{
	uint64_t expire=GetUs()+us+1;
	while(GetUs()<expire);
}
```
- bug1 并不存在（ulTicks++ 每 1ms 一次 = ms，正确）。真正要改的是 bug2、bug3、bug4（缺 Delay 函数）。
- bug4：`Delay()` 被删了，但 `app_mpu6050.c:55` 还调 `Delay(100)`，现在根本编译不过。

### 4.3 delay.c 改后（整文件）
```c
#include "delay.h"
#include "timer.h"
#include <stddef.h>

__IO uint64_t ulTicks;

void Delay_Init(timer_handler timer_x)
{
	timer_init(timer_x);
}

void TIM3_IRQHandler (void)
{
	if(TIM_GetITStatus(TIM3,TIM_IT_Update)==SET)
	{
		ulTicks++;                          /* 每 1ms 溢出一次 → ulTicks 单位是 ms */
		TIM_ClearITPendingBit(TIM3,TIM_IT_Update);
	}
}

uint64_t GetTick(void)
{
	return ulTicks;                         /* ms */
}

uint64_t GetUs(void)
{
	uint64_t last_tick=0;
	uint64_t final_tick=0;
	do{
		last_tick=ulTicks;
		final_tick=ulTicks*1000u+TIM_GetCounter(TIM3);   /* ms*1000 + 计数器(0~999us) = 微秒 */
	}while(last_tick!=ulTicks);
	return final_tick;                      /* bug3 修复：补 return */
}

void DelayUs(uint64_t us)
{
	uint64_t expire=GetUs()+us+1;
	while(GetUs()<expire);
}

/* bug4 修复：恢复 Delay(ms)，用 GetTick 自旋，不依赖 FreeRTOS（保持 delay.c 是纯驱动） */
void Delay(uint64_t ms)
{
	uint64_t expire=GetTick()+ms;
	while(GetTick()<expire);
}
```

### 4.4 delay.h 改后（加一个声明）
```c
void Delay_Init(timer_handler timer_x);
uint64_t GetTick(void);
uint64_t GetUs(void);
void DelayUs(uint64_t us);
void Delay(uint64_t ms);       /* 新增（恢复） */
```

说明：`timer_init(timer_us)` 里 timer.c 会把 TIM3 中断优先级写死为 15（低于内核阈值 5，安全），并开 Update 中断、清计数。`Delay_Init(timer_us)` 的调用点放在 init_task（见第 14 节）。

---

## 5. `user\src\board.c` —— 3 处小改

### 5.1 board_init 开头加 NVIC 优先级分组（改前）
```c
void board_init(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);
	...
```
**改后**：
```c
void board_init(void)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);   /* 全部 4 位都是抢占优先级，内核阈值 5 才有效 */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);
	...
```
> 不分组的话 NVIC 抢占/子优先级混用，configMAX_SYSCALL_INTERRUPT_PRIORITY=5 的断言会挂。
> TIM3/DMA1 时钟（第 536/537 行）**已经在开**，不用加。

### 5.2 遥控串口中断类型 RXNE → IDLE（改前，第 507 行）
```c
USART_NVIC_DESC(1, USART3_IRQn,USART_IT_RXNE)
```
**改后**：
```c
USART_NVIC_DESC(1, USART3_IRQn,USART_IT_IDLE)   /* 接收改 DMA，断帧用 IDLE */
```

### 5.3 删掉旧 motor_cmd 的包含（改前，第 16 行）
```c
#include "hardware_i2c_desc.h"
#include "app_rc_desc.h"
#include "motor_cmd_desc.h"     ← 删
```
**改后**：这一行删除。

---

## 6. 新增 `work_queue` 三件套（照抄天气时钟）

新建 3 个文件，内容与时钟项目**一字不差**（唯一区别：任务优先级按 v3 定为 tskIDLE_PRIORITY+1）。

### 6.1 `user\inc\work_queue_desc.h`
```c
#ifndef __WORK_QUEUE_DESC_H
#define __WORK_QUEUE_DESC_H
typedef void(*work_queue_callback_t)(void*args);
struct work_queue_item{
	work_queue_callback_t work;
	void*args;
};
#endif
```

### 6.2 `user\inc\work_queue.h`
```c
#ifndef __WORK_QUEUE_H_
#define __WORK_QUEUE_H_
typedef void(*work_queue_callback_t)(void*args);
void work_queue_init(void);
void work_queue_push(work_queue_callback_t func_callback
	,void*args);
#endif
```

### 6.3 `user\src\work_queue.c`
```c
#include "work_queue.h"
#include "work_queue_desc.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

static QueueHandle_t queue_handler=NULL;

static void work_queue_task(void *p)
{
	while(1)
	{
			struct work_queue_item temp={0};
			xQueueReceive(queue_handler,&temp,portMAX_DELAY);
			temp.work(temp.args);          /* 消费处：一行，无任何判断 */
	}
}

void work_queue_init(void)
{
	queue_handler=xQueueCreate(32,sizeof(struct work_queue_item));
	configASSERT(queue_handler);
	xTaskCreate(work_queue_task,"work_queue_task"
	,configMINIMAL_STACK_SIZE*2,NULL,tskIDLE_PRIORITY+1,NULL);   /* v3: 优先级 1 */
}

void work_queue_push(work_queue_callback_t func_callback,void*args)
{
	struct work_queue_item temp={0};
	if(func_callback&&args)      /* 注意：args 为 NULL 会被丢弃，调用方要保证非 NULL */
	{
		temp.work=func_callback;
		temp.args=args;
		xQueueSend(queue_handler,&temp,0);
	}
}

---

## 7. `user\src\app_motor.c` + `user\inc\app_motor.h` —— 守门员任务

### 7.1 app_motor.h 改后（整个文件）

```c
#ifndef __APP_MOTOR_H_
#define __APP_MOTOR_H_
#include <stdbool.h>
#include <stdint.h>
struct motor_Typedef;
typedef struct motor_Typedef* motor_handler;
typedef enum{
	MOTOR_LEFT,
	MOTOR_RIGHT,
	MOTOR_BOTH,
}motor_forward_t;
void app_motor_init(motor_handler motor_x);
uint8_t app_motor_getomega(motor_handler motor_x,motor_forward_t forward,float*res,uint8_t size);
void app_motor_start(void);               /* 新增：建命令队列（init_task 调） */
void app_motor_req_set_omega(float omega);/* 新增：入队，不碰电机 */
void app_motor_req_set_turn(float turn);  /* 新增：入队，不碰电机 */
void app_motor_req_enable(void);          /* 新增：入队 */
void app_motor_req_disable(void);         /* 新增：入队 */
void app_motor_req_reset(void);           /* 新增：入队 */
void motor_gatekeeper_task(void*p);       /* 新增：main.c 建任务 */
#endif
```
删除：`app_motor_cmd`、`app_motor_set_omega`、`app_motor_proc` 三个声明。

### 7.2 app_motor.c 改前（要删除的 3 个函数）

`app_motor_cmd`（原 32~46 行）、`app_motor_set_omega`（原 48~82 行）、`app_motor_proc`（原 84~115 行）整段删除。要点：
- 原 76~77 行是 `app_contorl.c` 调用的 `app_motor_set_omega(LEFT,omega_ref+omega_diff)` 那两行，后面改走命令队列。
- `app_motor_proc` 的 1ms 节拍判断（GetTick）改为 gatekeeper 的 `vTaskDelayUntil`。

### 7.3 app_motor.c 改后（整个文件）

```c
#include "app_motor.h"
#include "app_motor_desc.h"
#include "app_encoder.h"
#include "app_pwm.h"
#include "app_vbat.h"
#include "pid.h"
#include "work_queue.h"
#include "work_queue_desc.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stddef.h>

static motor_handler s_motor=NULL;
static QueueHandle_t s_cmd_queue=NULL;    /* 守门员命令队列：句柄不外露 */
static bool s_enabled=false;
static float s_target_omega=0.0f;         /* balance_task 请求的线速度 sp */
static float s_target_turn=0.0f;          /* balance_task 请求的转向 sp */

static void motor_side_init(motor_handler_side motor_side_x);

void app_motor_init(motor_handler motor_x)
{
	if(motor_x==NULL){return;}
	s_motor=motor_x;
	motor_side_init(motor_x->motor_left);
	motor_side_init(motor_x->motor_right);
}

static void motor_side_init(motor_handler_side motor_side_x)
{
	if(motor_side_x==NULL){return;}
	app_encoder_init(motor_side_x->encoder);
	app_pwm_init(motor_side_x->pwm);
}

/* ---------- 命令处理函数：只会在 gatekeeper 上下文里执行 ---------- */

static void do_enable(void)
{
	if(s_motor==NULL||s_motor->motor_left==NULL||s_motor->motor_right==NULL){return;}
	app_pwm_stby_ctl(s_motor->motor_left->pwm,1);
	app_pwm_stby_ctl(s_motor->motor_right->pwm,1);
	pid_reset(s_motor->motor_left->pid);
	pid_reset(s_motor->motor_right->pid);
	s_enabled=true;
}

static void do_disable(void)
{
	if(s_motor==NULL||s_motor->motor_left==NULL||s_motor->motor_right==NULL){return;}
	app_pwm_stby_ctl(s_motor->motor_left->pwm,0);
	app_pwm_stby_ctl(s_motor->motor_right->pwm,0);
	pid_reset(s_motor->motor_left->pid);
	pid_reset(s_motor->motor_right->pid);
	s_enabled=false;
}

static void do_reset(void)
{
	if(s_motor==NULL||s_motor->motor_left==NULL||s_motor->motor_right==NULL){return;}
	pid_reset(s_motor->motor_left->pid);
	pid_reset(s_motor->motor_right->pid);
}

/* 无参命令跳板：把 void(void) 函数指针塞进 work_queue_item（照抄时钟 main_loop.c 的 timer_excute 思路） */
typedef void(*motor_cmd_func_t)(void);
static void motor_cmd_excute(void*p)
{
	((motor_cmd_func_t)p)();
}

/* float → void* 位模式搬运：M3 上两者都是 32 位，union 只做位搬运，不丢 bit */
static void cb_set_omega(void*args)
{
	union{void*p;float f;}un;
	un.p=args;
	s_target_omega=un.f;
}

static void cb_set_turn(void*args)
{
	union{void*p;float f;}un;
	un.p=args;
	s_target_turn=un.f;
}

/* ---------- 对外请求接口：只入队，绝不碰电机 ---------- */
/* 注意：必须直接 xQueueSend。work_queue_push 带 if(func_callback&&args) 检查，
   omega=0 → args=NULL 会被丢掉。所以守门员命令队列不经过 work_queue。 */

void app_motor_req_set_omega(float omega)
{
	union{void*p;float f;}un;
	un.f=omega;
	struct work_queue_item it={cb_set_omega,un.p};
	xQueueSend(s_cmd_queue,&it,0);
}

void app_motor_req_set_turn(float turn)
{
	union{void*p;float f;}un;
	un.f=turn;
	struct work_queue_item it={cb_set_turn,un.p};
	xQueueSend(s_cmd_queue,&it,0);
}

void app_motor_req_enable(void)
{
	struct work_queue_item it={motor_cmd_excute,(void*)do_enable};
	xQueueSend(s_cmd_queue,&it,0);
}

void app_motor_req_disable(void)
{
	struct work_queue_item it={motor_cmd_excute,(void*)do_disable};
	xQueueSend(s_cmd_queue,&it,0);
}

void app_motor_req_reset(void)
{
	struct work_queue_item it={motor_cmd_excute,(void*)do_reset};
	xQueueSend(s_cmd_queue,&it,0);
}

/* ---------- 初始化：建命令队列（init_task 里调用，调度器启动前） ---------- */

void app_motor_start(void)
{
	s_cmd_queue=xQueueCreate(16,sizeof(struct work_queue_item));
	configASSERT(s_cmd_queue);
}

/* ---------- 守门员任务：整个项目唯一碰 PWM / 电机 PID 的地方 ---------- */

void motor_gatekeeper_task(void*p)
{
	(void)p;
	TickType_t xLastWakeTime=xTaskGetTickCount();
	for(;;)
	{
		/* ① 排空命令队列 —— 与天气时钟 ui_task 的 while(xQueueReceive(...,0)) 排空写法一致
		   （对照见 7.4）：有命令先处理，没命令就做自己的 1ms 活 */
		struct work_queue_item it={0};
		while(xQueueReceive(s_cmd_queue,&it,0)==pdTRUE)
		{
			it.work(it.args);          /* 消费处：一行，无任何判断（照抄时钟 work_queue） */
		}
		/* ② 使能时跑 1ms 速度环（原 app_motor_proc 搬来，节拍改用 vTaskDelayUntil） */
		if(s_enabled&&s_motor!=NULL)
		{
			float omega_L=encoder_get_speed(s_motor->motor_left->encoder);
			float omega_R=encoder_get_speed(s_motor->motor_right->encoder);
			/* 左轮目标=线速度+转向角速度，右轮目标=线速度-转向角速度（替代原 app_contorl 里
			   app_motor_set_omega 的双调用） */
			pid_setSp(s_motor->motor_left->pid,s_target_omega+s_target_turn);
			pid_setSp(s_motor->motor_right->pid,s_target_omega-s_target_turn);
			float ua_L=pid_compute(s_motor->motor_left->pid,omega_L);
			float ua_R=pid_compute(s_motor->motor_right->pid,omega_R);
			float vbat=app_vbat_get_cached();   /* 现场读改缓存读：AD 只能 app_vbat 模块碰 */
			app_pwm_motor_set(s_motor->motor_left->pwm,LEFT,vbat?ua_L/vbat*100.0f:0);
			app_pwm_motor_set(s_motor->motor_right->pwm,RIGHT,vbat?ua_R/vbat*100.0f:0);
		}
		vTaskDelayUntil(&xLastWakeTime,pdMS_TO_TICKS(1));   /* 1ms 硬节拍 */
	}
}

uint8_t app_motor_getomega(motor_handler motor_x,motor_forward_t forward,float*res,uint8_t size)
{
	/* 原样保留（balance_task 只读编码器速度，不进命令队列，属于读操作） */
	if(motor_x==NULL||res==0||size==0){return 0;}
	...（原函数体一字不动，不再重复列出）...
}
```

说明：
- 命令队列元素就是 `struct work_queue_item`，消费处 `it.work(it.args)` 与时钟项目一字不差；没有自创任何结构体/联合体嵌结构体。
- float 只在"入队处"和"回调处"用 `union{void*p;float f;}` 做位搬运，这是指针转换，不是新结构体。

### 7.4 gatekeeper 就是天气时钟 ui_task 的结构（对照）

时钟 ui.c 105~151 行的 ui_task 原文（已核对）：

```c
static void ui_task(void*p)
{
	while(1)
	{
		struct ui_message temp={0};
		while(xQueueReceive(queue_ui,&temp,0)==pdTRUE){   /* 先排空命令队列 */
			switch(temp.action){ ...逐条执行... }
		}
		lv_timer_handler();                     /* 再做自己的周期活（LVGL 刷新） */
		vTaskDelay(pdMS_TO_TICKS(5));           /* 最后让出 CPU */
	}
}
```

守门员是同一个骨架，只把"LCD"换成了"电机"：

| 天气时钟 ui_task | 平衡车 motor_gatekeeper_task |
|---|---|
| `while(xQueueReceive(queue_ui,&temp,0)==pdTRUE)` 排空 | `while(xQueueReceive(s_cmd_queue,&it,0)==pdTRUE)` 排空 |
| `switch(temp.action)` 逐条执行 | `it.work(it.args)` 逐条执行 |
| `lv_timer_handler()` 自己的周期活 | `if(s_enabled)` 1ms 速度环 |
| `vTaskDelay(pdMS_TO_TICKS(5))` | `vTaskDelayUntil(&xLastWakeTime,pdMS_TO_TICKS(1))` |
| 独占 LCD 硬件 | 独占 PWM / 电机 PID 硬件 |
| 生产者 = wifi/mqtt 任务调 `ui_set_*`（内部 xQueueSend） | 生产者 = balance_task 调 `app_motor_req_*`（内部 xQueueSend） |

结论：守门员的意义 = ui_task 的意义 —— 一个"独占硬件资源、排队消费命令"的守门人。
任何任务想动电机，只能像想画屏一样把命令放进队列；PWM/PID 永远只有一个任务在碰。

---

## 8. `user\src\hardware_i2c.c` + `user\inc\hardware_i2c.h` —— 中断版突发读

### 8.1 hardware_i2c.h 新增（加在末尾）

```c
/* 中断版完成回调：status>=0 为成功读到的字节数，<0 为错误 */
typedef void(*i2c_done_callback_t)(hard_i2c_handler,int status,void*ctx);
int  hardware_i2c_regReadBytes_IT(hard_i2c_handler,uint8_t addr,uint8_t reg,
	uint8_t*pbuf,uint16_t buf_size,i2c_done_callback_t done,void*ctx);
```

### 8.2 hardware_i2c.c 新增（追加在文件末尾，不改任何现有阻塞函数）

```c
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

typedef enum{
	I2C_IT_IDLE,
	I2C_IT_START1,     /* 等 SB（第一次 START） */
	I2C_IT_ADDR_W,     /* 等 ADDR（写方向） */
	I2C_IT_TXE_REG,    /* 发寄存器地址 */
	I2C_IT_BTF_REG,    /* 寄存器地址发送完（重复 START 前） */
	I2C_IT_START2,     /* 等 SB（重复 START） */
	I2C_IT_ADDR_R,     /* 等 ADDR（读方向） */
	I2C_IT_RD_RXNE,    /* 读前 N-2 字节 */
	I2C_IT_BTF1,       /* 最后两字节：关ACK读+STOP */
	I2C_IT_LAST,       /* 读最后一字节 */
}i2c_it_state_t;

struct i2c_it_ctx{
	hard_i2c_handler i2c;
	uint8_t  addr;
	uint8_t  reg;
	uint8_t* buf;
	uint16_t size;
	uint16_t bytes_read;
	i2c_done_callback_t done;
	void* ctx;
	i2c_it_state_t state;
};
static struct i2c_it_ctx s_it={0};

static void i2c_it_finish(int status)
{
	I2C_TypeDef* i2c=(s_it.i2c!=NULL)?s_it.i2c->__i2c_base->__I2C_x:I2C1;
	I2C_ITConfig(i2c,I2C_IT_EVT|I2C_IT_BUF|I2C_IT_ERR,DISABLE);
	i2c_done_callback_t done=s_it.done;
	void* ctx=s_it.ctx;
	hard_i2c_handler h=s_it.i2c;
	memset(&s_it,0,sizeof(s_it));
	if(done)
	{
		done(h,status,ctx);   /* 上抛：由应用层回调内部调 GiveFromISR */
	}
}

void I2C1_EV_IRQHandler(void)
{
	I2C_TypeDef* i2c=I2C1;
	switch(s_it.state)
	{
		case I2C_IT_START1:
			if(I2C_GetFlagStatus(i2c,I2C_FLAG_SB)==SET)
			{
				I2C_ClearFlag(i2c,I2C_FLAG_AF);
				I2C_SendData(i2c,s_it.addr&0xfe);       /* 写方向寻址 */
				s_it.state=I2C_IT_ADDR_W;
			}
			break;
		case I2C_IT_ADDR_W:
			if(I2C_GetFlagStatus(i2c,I2C_FLAG_ADDR)==SET)
			{
				I2C_ReadRegister(i2c,I2C_Register_SR1);  /* 读 SR1+SR2 清 ADDR */
				I2C_ReadRegister(i2c,I2C_Register_SR2);
				s_it.state=I2C_IT_TXE_REG;
			}
			break;
		case I2C_IT_TXE_REG:
			if(I2C_GetFlagStatus(i2c,I2C_FLAG_TXE)==SET)
			{
				I2C_SendData(i2c,s_it.reg);              /* 发寄存器地址 */
				s_it.state=I2C_IT_BTF_REG;
			}
			break;
		case I2C_IT_BTF_REG:
			if(I2C_GetFlagStatus(i2c,I2C_FLAG_BTF)==SET)
			{
				I2C_GenerateSTART(i2c,ENABLE);           /* 重复起始，换读方向 */
				s_it.state=I2C_IT_START2;
			}
			break;
		case I2C_IT_START2:
			if(I2C_GetFlagStatus(i2c,I2C_FLAG_SB)==SET)
			{
				I2C_ClearFlag(i2c,I2C_FLAG_AF);
				I2C_SendData(i2c,s_it.addr|0x01);        /* 读方向寻址 */
				s_it.state=I2C_IT_ADDR_R;
			}
			break;
		case I2C_IT_ADDR_R:
			if(I2C_GetFlagStatus(i2c,I2C_FLAG_ADDR)==SET)
			{
				if(s_it.size==1)
				{
					/* 单字节特例：先关ACK，临界区清ADDR+STOP（照抄阻塞版 480~502 行） */
					I2C_AcknowledgeConfig(i2c,DISABLE);
					__disable_irq();
					I2C_ReadRegister(i2c,I2C_Register_SR1);
					I2C_ReadRegister(i2c,I2C_Register_SR2);
					I2C_GenerateSTOP(i2c,ENABLE);
					__enable_irq();
					s_it.state=I2C_IT_LAST;
				}
				else
				{
					I2C_AcknowledgeConfig(i2c,ENABLE);
					I2C_ReadRegister(i2c,I2C_Register_SR1);
					I2C_ReadRegister(i2c,I2C_Register_SR2);
					s_it.bytes_read=0;
					s_it.state=I2C_IT_RD_RXNE;
				}
			}
			break;
		case I2C_IT_RD_RXNE:
			if(I2C_GetFlagStatus(i2c,I2C_FLAG_RXNE)==SET)
			{
				if(s_it.bytes_read<s_it.size-2)          /* 只读前 N-2 字节 */
				{
					s_it.buf[s_it.bytes_read++]=I2C_ReceiveData(i2c);
				}
				else                                     /* 读满 N-2，剩余最后两个字节交给 BTF */
				{
					s_it.state=I2C_IT_BTF1;
				}
			}
			break;
		case I2C_IT_BTF1:
			if(I2C_GetFlagStatus(i2c,I2C_FLAG_BTF)==SET) /* DR+移位寄存器都满=最后两字节在场 */
			{
				I2C_AcknowledgeConfig(i2c,DISABLE);      /* 赶在最后一字节的 ACK 位之前关 ACK */
				s_it.buf[s_it.bytes_read++]=I2C_ReceiveData(i2c);   /* 读倒数第二字节 */
				I2C_GenerateSTOP(i2c,ENABLE);
				s_it.state=I2C_IT_LAST;
			}
			break;
		case I2C_IT_LAST:
			if(I2C_GetFlagStatus(i2c,I2C_FLAG_RXNE)==SET)
			{
				s_it.buf[s_it.bytes_read++]=I2C_ReceiveData(i2c);
				i2c_it_finish((int)s_it.size);
			}
			break;
		default:
			break;
	}
}

void I2C1_ER_IRQHandler(void)
{
	I2C_TypeDef* i2c=I2C1;
	if(I2C_GetITStatus(i2c,I2C_IT_AF)!=RESET){I2C_ClearITPendingBit(i2c,I2C_IT_AF);}
	if(I2C_GetITStatus(i2c,I2C_IT_BERR)!=RESET){I2C_ClearITPendingBit(i2c,I2C_IT_BERR);}
	if(I2C_GetITStatus(i2c,I2C_IT_ARLO)!=RESET){I2C_ClearITPendingBit(i2c,I2C_IT_ARLO);}
	I2C_GenerateSTOP(i2c,ENABLE);
	i2c_it_finish(-1);
}

int hardware_i2c_regReadBytes_IT(hard_i2c_handler hard_i2c_x,uint8_t addr
	,uint8_t reg,uint8_t*pbuf,uint16_t buf_size,i2c_done_callback_t done,void*ctx)
{
	if(hard_i2c_x==NULL||pbuf==NULL||buf_size==0||s_it.state!=I2C_IT_IDLE)
	{
		return -1;   /* 忙或参数错：拒绝新事务 */
	}
	I2C_TypeDef* i2c=hard_i2c_x->__i2c_base->__I2C_x;

	s_it.i2c=hard_i2c_x;
	s_it.addr=addr;
	s_it.reg=reg;
	s_it.buf=pbuf;
	s_it.size=buf_size;
	s_it.bytes_read=0;
	s_it.done=done;
	s_it.ctx=ctx;
	s_it.state=I2C_IT_START1;

	/* 优先级 5：≤ 内核阈值，ISR 里可以安全调 FromISR 接口 */
	NVIC_InitTypeDef nvic_InStruct;
	memset(&nvic_InStruct,0,sizeof(NVIC_InitTypeDef));
	nvic_InStruct.NVIC_IRQChannel=I2C1_EV_IRQn;
	nvic_InStruct.NVIC_IRQChannelCmd=ENABLE;
	nvic_InStruct.NVIC_IRQChannelPreemptionPriority=5;
	nvic_InStruct.NVIC_IRQChannelSubPriority=0;
	NVIC_Init(&nvic_InStruct);
	nvic_InStruct.NVIC_IRQChannel=I2C1_ER_IRQn;
	NVIC_Init(&nvic_InStruct);

	/* IT 只在事务启动时使能，finish() 关闭 → 不干扰初始化阶段的阻塞读写 */
	I2C_ITConfig(i2c,I2C_IT_EVT|I2C_IT_BUF|I2C_IT_ERR,ENABLE);
	I2C_GenerateSTART(i2c,ENABLE);
	return 0;
}
```

### 8.3 字节账目说明（N=2/3/14 通吃）

| 状态 | 动作 | 累计读 |
|---|---|---|
| RD_RXNE | 只读 N-2 字节（`bytes_read<N-2` 才读） | N-2 |
| BTF1 | BTF 时 DR 和移位寄存器各有一字节 → 关ACK、读 1 字节、发STOP | N-1 |
| LAST | RXNE 读最后 1 字节 → finish | N |

- `I2C_IT_BUF` 必须开：RXNE/TXE 中断归 ITBUFEN 管，EVT 只管 SB/ADDR/BTF。
- 中断函数名与启动文件 Default_Handler 里的 `[WEAK]` 空壳同名，直接定义即覆盖，**不需要改启动文件**。
- 单字节（size==1）不经过 RD_RXNE/BTF1，在 ADDR_R 直接走临界区关ACK+STOP，与阻塞版行为一致。

---

## 9. `user\src\app_mpu6050.c` + `user\inc\app_mpu6050.h` —— 中断读+信号量

### 9.1 app_mpu6050.h 改后

```c
void app_mpu6050_init(mpu6050_handler mpu6050_x);
void mpu6050_sample(mpu6050_handler mpu_x,uint16_t ms);   /* 替换 mpu6050_proc */
float App_MPU6050_GetAx(mpu6050_handler mpu6050_x);
...（GetAy/GetAz/GetTemperature/GetGx/GetGy/GetGz/GetYaw/GetPitch/GetRoll 原样）...
```
删除 `void mpu6050_proc(...)` 声明。

### 9.2 改前（删除 1 个函数 + 1 个内部函数）

- `app_mpu6050_update`（92~118 行）整段删除，解析逻辑并入 mpu6050_sample。
- `mpu6050_proc`（120~146 行）整段删除，节拍判断由 balance_task 保证，互补滤波并入 mpu6050_sample。
- 注意第 55 行 `Delay(100)` **不用动**：delay.c 修好后即可编译。

### 9.3 改后：新增信号量 + 完成回调 + sample

```c
/* 文件头部新增 includes：hardware_i2c.h 已包含（回调类型 i2c_done_callback_t 定义在 hardware_i2c.h） */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

static SemaphoreHandle_t s_mpu_done_sem=NULL;
static uint8_t s_mpu_buf[14]={0};
static int s_mpu_i2c_status=-1;

void app_mpu6050_init(mpu6050_handler mpu6050_x)
{
	if(mpu6050_x==NULL){return;}
	hardware_i2c_init(mpu6050_x->__hard_i2c);
	s_mpu_done_sem=xSemaphoreCreateBinary();
	configASSERT(s_mpu_done_sem);
	mpu_write_reg(mpu6050_x,MPU6050_PWR_MGMT_1,0x80);
	Delay(100);
	mpu_write_reg(mpu6050_x,MPU6050_PWR_MGMT_1,0x00);
	mpu_write_reg(mpu6050_x,MPU6050_GYRO_CONFIG,0x18);
	mpu_write_reg(mpu6050_x,MPU6050_ACCEL_CONFIG,0x00);
}

/* 驱动层回调：硬件 I2C 中断里调用，只给信号量，不碰 I2C 寄存器 */
static void mpu_i2c_done(hard_i2c_handler i2c,int status,void*ctx)
{
	(void)i2c;(void)ctx;
	s_mpu_i2c_status=status;
	BaseType_t xHigherPriorityTaskWoken=pdFALSE;
	xSemaphoreGiveFromISR(s_mpu_done_sem,&xHigherPriorityTaskWoken);
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/* 采样+融合：balance_task 每 5ms 调一次（原 mpu6050_proc + app_mpu6050_update 合并） */
void mpu6050_sample(mpu6050_handler mpu_x,uint16_t ms)
{
	if(mpu_x==NULL||ms==0||s_mpu_done_sem==NULL){return;}
	static uint64_t last_tick=0;
	uint64_t cur_tick=GetTick();
	if(cur_tick-last_tick<ms){return;}
	last_tick=cur_tick;

	s_mpu_i2c_status=-1;   /* 先复位，防吃到上一次的残留 */
	if(hardware_i2c_regReadBytes_IT(mpu_x->__hard_i2c,MPU6050_WRITEADDRESS
		,MPU6050_ACCEL_XOUT_H,s_mpu_buf,14,mpu_i2c_done,NULL)!=0)
	{
		return;            /* 忙/参数错：本次放弃 */
	}
	if(xSemaphoreTake(s_mpu_done_sem,pdMS_TO_TICKS(5))!=pdTRUE)
	{
		return;            /* 5ms 超时：本次失败，不更新 */
	}
	if(s_mpu_i2c_status!=14){return;}

	/* ---- 解析（原 app_mpu6050_update 的 92~118 行原样搬入） ---- */
	int16_t ax_raw=(int16_t)(((uint16_t)s_mpu_buf[0]<<8)|s_mpu_buf[1]);
	...（ax/ay/az/temperature/gx/gy/gz 换算原样）...

	/* ---- 互补滤波（原 mpu6050_proc 的 130~144 行原样搬入） ---- */
	float yaw_g=yaw+gz*(ms/1000.0f);
	float pitch_g=pitch+gx*(ms/1000.0f);
	float roll_g=roll-gy*(ms/1000.0f);
	float pitch_a=qatan2(ay,az)/3.1415927f*180.0f;
	float roll_a=qatan2(ax,az)/3.1415927f*180.0f;
	yaw=yaw_g;
	pitch=0.95238f*pitch_g+(1-0.95238f)*pitch_a;
	roll=0.95238f*roll_g+(1-0.95238f)*roll_a;
}
```

说明：14 字节突发读约几百微秒，5ms 超时远够；I2C 状态机保证同时只有一个事务，最多残留一次 give，下轮 Take 成功但 status 被复位后再写，检查 `!=14` 即可挡掉脏数据。

---

## 10. `user\src\app_contorl.c` + `user\inc\app_contorl.h` —— balance_task

### 10.1 app_contorl.h 改后

```c
void app_contorl_reset(app_contorl_handler contorl_x);
void balance_task(void*p);      /* 新增：main.c 建任务，参数传 app_contorl_1 */
```
删除 `void app_contorl_proc(...)` 声明。

### 10.2 app_contorl.c 改后（整个文件）

```c
#include "app_contorl.h"
#include "app_contorl_desc.h"
#include "pid.h"
#include "app_mpu6050.h"
#include "app_motor.h"
#include "app_rc.h"
#include "delay.h"
#include "qmath.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stddef.h>

static const float g = 9.8;
static const float lp = 0.062;
static const float rw = 0.032;
static float omega_ref=0.0f;
static uint64_t last_us=0;

/* 原 app_contorl_proc 改名，删掉 5ms 节拍判断（节拍交给 balance_task 的 vTaskDelayUntil） */
static void app_contorl_control(app_contorl_handler contorl_x)
{
	if(contorl_x==NULL||contorl_x->pid_theta==NULL||contorl_x->pid_theta_dot==NULL
		||contorl_x->motor==NULL||contorl_x->mpu==NULL||contorl_x->pid_velocity==NULL
		||contorl_x->pid_turn==NULL)
	{
		return;
	}
	uint64_t cur_us=GetUs();
	float delaT=(cur_us-last_us)*1e-6;

	/* 开头：pull 遥控目标（rc_task 只写自己的变量，这里读） */
	float vel=0.0f,turn=0.0f;
	app_rc_get_target(&vel,&turn);
	pid_setSp(contorl_x->pid_velocity,vel);
	pid_setSp(contorl_x->pid_turn,turn);

	float res[2]={0};
	uint8_t ret_val=app_motor_getomega(contorl_x->motor,MOTOR_BOTH,res,sizeof(res)/sizeof(float));
	if(ret_val<sizeof(res)/sizeof(float)){return;}
	float sum=0;
	for(uint8_t i=0;i<sizeof(res)/sizeof(float);++i){sum+=res[i];}
	float omega=0.5f*sum;
	float theta=App_MPU6050_GetPitch(contorl_x->mpu)*0.0174533f;
	float theta_dot=App_MPU6050_GetGx(contorl_x->mpu)*0.0174533f;
	float x_dot=rw*(omega+theta_dot*(lp+rw)/rw);

	float theta_ref=qatan(pid_compute(contorl_x->pid_velocity,x_dot)/g);
	pid_setSp(contorl_x->pid_theta,theta_ref);

	float pid_theta_dot=pid_compute(contorl_x->pid_theta,theta);
	pid_setSp(contorl_x->pid_theta_dot,pid_theta_dot);
	float pid_theta_dot_dot=pid_compute(contorl_x->pid_theta_dot,theta_dot);
	float x_dot_dot=(g*qsin(theta)-pid_theta_dot_dot*lp)/qcos(theta);

	if(last_us)
	{
		omega_ref+=1.0f/rw*delaT*x_dot_dot;
	}

	/* 转向环 */
	float gz=App_MPU6050_GetGz(contorl_x->mpu)*0.0174533f;
	float omega_diff=pid_compute(contorl_x->pid_turn,gz);

	/* 末尾：不再直接 set_omega，改走守门员命令队列 */
	app_motor_req_set_omega(omega_ref);
	app_motor_req_set_turn(omega_diff);

	last_us=cur_us;
}

/* 5ms 任务：先采 MPU（中断读+信号量），再跑控制律 */
void balance_task(void*p)
{
	app_contorl_handler c=(app_contorl_handler)p;
	TickType_t xLastWakeTime=xTaskGetTickCount();
	for(;;)
	{
		mpu6050_sample(c->mpu,5);   /* 5ms 采样+互补滤波 */
		app_contorl_control(c);     /* 5ms 控制律 */
		vTaskDelayUntil(&xLastWakeTime,pdMS_TO_TICKS(5));
	}
}

void app_contorl_reset(app_contorl_handler contorl_x)
{
	if(contorl_x==NULL){return;}
	last_us=0;
	omega_ref=0;
	pid_reset(contorl_x->pid_theta);
	pid_reset(contorl_x->pid_theta_dot);
	pid_reset(contorl_x->pid_velocity);
}
```

说明：`balance_task` 的任务参数由 main.c 传 `(void*)app_contorl_1`，模块内不再 include board.h。

---

## 11. `user\src\app_rc.c` + `user\inc\app_rc.h` —— DMA 循环接收 + IDLE 断帧

### 11.1 app_rc.h 改后

```c
void app_rc_init(app_rc_handler app_rc_x);
void rc_task(void*p);                     /* 新增 */
void app_rc_get_target(float*vel,float*turn);   /* 新增：pull 模式读取 */
```
删除 `void app_rc_proc(...)` 声明。

### 11.2 改前（删除的内容）

- 三缓冲结构体 `rc_buf_TypeDef`、`rc_buf_init`、`rc_buf_handler_1`（1~62 行）删除。
- `app_rc_usart_nvic_init`（65~80 行）删除：原优先级写死 0，改成在 app_rc_init 内直接配 5。
- `app_rc_proc`（82~127 行）删除：解析逻辑进 rc_task，失控保护改队列超时。
- `USART3_IRQHandler`（129~162 行）删除：RXNE 逐字节收改 DMA+IDLE。

### 11.3 app_rc.c 改后（整个文件）

```c
#include "app_rc.h"
#include "app_rc_desc.h"
#include "my_usart.h"
#include "my_usart_desc.h"
#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define CMD_MAX_LEN 64
#define RC_DMA_BUF_SIZE 64

typedef struct{
	char data[CMD_MAX_LEN];
	uint16_t len;
}rc_line_t;

static app_rc_handler s_rc=NULL;
static QueueHandle_t s_rc_line_queue=NULL;
static float s_target_velocity=0.0f;
static float s_target_turn=0.0f;

static uint8_t s_dma_buf[RC_DMA_BUF_SIZE];
static uint16_t s_last_ndtr=RC_DMA_BUF_SIZE;   /* 上次 IDLE 时 DMA 剩余计数 */

void app_rc_init(app_rc_handler app_rc_x)
{
	if(app_rc_x==NULL){return;}
	s_rc=app_rc_x;
	usart_init(app_rc_x->__usart);   /* my_usart.c 只做 GPIO+波特率+使能，不开任何中断 */

	/* USART3 中断只开 IDLE，优先级 5（≤内核阈值，可 FromISR） */
	NVIC_InitTypeDef nvic_InStruct;
	memset(&nvic_InStruct,0,sizeof(NVIC_InitTypeDef));
	nvic_InStruct.NVIC_IRQChannel=USART3_IRQn;
	nvic_InStruct.NVIC_IRQChannelCmd=ENABLE;
	nvic_InStruct.NVIC_IRQChannelPreemptionPriority=5;
	nvic_InStruct.NVIC_IRQChannelSubPriority=0;
	NVIC_Init(&nvic_InStruct);
	USART_ITConfig(app_rc_x->__usart_x,USART_IT_IDLE,ENABLE);

	/* DMA1_Channel3 循环接收（USART3_RX 固定通道；SPL 直接配，my_usart.h 无 DMA 函数） */
	DMA_DeInit(DMA1_Channel3);
	DMA_InitTypeDef dma_InStruct;
	memset(&dma_InStruct,0,sizeof(dma_InStruct));
	dma_InStruct.DMA_PeripheralBaseAddr=(uint32_t)&app_rc_x->__usart->__usart_x->DR;
	dma_InStruct.DMA_MemoryBaseAddr=(uint32_t)s_dma_buf;
	dma_InStruct.DMA_DIR=DMA_DIR_PeripheralSRC;
	dma_InStruct.DMA_BufferSize=RC_DMA_BUF_SIZE;
	dma_InStruct.DMA_PeripheralInc=DMA_PeripheralInc_Disable;
	dma_InStruct.DMA_MemoryInc=DMA_MemoryInc_Enable;
	dma_InStruct.DMA_PeripheralDataSize=DMA_PeripheralDataSize_Byte;
	dma_InStruct.DMA_MemoryDataSize=DMA_MemoryDataSize_Byte;
	dma_InStruct.DMA_Mode=DMA_Mode_Circular;      /* 环形：DMA 自己回绕，无需 DMA 中断 */
	dma_InStruct.DMA_Priority=DMA_Priority_High;
	dma_InStruct.DMA_M2M=DMA_M2M_Disable;
	DMA_Init(DMA1_Channel3,&dma_InStruct);
	USART_DMACmd(app_rc_x->__usart_x,USART_DMAReq_RX,ENABLE);
	DMA_Cmd(DMA1_Channel3,ENABLE);

	s_rc_line_queue=xQueueCreate(4,sizeof(rc_line_t));
	configASSERT(s_rc_line_queue);
}

void USART3_IRQHandler(void)
{
	if(USART_GetITStatus(USART3,USART_IT_IDLE)==SET)
	{
		/* 注意：USART_ClearITPendingBit 内部会读 DR，那一读可能抢走 DMA 的一个字节
		   —— 已知风险，已接受（只发生在帧边界，最多丢一个字符，下一帧自愈） */
		USART_ClearITPendingBit(USART3,USART_IT_IDLE);

		/* 环形缓冲位置 = (初始 CNDTR - 当前 CNDTR) & 63 */
		uint16_t cur_ndtr=DMA_GetCurrDataCounter(DMA1_Channel3);
		uint16_t cur_pos=(RC_DMA_BUF_SIZE-cur_ndtr)&(RC_DMA_BUF_SIZE-1);
		uint16_t last_pos=(RC_DMA_BUF_SIZE-s_last_ndtr)&(RC_DMA_BUF_SIZE-1);
		s_last_ndtr=cur_ndtr;

		rc_line_t line={0};
		while(last_pos!=cur_pos&&line.len<CMD_MAX_LEN-1)
		{
			line.data[line.len++]=s_dma_buf[last_pos];
			last_pos=(last_pos+1)&(RC_DMA_BUF_SIZE-1);
		}
		line.data[line.len]=0;

		BaseType_t xHigherPriorityTaskWoken=pdFALSE;
		xQueueSendFromISR(s_rc_line_queue,&line,&xHigherPriorityTaskWoken);
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
}

void rc_task(void*p)
{
	(void)p;
	rc_line_t line={0};
	for(;;)
	{
		if(xQueueReceive(s_rc_line_queue,&line,pdMS_TO_TICKS(50))==pdTRUE)
		{
			/* 解析 move 命令（逻辑照搬原 app_rc_proc 的 97~119 行） */
			if(strncasecmp(line.data,"move ",strlen("move "))==0)
			{
				int speed=0,omega_speed=0;
				if(sscanf(line.data+strlen("move "),"%d %d",&omega_speed,&speed)==2)
				{
					s_target_velocity=-speed*0.01f*0.7f;
					s_target_turn=-omega_speed*0.01f*15.0f;
				}
				else
				{
					s_target_velocity=0;
					s_target_turn=0;
				}
			}
			else
			{
				s_target_velocity=0;
				s_target_turn=0;
			}
		}
		else
		{
			/* 50ms 没等到整行 → 失控保护：清自己的目标，不碰别人 */
			s_target_velocity=0;
			s_target_turn=0;
		}
	}
}

void app_rc_get_target(float*vel,float*turn)
{
	if(vel){*vel=s_target_velocity;}
	if(turn){*turn=s_target_turn;}
}
```

说明：
- 失控保护 = rc_task 的 `xQueueReceive` 50ms 超时，不需要 GetTick 轮询。
- app_rc 不再持有 pid 指针（`app_rc_desc` 里的 `__pid_velocity/__pid_turn` 变闲置，可留可删）。
- 第 5.2 节把 board.c 的 desc 从 RXNE 改成 IDLE：`usart_init` 其实不消费这个 desc（my_usart.c 不开中断），改它只为语义一致，真正的 IDLE 使能在上边 app_rc_init 里。

---

## 12. `user\src\app_vbat.c` + `user\inc\app_vbat.h` —— 电压缓存

### 12.1 app_vbat.h 改后

```c
void app_vbat_init(timer_handler timer_x,adc_handler adc_x,bat_led_init_handler led_init_handler);
float app_vbat_get(adc_handler adc_x);
float app_vbat_get_cached(void);        /* 新增：gatekeeper 读电压只走这里 */
void app_vbat_update_cache(adc_handler adc_x);   /* 新增 */
void app_vbat_bat_work(void);           /* 新增：原 app_vbat_proc 改名去参 */
```
删除 `void app_vbat_proc(...)` 声明。

### 12.2 app_vbat.c 改后（整个文件）

```c
#include "app_vbat.h"
#include "timer.h"
#include "adc.h"
#include "bat_led.h"
#include "delay.h"
#include <stdint.h>
#include <stddef.h>

static adc_handler s_vbat_adc=NULL;
static bat_led_init_handler s_vbat_led=NULL;
static float s_vbat_cached=0.0f;

static uint64_t lastTime = 0;   /* 低电压闪烁节拍 */
static uint8_t  stage = 0;

void app_vbat_init(timer_handler timer_x,adc_handler adc_x,bat_led_init_handler led_init_handler)
{
	timer_init(timer_x);
	adc_init(adc_x);
	bat_led_init_s(led_init_handler);
	s_vbat_adc=adc_x;
	s_vbat_led=led_init_handler;
	s_vbat_cached=vbat_get(adc_x);   /* 上电先缓存一次 */
}

float app_vbat_get(adc_handler adc_x)
{
	return vbat_get(adc_x);
}

void app_vbat_update_cache(adc_handler adc_x)
{
	if(adc_x==NULL){return;}
	s_vbat_cached=vbat_get(adc_x);
}

float app_vbat_get_cached(void)
{
	return s_vbat_cached;
}

/* 原 app_vbat_proc 改名去参：LED 逻辑一字不动，句柄改用缓存的 */
void app_vbat_bat_work(void)
{
	if(s_vbat_led==NULL||s_vbat_adc==NULL){return;}
	app_vbat_update_cache(s_vbat_adc);
	float vbat=s_vbat_cached;
	if(vbat>7.9f){...bat_led_set 原样 3 灯全亮...}
	else if(vbat>7.4f){...}
	else if(vbat>7){...}
	else if(vbat>6.5){...}
	else { /* 低电压：GetTick 100ms 闪烁逻辑原样搬入 */ }
}
```

说明：AD 采样现在只发生在 app_vbat 模块内（`app_vbat_bat_work`），gatekeeper 读的是缓存，避免两个任务同时碰 ADC。

---

## 13. 新增 `app_slow` 两件套（软件定时器 + work_queue 慢速事务）

### 13.1 `user\inc\app_slow.h`

```c
#ifndef __APP_SLOW_H_
#define __APP_SLOW_H_
void slow_tasks_init(void);
#endif
```

### 13.2 `user\src\app_slow.c`（照抄时钟 main_loop.c 的软定时器模式）

```c
#include "app_slow.h"
#include "app_vbat.h"
#include "app_button.h"
#include "board.h"            /* my_button_1 */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "work_queue.h"
#include <stddef.h>

typedef void(*callback_func_t)(void);

/* 跳板：把无参函数指针变成 work_queue 的 work(带 args) —— 照抄时钟 */
static void timer_excute(void*p)
{
	callback_func_t func=(callback_func_t)p;
	func();
}

/* 软定时器回调：只 push，不干活 —— 照抄时钟 os_timer_callback_func */
static void os_timer_callback_func(TimerHandle_t timer)
{
	callback_func_t func=(callback_func_t)pvTimerGetTimerID(timer);
	work_queue_push(timer_excute,func);
}

static void bat_work_wrap(void){ app_vbat_bat_work(); }
static void btn_work_wrap(void){ app_button_proc(my_button_1); }

static TimerHandle_t s_bat_timer=NULL;
static TimerHandle_t s_btn_timer=NULL;

void slow_tasks_init(void)
{
	/* timerID 直接存无参函数指针；work_queue_push 的 args 检查由 timer_excute 的跳板绕开 */
	s_bat_timer=xTimerCreate("bat_timer",pdMS_TO_TICKS(500),pdTRUE
		,(void*)bat_work_wrap,os_timer_callback_func);
	s_btn_timer=xTimerCreate("btn_timer",pdMS_TO_TICKS(20),pdTRUE
		,(void*)btn_work_wrap,os_timer_callback_func);
	xTimerStart(s_bat_timer,0);
	xTimerStart(s_btn_timer,0);
}
```

说明：`my_button_1` 是 board.h 里已有的 extern 句柄，app_slow 只负责周期性喂 `app_button_proc`。软定时器回调在定时器守护任务（优先级 3）里执行，只 push 队列，不碰任何外设。

---

## 14. `user\src\main.c` —— 编排层

### 14.1 改前（删除的部分）

- 第 10~11 行两个全局：`QueueHandle_t motor_cmd_queue`、`SemaphoreHandle_t mpu_done_sem` 删除。
- `button_clicked_cb` 里的注释壳（16~25 行）换成真实现。
- 裸机 while(1) 循环（42~50 行）删除。

### 14.2 main.c 改后（整个文件）

```c
#include "board.h"
#include "button_usr_read.h"
#include "app_mpu_test.h"
#include "delay.h"
#include "work_queue.h"
#include "app_slow.h"
#include "FreeRTOS.h"
#include "task.h"

static uint8_t state=0;

static void button_clicked_cb(void*button,uint8_t clicks)
{
	if(!button||clicks!=1)
	{
		return;
	}
	state=!state;
	app_contorl_reset(app_contorl_1);
	if(state)
	{
		app_motor_req_enable();
	}
	else
	{
		app_motor_req_disable();
	}
}

static void init_task(void*p)
{
	(void)p;
	Delay_Init(timer_us);            /* 1. TIM3 时基（1ms） */
	work_queue_init();               /* 2. 慢速队列+任务 */
	usart_init(usart_1);             /* 3. 原 main 里的串口初始化（保留原行为） */
	app_vbat_init(timer_1,adc_1,__bat_led_init);
	app_button_init(my_button_1,button_usr_read,button_clicked_cb);
	app_motor_init(motor_handler_my);
	app_mpu6050_init(mpu6050_1);
	app_rc_init(app_rc_1);
	app_motor_start();               /* 建守门员命令队列 */
	slow_tasks_init();               /* 电池/按键软定时器 */
	vTaskDelete(NULL);               /* 一次性任务自我删除 */
}

int main(void)
{
	board_init();                    /* NVIC 分组+外设时钟（第 5 节） */
	/* 先建 init_task：与 gatekeeper 同为优先级 4，先创建的先运行，
	   保证调度器一起跑时队列已建好 */
	xTaskCreate(init_task,"init_task",configMINIMAL_STACK_SIZE*2,NULL,4,NULL);
	xTaskCreate(motor_gatekeeper_task,"gatekeeper",configMINIMAL_STACK_SIZE*2,NULL,4,NULL);
	xTaskCreate(balance_task,"balance_task",configMINIMAL_STACK_SIZE*2,NULL,3,(void*)app_contorl_1);
	xTaskCreate(rc_task,"rc_task",configMINIMAL_STACK_SIZE*2,NULL,2,NULL);
	vTaskStartScheduler();
	while(1);                        /* 调度器启动后永不返回 */
}

/* 3 个 hook 原样保留 */
void vAssertCalled ( const char *file, int line){ while(1); }
void vApplicationStackOverflowHook (TaskHandle_t t, char *n){ (void)t;(void)n;configASSERT(0); }
void vApplicationMallocFailedHook ( void ){ configASSERT(0); }
```

说明：main.c 不再出现任何队列/信号量句柄；`init_task` 是编排层唯一干"初始化"活的地方，各模块句柄仍由 board.c 持有并通过 board.h extern 暴露。

---

## 15. `user\src\app_encoder.c` —— 中断优先级 0 → 4

改前（第 232 行）：

```c
	nvic_InStructer.NVIC_IRQChannelPreemptionPriority=0;
```

改后：

```c
	nvic_InStructer.NVIC_IRQChannelPreemptionPriority=4;   /* 高于内核阈值5，但本 ISR 不调任何 FreeRTOS API，合法 */
```

说明：编码器 EXTI 测的是脉宽（T 法），要求测量不被 FreeRTOS 临界区延迟污染，所以提到 4 级；它内部只用 GetUs 和 volatile 变量，不碰 FreeRTOS。

---

## 16. 删除旧命令三件套 `motor_cmd`

删除文件：
- `user\src\motor_cmd.c`
- `user\inc\motor_cmd.h`
- `user\inc\motor_cmd_desc.h`

改引用：
- `board.c` 第 16 行 `#include "motor_cmd_desc.h"` 已在第 5.3 节删除。
- `board.h` 第 17 行 `#include "motor_cmd.h"` 删除。

替代关系：`motor_cmd_init/set_type/set_value`（type+value 结构体）全部被第 7 节的 `struct work_queue_item` + 回调命令替代，不再有第二个命令协议。

---

## 17. 编译验证顺序 + 工程文件调整

### 17.1 按依赖顺序，一步一编译

| 步 | 改动 | 验证点 |
|---|---|---|
| 1 | FreeRTOSConfig.h 核对（不动） | 确认 12KB 堆、1000Hz、优先级分组宏 |
| 2 | startup 向量表 3 处 DCD + 3 条 IMPORT | 链接过，无"未定义符号" |
| 3 | stm32f10x_it.c 删 3 个内核空壳 | 链接过（WEAK 空壳删除不影响其它） |
| 4 | delay.c + delay.h 修复 | `app_mpu6050.c:55` 的 Delay(100) 恢复编译 |
| 5 | board.c 3 处 + board.h 删 motor_cmd.h | 编译过 |
| 6 | work_queue 三件套 | 编译过 |
| 7 | app_motor 守门员 | 编译过（依赖 work_queue_desc.h） |
| 8 | hardware_i2c IT | 编译过 |
| 9 | app_mpu6050 | 编译过（依赖 semphr.h） |
| 10 | app_contorl | 编译过（依赖 app_rc_get_target / app_motor_req_* / mpu6050_sample） |
| 11 | app_rc | 编译过 |
| 12 | app_vbat | 编译过 |
| 13 | app_slow | 编译过（依赖 app_button_proc、app_vbat_bat_work） |
| 14 | main.c | 编译过（删了旧全局后无未使用警告以外的错） |
| 15 | app_encoder 优先级 | 编译过 |
| 16 | 删 motor_cmd + .uvprojx 调整 | **最终**编译，0 error |

### 17.2 `.uvprojx` 调整（Keil 工程文件）

- 添加编译单元：`user\src\work_queue.c`、`user\src\app_slow.c`
- 移除编译单元：`user\src\motor_cmd.c`
- 头文件路径：`user\inc` 若已在 include path 则无需改（新头都放那里）

### 17.3 上板自检清单

1. 烧录后 LED/按键任务正常（app_slow 两个定时器在跑）。
2. 遥控发 `move x y`，rc_task 解析后 balance_task 能拉到目标。
3. 按按键 → 状态翻转，电机使能/失能切换（走命令队列）。
4. 断电重开，I2C 读 MPU 的 pitch 曲线连续（中断读无卡死）。
5. 使能后 1ms 速度环节拍用逻辑分析仪/示波器验证 PWM 更新周期稳定。
