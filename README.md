# 低空 CNS 主控箱 V1.0

## 产品定位

本工程面向低空通信导航监视（CNS）教学实验产品的 V1.0 嵌入式主控箱。

V1.0 的目标不是一次性做完整低空 CNS 平台，而是先交付一个可靠、可验收、可复制的 STM32F407 主控箱底座，把低速采集、状态显示、基础控制、告警和日志记录闭环做稳。

## V1.0 交付边界

V1.0 只聚焦主控箱本体：

- 一个实物主控箱：外壳、供电、电源保护、急停/总电源、接线端子、状态指示和外部接口。
- 一块嵌入式主控：`STM32F407ZGTx`，负责实时控制、安全 IO、底层状态采集和告警。
- 一个本地人机界面：DWIN 串口屏，用于状态、参数、告警、日志开关和基础控制显示。
- 一组直连设备：GNSS、LoRa 数传、MPU6050/BME280、电源检测、按键、蜂鸣器、电机演示和 SD 卡。
- 一套基础固件：上电自检、设备状态采集、页面显示、参数保存、报警处理、CSV 日志和掉电恢复入口。
- 一张接口边界表：明确 V1.0 可由 F407 直连的设备，以及 V2.0 才通过 Linux 网关接入的复杂设备。

## V1.0 暂缓范围

以下内容不压入 V1.0 固件交付：

- 5G-A 高速数据处理
- Mesh、图传、雷达、光电、RTK、UWB 等复杂设备接入
- Web 控制台、地图态势、视频融合和多箱联动
- AI 驾驶员、AI 教官、AI 调度和 AI 运维
- Qt 界面和数据库
- 完整 CNS 综合实训平台

5G-A、Mesh、图传、雷达、光电、RTK、UWB 等设备可以在 V1.0 做接口和结构预留，但高速数据和复杂协议应在 V2.0 的 Linux 网关侧处理，F407 只保留状态读取、控制握手或安全联锁接口。

## 项目信息

- 产品名称：低空 CNS 主控箱 V1.0
- Keil 工程：`MDK-ARM/low_altitude_cns_f407.uvprojx`
- 构建目标：`Target 1`
- 输出文件：`Objects/cns_f407.axf`
- 主控芯片：`STM32F407ZGTx`
- 软件栈：`HAL + FreeRTOS + Keil MDK ARMCC5`

## 目录说明

- `Core`：启动文件、中断、HAL 时基、系统时钟和 `main` 入口
- `Drivers`：CMSIS 与 STM32F4 HAL 驱动源码
- `Middlewares/Source`：Keil 工程实际使用的 FreeRTOS 源码
- `Middlewares/Third_Party`：第三方中间件保留区，后续用于 FatFs/SD 卡等模块
- `App`：任务、状态、告警、日志、协议、存储抽象等应用层代码
- `Sensor_Driver`：传感器驱动适配层，提供统一采样数据结构、驱动接口和传感器注册表
- `Bsp`：UART、CAN、SPI、I2C、SD、GPIO、ADC、PWM 等板级支持代码
- `Docs`：接口表、协议格式、日志规范、测试记录、验收记录等项目文档

## 固件分层

固件保持分层，避免把协议解析、数据转换、显示和日志逻辑散落到业务代码里：

- `Bsp`：只处理板级外设初始化和底层收发，不直接理解 CNS 业务。
- `Sensor_Driver`：处理具体传感器驱动适配、传感器协议解析、量纲转换和统一采样输出。
- `App`：处理任务调度、状态机、系统注册表、告警、日志、协议封装和业务数据流转。
- `Protocol`：负责 CNS 帧解析与封装，为后续管控中心、Linux 网关或上位机预留统一数据出口。
- `Storage`：屏蔽 SDIO/SPI/FatFs 细节，业务任务只写 CSV 记录，不直接操作文件系统。
- `Display`：面向 DWIN 串口屏输出页面变量和状态，不在 F407 上实现复杂 UI 框架。

当前建议的数据流边界：

```text
App
  -> Sensor_RegistryReadAll()
      -> Sensor_Driver_t.Read()
          -> Bsp UART/RS485/CAN/I2C/SPI raw transfer
  -> App_LogWrite() / App_ProtocolEncode() / Display refresh
```

BSP 不返回业务化的传感器数据，只提供稳定的有线通信访问；具体传感器驱动负责把原始字节流转换成统一的 `Sensor_Sample_t`。

## 当前任务设计

当前工程已经建立以下 FreeRTOS 任务入口：

- `system`：系统启动、心跳、自检和状态上报
- `comm`：通信收发入口，后续接入 GNSS、LoRa、CAN 预留等模块
- `protocol`：CNS 数据帧解析与封装
- `logger`：CSV 日志队列处理
- `display`：DWIN 串口屏刷新入口
- `alarm`：异常告警处理入口

后续新增设备时，应优先按任务和模块边界扩展，不要让某个驱动直接更新显示页面、直接写文件或直接拼接外部通信帧。

## 注册表与自检

当前工程包含两级注册表：

