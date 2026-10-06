# STM32 平衡小车 (FreeRTOS)

基于 STM32F103C8T6 的两轮自平衡小车，MPU6050 姿态解算 + PID 控制 + 蓝牙遥控。

## 硬件平台

- MCU: STM32F103C8T6
- IMU: MPU6050 (I2C)
- 电机: 直流减速电机 + 编码器
- 遥控: 蓝牙串口 (USART)
- 电池: 锂电池电压采集

## 功能

- FreeRTOS 多任务: 平衡控制 5ms / 速度环 / 遥控解析 / 慢任务
- 串级 PID: 直立环 (Pitch) + 速度环 + 转向环
- MPU6050 DMP 姿态解算
- 蓝牙遥控: 前进/后退/转向，50ms 超时保护
- 电池电压补偿 PWM 占空比

## 工程结构

```
├── template/
│   ├── user/
│   │   ├── src/       # 应用层: app_control, app_motor, app_rc, app_mpu6050
│   │   └── inc/       # 头文件
│   └── mdk/           # Keil 工程
└── FreeRTOS移植改动文档.md
```

## 编译

- Keil 5 + ARMCC 5.06
- 工程文件: `template/balance_car.uvprojx`
