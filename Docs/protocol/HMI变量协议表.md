# HMI 变量协议表

> 适用于 DWIN 串口 HMI。若使用 PC Qt 上位机，应另建 Qt 串口协议表。

| 变量地址 | 变量名 | 数据类型 | 单位 | 来源 | 刷新周期 | 读写 | 说明 |
|---|---|---|---|---|---|---|---|
| 待定 | system_status | uint16 | - | CNS_State | 500ms | 只读 | 系统总状态 |
| 待定 | battery_voltage | uint16 | 0.01V | Power | 1000ms | 只读 | 电池电压 |
| 待定 | gnss_fix | uint16 | - | GNSS | 1000ms | 只读 | 定位状态 |
| 待定 | latitude | int32 | 1e-7 deg | GNSS | 1000ms | 只读 | 纬度 |
| 待定 | longitude | int32 | 1e-7 deg | GNSS | 1000ms | 只读 | 经度 |
| 待定 | temperature | int16 | 0.1C | BME280 | 1000ms | 只读 | 温度 |
| 待定 | humidity | uint16 | 0.1%RH | BME280 | 1000ms | 只读 | 湿度 |
| 待定 | lora_status | uint16 | - | LoRa | 500ms | 只读 | 通信状态 |
| 待定 | lora_loss_rate | uint16 | 0.1% | LoRa | 1000ms | 只读 | 丢包率 |
| 待定 | alarm_code | uint16 | - | Alarm | 200ms | 只读 | 当前标准错误码，使用 `App_ErrorCode_t` |
| 待定 | log_enable | uint16 | - | HMI/Param | 事件 | 读写 | 日志开关 |
| 待定 | motor_pwm_1 | uint16 | % | HMI/Motor | 事件 | 读写 | 电机 1 占空比 |

## 规则

1. HMI 不直接访问底层模块，只显示 `CNS_State` 或参数管理层数据。
2. 多字节数据需要统一大小端和比例系数。
3. HMI 可写变量必须进入参数管理或命令队列，不允许直接驱动硬件。