- `App_Registry`：系统模块注册表，管理 `system`、`comm`、`protocol`、`logger`、`display`、`alarm`、`storage`、`sensor` 等模块。
- `Sensor_Registry`：传感器驱动注册表，管理 GNSS、气象、电源检测、继电器 IO 等具体传感器或设备驱动；Remote ID 仅作后续扩展预留。

启动流程：

```text
main()
  -> BSP_Init()
  -> App_TasksInit()
      -> App_StatusInit()
      -> App_AlarmInit()
      -> App_RegistryInitAll()
          -> Sensor_RegistryInitAll()
      -> App_RegistrySelfCheckAll()
          -> Sensor_RegistrySelfCheckAll()
      -> create FreeRTOS tasks
  -> vTaskStartScheduler()
```

运行阶段由 `system` 任务周期调用 `App_RegistryPoll()`，系统注册表会按 `check_period_ms` 触发各模块自检。`sensor` 模块的自检入口会继续下沉到 `Sensor_RegistrySelfCheckAll()`。

## 传感器驱动接口

`Sensor_Driver` 目录目前提供统一接口骨架：

- `sensor_data.h`：定义 `Sensor_Sample_t`、`Sensor_Value_t`、`Sensor_Type_t` 和 `Sensor_ValueType_t`
- `sensor_driver.h`：定义 `Sensor_Driver_t`，统一 `init`、`self_check`、`read`、`get_status` 函数指针
- `sensor_registry.h/.c`：统一传感器初始化、自检、批量读取和驱动查询入口

具体传感器驱动应实现一个 `Sensor_Driver_t`：

```c
static const Sensor_Driver_t g_gnss_driver =
{
  .device_id = 1U,
  .sensor_type = SENSOR_TYPE_GNSS,
  .name = "gnss",
  .init = Gnss_Init,
  .self_check = Gnss_SelfCheck,
  .read = Gnss_Read,
  .get_status = Gnss_GetStatus
};
```

然后挂入 `Sensor_Driver/Src/sensor_registry.c` 的 `s_sensor_drivers[]`：

```c
static const Sensor_Driver_t *const s_sensor_drivers[] =
{
  &g_gnss_driver
};
```

采样接口统一返回 `Sensor_Sample_t`，上层不直接依赖具体传感器协议：

```text
GNSS NMEA / BME280 register / power ADC sample
  -> concrete driver parse
  -> Sensor_Sample_t
  -> logger / display / protocol
```

## V1.0 建议直连设备

| 功能 | 建议设备 | 与 F407 的关系 |
| --- | --- | --- |
| 导航定位 | ATGM336H 或同类 GNSS 模块 | UART，解析 NMEA |
| 通信链路演示 | LoRa 数传模块 | UART，透明传输/链路状态 |
| 姿态/环境 | MPU6050、BME280 | I2C |
| 显示交互 | DWIN 5 寸串口 HMI | UART，页面变量刷新 |
| 日志存储 | MicroSD 卡 | 优先 SDIO，必要时退回 SPI |
| 电源状态 | 电池电压/电源分支检测 | ADC + 分压保护 |
| 执行演示 | 低速直流电机 + 驱动板 | PWM + GPIO，不做真实飞控 |
| 安全告警 | 蜂鸣器、急停/总电源、状态灯 | GPIO/PWM/外部硬件联锁 |

Remote ID 在 V1.0 只做接口、结构和电气预留，不作为必须验收的直连设备；真实协议接入和外场验证放入 V1.1 或后续版本。

## 日志与 SD 卡导出

日志模块已经和存储后端解耦：

```text
App_LogWrite()
  -> 日志队列
  -> App_LoggerTask
  -> App_StorageWriteCsvLine()
  -> 后续接入 FatFs/SD 卡
```

V1.0 默认采用 filesystem-based storage，优先输出 CSV，便于学生导出数据、写实验报告和复盘调试过程。

建议后续固定以下日志字段：

- `timestamp_ms`
- `level`
- `module`
- `field`
- `value`
- `unit`
- `status`
- `error_code`

## 告警与自检

V1.0 至少保留以下入口：

- 上电自检：主控、显示、存储、GNSS、LoRa、传感器、电源状态。
- 运行心跳：各任务通过状态模块维护心跳和在线状态。
- 告警来源：模块离线、初始化失败、通信超时、存储不可用、低电量、堆内存失败、栈溢出。
- 告警输出：DWIN 页面提示、蜂鸣器、状态灯、CSV 日志记录。

## 后续路线

### V1.0：主控箱底座

先证明嵌入式主控箱能稳定采集、显示、控制、报警和记录。

### V2.0：边缘网关主控箱

加入 Linux 网关，接入 5G、Mesh、图传、RTK、UWB 等复杂设备。STM32 继续负责实时控制、安全 IO 和底层状态采集，Linux 侧负责复杂协议、网络、视频和设备适配。

### V3.0：CNS 综合实训平台

扩展为多个主控箱、CNS 设备池、Web/大屏平台和 AI 教官/调度/运维能力，形成完整实验室产品体系。
