# CNS_State 状态表

`CNS_State` 是应用层汇总快照，用于日志记录、HMI 显示和对外上报。

各驱动和业务模块不直接写入 `CNS_State`。传感器驱动通过 `Sensor_Sample_t` 输出采样数据，系统模块通过 `App_ModuleStatus_t` 输出健康状态，告警模块通过告警接口输出当前告警；应用层统一汇总这些数据后生成 `CNS_State` 快照。

## 建议结构

```c
typedef struct {
    SystemStatus_t system;
    PowerStatus_t power;
    GnssData_t gnss;
    SensorData_t sensor;
    LoraStatus_t lora;
    MotorStatus_t motor;
    LogStatus_t log;
    AlarmStatus_t alarm;
} CNS_State_t;
```

## 字段规划

| 分组 | 字段 | 类型 | 说明 |
|---|---|---|---|
| system | uptime_ms | uint32 | 系统运行时间 |
| system | selfcheck_result | uint32 | 自检结果位图 |
| power | battery_mv | uint16 | 电池电压 |
| power | battery_percent | uint8 | 电量估算 |
| gnss | fix_status | uint8 | 定位状态 |
| gnss | latitude | int32 | 纬度，1e-7 度 |
| gnss | longitude | int32 | 经度，1e-7 度 |
| gnss | altitude_mm | int32 | 高度，毫米 |
| sensor | roll_x10 | int16 | 横滚角，0.1 度 |
| sensor | pitch_x10 | int16 | 俯仰角，0.1 度 |
| sensor | temperature_x10 | int16 | 温度，0.1 摄氏度 |
| sensor | humidity_x10 | uint16 | 湿度，0.1% |
| lora | online | uint8 | 在线状态 |
| lora | tx_count | uint32 | 发送计数 |
| lora | rx_count | uint32 | 接收计数 |
| lora | loss_rate_x10 | uint16 | 丢包率，0.1% |
| motor | pwm[4] | uint8 | 4 路 PWM 占空比 |
| log | enabled | uint8 | 日志开关 |
| log | file_index | uint16 | 当前文件编号 |
| alarm | current_code | uint16 | 当前报警码 |

## 规则

1. 各模块只更新自身状态或输出采样数据，`CNS_State` 由应用层统一汇总生成。
2. 日志、HMI 和上报协议只读取 `CNS_State` 快照，不直接读取驱动内部变量。
3. 多任务访问时必须使用互斥锁、临界区、消息队列或双缓冲快照，避免读到不一致的数据组合。
4. 字段修改必须同步更新 HMI、CSV、上报协议和测试文档。
