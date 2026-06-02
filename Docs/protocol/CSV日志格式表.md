# CSV 日志格式表

## 文件命名

```text
CNS_LOG_YYYYMMDD_NNN.csv
```

## 字段定义

| 字段 | 单位 | 来源 | 说明 |
|---|---|---|---|
| timestamp_ms | ms | system | 系统时间 |
| battery_mv | mV | power | 电池电压 |
| gnss_fix | - | gnss | 定位状态 |
| latitude | deg | gnss | 纬度 |
| longitude | deg | gnss | 经度 |
| altitude_m | m | gnss | 高度 |
| speed_mps | m/s | gnss | 速度 |
| roll_deg | deg | sensor | 横滚角 |
| pitch_deg | deg | sensor | 俯仰角 |
| temperature_c | C | sensor | 温度 |
| humidity_pct | % | sensor | 湿度 |
| pressure_hpa | hPa | sensor | 气压 |
| lora_online | - | lora | LoRa 在线状态 |
| lora_tx_count | count | lora | 发送计数 |
| lora_rx_count | count | lora | 接收计数 |
| lora_loss_rate | % | lora | 丢包率 |
| motor_pwm_1 | % | motor | 电机 1 占空比 |
| motor_pwm_2 | % | motor | 电机 2 占空比 |
| motor_pwm_3 | % | motor | 电机 3 占空比 |
| motor_pwm_4 | % | motor | 电机 4 占空比 |
| alarm_code | - | alarm | 当前报警码 |

## 规则

1. 第一行为表头。
2. 默认记录频率 1Hz。
3. 写入失败必须触发日志异常状态。
4. 字段变更必须同步更新教学实验模板。
