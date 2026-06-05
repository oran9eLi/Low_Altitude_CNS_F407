# CNS 帧格式定义 V1.0

> 主控箱 STM32F407 与中心平台、Linux 网关、远端外设节点、后续上位机之间的统一二进制通信帧格式。

DWIN 串口 HMI 仍按 `HMI变量协议表.md` 访问变量地址，不直接使用本二进制帧。HMI 显示数据来源于应用层汇总的 `CNS_State` 快照。

---

## 1. 基本约束

### 1.1 命名

| 术语 | 含义 |
|---|---|
| `CNS_Frame_t` | 完整通信帧结构体，包含固定帧头、payload 和 CRC |
| `CNS_MessageType_t` | 消息类型枚举，决定 payload 格式 |
| `source` | 源设备地址，1 字节 |
| `target` | 目标设备地址，1 字节，`0x00` 表示广播 |
| `payload` | 有效载荷，格式由 `msg_type` 决定 |

### 1.2 传输规则

| 项目 | 规则 |
|---|---|
| 线上编码 | 原始二进制字节流，不做 hex/ASCII 编码 |
| 字节序 | 多字节字段统一 little-endian |
| 帧头长度 | 固定 15 字节，不使用可变帧头 |
| CRC 长度 | 2 字节 |
| 最大 payload | 128 字节 |
| 最大总帧长 | 145 字节，即 `15 + payload_len + 2` |
| CRC 算法 | CRC-16/CCITT-FALSE |

V1.0 不定义 COMPACT 可变帧头。LoRa 链路与网关链路使用同一帧格式，便于调试、抓包和复用编解码代码。

---

## 2. 固定帧结构

### 2.1 帧布局

```text
┌────────┬────────┬─────────┬──────────┬──────────┬────────┬────────┬───────┬──────────────┬─────────────┬──────────┬───────┐
│HEADER0 │HEADER1 │VERSION  │ MSG_TYPE │ SEQUENCE │ SOURCE │ TARGET │ FLAGS │ TIMESTAMP_MS │ PAYLOAD_LEN │ PAYLOAD  │ CRC16 │
│ 1B     │ 1B     │ 1B      │ 1B       │ 2B LE    │ 1B     │ 1B     │ 1B    │ 4B LE        │ 2B LE       │ N Byte   │ 2B LE │
└────────┴────────┴─────────┴──────────┴──────────┴────────┴────────┴───────┴──────────────┴─────────────┴──────────┴───────┘
 0        1        2         3          4          6        7        8       9              13            15         15+N
```

### 2.2 字段定义

| 偏移 | 长度 | 字段 | 类型 | 说明 |
|---:|---:|---|---|---|
| 0 | 1 | `header0` | uint8 | 固定 `0xAA` |
| 1 | 1 | `header1` | uint8 | 固定 `0x55` |
| 2 | 1 | `version` | uint8 | 当前 `0x01` |
| 3 | 1 | `msg_type` | uint8 | 消息类型，见第 3 章 |
| 4 | 2 | `sequence` | uint16 | 帧序号，每个发送端独立递增；`0x0000` 保留不用，溢出后回到 `0x0001` |
| 6 | 1 | `source` | uint8 | 源设备地址，见第 4 章 |
| 7 | 1 | `target` | uint8 | 目标设备地址，`0x00` 为广播 |
| 8 | 1 | `flags` | uint8 | 标志位，见 2.3 |
| 9 | 4 | `timestamp_ms` | uint32 | 发送端本地运行时间，单位 ms |
| 13 | 2 | `payload_len` | uint16 | payload 有效字节数，范围 `0~128` |
| 15 | N | `payload` | uint8[N] | 负载数据 |
| 15+N | 2 | `crc16` | uint16 | CRC-16/CCITT-FALSE，覆盖偏移 `0` 到 `14+N` |

总帧长：

```text
frame_len = 15 + payload_len + 2
```

### 2.3 FLAGS 字段

| Bit | 名称 | 置 1 含义 |
|---:|---|---|
| 0 | `ACK_REQ` | 接收方需要回复 `CNS_MSG_ACK` |
| 1 | `IS_ACK` | 当前帧是对某个请求的应答或应答数据 |
| 2 | `IS_RETRY` | 当前帧是重传帧 |
| 3-7 | `RESERVED` | 保留，发送时填 0，接收时忽略 |

---

## 3. 消息类型

| 值 | 枚举 | 方向 | 说明 |
|---:|---|---|---|
| `0x01` | `CNS_MSG_HEARTBEAT` | 双向 | 心跳，payload 为空 |
| `0x02` | `CNS_MSG_STATUS` | 主控 -> 上层 | 系统状态快照，payload 见 5.1 |
| `0x03` | `CNS_MSG_SENSOR` | 主控 -> 上层 | 传感器采样明细，payload 见 5.2 |
| `0x04` | `CNS_MSG_ALARM` | 主控 -> 上层 | 异常状态变化，payload 见 5.3 |
| `0x06` | `CNS_MSG_SELFCHECK` | 主控 -> 上层 | 自检结果，payload 见 5.4 |
| `0x10` | `CNS_MSG_QUERY` | 上层 -> 主控 | 查询请求，payload 见 5.5 |
| `0x11` | `CNS_MSG_CONFIG` | 双向 | 参数读取或设置，payload 见 5.6 |
| `0x12` | `CNS_MSG_CONTROL` | 上层 -> 主控 | 控制命令，payload 见 5.7 |
| `0x80` | `CNS_MSG_ACK` | 双向 | 通用应答，payload 见 5.8 |

---

## 4. 设备地址

`source` 和 `target` 为 1 字节设备地址。地址段用于区分节点角色，具体设备地址由参数配置或出厂默认值决定。

地址段不表达物理链路。主控箱通过 LoRa、网关或有线串口通信时，节点角色仍然是主控箱；远端外设节点指通过 LoRa 等链路接入的传感、执行或教学测试节点。

| 地址范围 | 节点类型 | 容量 | 默认值 |
|---|---|---:|---|
| `0x00` | 广播地址 | - | - |
| `0x01~0x0F` | 中心/平台侧逻辑节点 | 15 | `0x01` |
| `0x10~0x1F` | 网关/边缘桥接节点 | 16 | `0x10` |
| `0x20~0x5F` | 主控箱 | 64 | `0x20` |
| `0x60~0x9F` | 远端外设节点 | 64 | `0x60` |
| `0xA0~0xBF` | 本地显示/调试终端 | 32 | `0xA0` |
| `0xC0~0xEF` | 平台扩展节点 | 48 | - |
| `0xF0~0xFE` | 保留/实验地址 | 15 | - |
| `0xFF` | 保留 | - | - |

地址判断宏建议：

```c
#define CNS_ADDR_IS_CENTER(a)     (((a) >= 0x01U) && ((a) <= 0x0FU))
#define CNS_ADDR_IS_GATEWAY(a)    (((a) >= 0x10U) && ((a) <= 0x1FU))
#define CNS_ADDR_IS_MASTER(a)     (((a) >= 0x20U) && ((a) <= 0x5FU))
#define CNS_ADDR_IS_REMOTE(a)     (((a) >= 0x60U) && ((a) <= 0x9FU))
#define CNS_ADDR_IS_DISPLAY(a)    (((a) >= 0xA0U) && ((a) <= 0xBFU))
#define CNS_ADDR_IS_EXTENSION(a)  (((a) >= 0xC0U) && ((a) <= 0xEFU))
#define CNS_ADDR_IS_EXPERIMENT(a) (((a) >= 0xF0U) && ((a) <= 0xFEU))
```

设备地址从持久化参数读取。持久化值无效时使用默认值。

```text
上电 -> 读取持久化地址 -> 地址在 0x01~0xFE 内 -> 使用持久化地址
                      -> 地址无效或未写入     -> 使用出厂默认值
```

---

## 5. Payload 定义

### 5.1 STATUS (0x02)

系统状态快照由应用层汇总 `CNS_State` 后编码。STATUS 是高频消息，使用固定长度 payload。

**payload 长度：56 字节**

| 偏移 | 长度 | 字段 | 类型 | 来源 | 说明 |
|---:|---:|---|---|---|---|
| 0 | 1 | `system_phase` | uint8 | 系统任务 | `0=INIT, 1=SELFCHECK, 2=RUNNING, 3=ERROR, 4=RECOVERY` |
| 1 | 4 | `uptime_ms` | uint32 | 系统 tick | 系统运行时间 |
| 5 | 4 | `selfcheck_map` | uint32 | 注册表 | bit 按 `App_ModuleId_t` 排列，1 表示该模块自检通过 |
| 9 | 2 | `battery_mv` | uint16 | 电源采样 | 电池电压，单位 mV |
| 11 | 1 | `gnss_fix` | uint8 | GNSS | `0=无定位, 2=2D, 3=3D` |
| 12 | 4 | `gnss_lat_e7` | int32 | GNSS | 纬度，单位 degree * 1e7 |
| 16 | 4 | `gnss_lon_e7` | int32 | GNSS | 经度，单位 degree * 1e7 |
| 20 | 4 | `gnss_alt_mm` | int32 | GNSS | 高度，单位 mm |
| 24 | 2 | `roll_x10` | int16 | IMU | 横滚角，单位 0.1 度 |
| 26 | 2 | `pitch_x10` | int16 | IMU | 俯仰角，单位 0.1 度 |
| 28 | 2 | `temp_x10` | int16 | BME280 | 温度，单位 0.1 摄氏度 |
| 30 | 2 | `humi_x10` | uint16 | BME280 | 湿度，单位 0.1 %RH |
| 32 | 1 | `lora_online` | uint8 | 通信模块 | `0=离线, 1=在线` |
| 33 | 4 | `lora_tx_cnt` | uint32 | 通信模块 | 累计发送帧数 |
| 37 | 4 | `lora_rx_cnt` | uint32 | 通信模块 | 累计接收帧数 |
| 41 | 2 | `lora_loss_x10` | uint16 | 通信模块 | 丢包率，单位 0.1% |
| 43 | 2 | `alarm_code` | uint16 | 告警模块 | 当前最高优先级活动异常错误码；无异常为 `ERR_OK` |
| 45 | 1 | `alarm_severity` | uint8 | 告警模块 | `0=正常, 1=一般, 2=重要, 3=严重` |
| 46 | 4 | `motor_pwm` | uint8[4] | 电机模块 | 4 路电机 PWM 占空比，范围 0~100 |
| 50 | 1 | `log_enabled` | uint8 | 日志模块 | `0=关闭, 1=开启` |
| 51 | 2 | `log_index` | uint16 | 日志模块 | 当前日志文件编号 |
| 53 | 3 | `reserved` | uint8[3] | - | 保留，填 0 |

### 5.2 SENSOR (0x03)

传感器采样明细使用变长列表。该消息表达采样值，不表达异常触发或恢复；传感器异常通过 `CNS_MSG_ALARM` 上报。

```text
cnt(1B) + sample[0] + sample[1] + ...
```

单个采样点：

```text
sensor_id(2) + sensor_type(1) + channel(2) + value_type(1) + timestamp_ms(4) + status_flags(4) + value_len(1) + value(N)
```

| 字段 | 长度 | 说明 |
|---|---:|---|
| `sensor_id` | 2 | 与 `Sensor_Driver_t.device_id` 一致 |
| `sensor_type` | 1 | `Sensor_Type_t` 的线上编码 |
| `channel` | 2 | 与 `Sensor_Sample_t.channel` 一致 |
| `value_type` | 1 | `Sensor_ValueType_t`：`0=NONE, 1=FLOAT, 2=INT32, 3=UINT32, 4=BOOL, 5=GEO` |
| `timestamp_ms` | 4 | 采样时间戳 |
| `status_flags` | 4 | 采样质量或数据状态标志；0 表示正常 |
| `value_len` | 1 | value 字节数 |
| `value` | N | 采样值，小端编码 |

value 编码：

| `value_type` | `value_len` | value 格式 |
|---|---:|---|
| `NONE` | 0 | 无 |
| `FLOAT` | 4 | IEEE754 float32 |
| `INT32` | 4 | int32 |
| `UINT32` | 4 | uint32 |
| `BOOL` | 1 | uint8，0=false，非 0=true |
| `GEO` | 12 | `lat_e7:int32 + lon_e7:int32 + alt_mm:int32` |

单帧 payload 不超过 128 字节。采样点过多时拆成多帧发送，`sequence` 正常递增。

### 5.3 ALARM (0x04)

异常状态变化使用变长列表。每条记录表达一个异常 key 的触发或恢复。

活动异常 key：

```text
source_module + error_code + instance_id
```

payload：

```text
cnt(1B) + alarm_record[0] + alarm_record[1] + ...
```

单个告警记录固定头：

```text
op(1) + source_module(1) + severity(1) + payload_type(1) + error_code(2) + instance_id(2) + first_ms(4) + last_ms(4) + detail_len(1) + detail(N)
```

| 字段 | 长度 | 说明 |
|---|---:|---|
| `op` | 1 | `0=RAISE, 1=CLEAR` |
| `source_module` | 1 | `App_ModuleId_t` |
| `severity` | 1 | `0=正常, 1=一般, 2=重要, 3=严重`；CLEAR 时填 0 |
| `payload_type` | 1 | `App_AlarmPayloadType_t` 的线上编码 |
| `error_code` | 2 | `App_ErrorCode_t` |
| `instance_id` | 2 | 同一模块同一错误下的实例 ID，例如传感器 ID |
| `first_ms` | 4 | 首次触发时间；CLEAR 时可填原记录首次触发时间，未知填 0 |
| `last_ms` | 4 | 最近更新时间或清除时间 |
| `detail_len` | 1 | detail 字节数 |
| `detail` | N | payload_type 决定的上下文 |

detail 编码：

| `payload_type` | detail 内容 |
|---|---|
| `NONE` | `detail_len=0` |
| `SENSOR` | `sensor_id:uint16 + sensor_type:uint8 + driver_error:uint32 + name_len:uint8 + name:name_len` |
| `COMM` | `link_id:uint16 + timeout_ms:uint32 + detail:uint32` |
| `STORAGE` | `target_id:uint16 + operation:uint32 + detail:uint32` |
| `SYSTEM` | `hook_code:uint32 + task_id:uint32 + detail:uint32` |
| `RAW` | `raw_detail:uint32` |

传感器名称使用 ASCII。超过单帧长度限制时，优先保留固定头和 `driver_error`，名称可以截断。

### 5.4 SELFCHECK (0x06)

自检结果在上电自检结束后发送一次，也可以通过 QUERY 请求重新上报。

```text
overall_result(1) + passed_count(1) + abnormal_count(1) + module_record[0] + ...
```

单个模块记录：

```text
module_id(1) + check_result(1) + error_code(2)
```

| 字段 | 长度 | 说明 |
|---|---:|---|
| `overall_result` | 1 | 本轮自检最高严重程度：`0=正常, 1=一般, 2=重要, 3=严重` |
| `passed_count` | 1 | 正常模块数量 |
| `abnormal_count` | 1 | 异常模块数量 |
| `module_id` | 1 | `App_ModuleId_t` |
| `check_result` | 1 | `App_CheckResult_t` |
| `error_code` | 2 | 异常模块错误码，正常模块不进入列表 |

### 5.5 QUERY (0x10)

```text
query_type(1) + param(1)
```

| 字段 | 说明 |
|---|---|
| `query_type` | 请求的消息类型，例如 `0x02=STATUS, 0x03=SENSOR, 0x04=ALARM, 0x06=SELFCHECK` |
| `param` | 查询参数，当前填 0；后续可用于传感器筛选或页号 |

主控返回对应消息类型，返回帧 `flags.IS_ACK=1`。如果请求无法执行，返回 `CNS_MSG_ACK` 并携带失败结果。

### 5.6 CONFIG (0x11)

```text
method(1) + param_id(1) + value_len(1) + value(N)
```

| 字段 | 说明 |
|---|---|
| `method` | `0x01=SET, 0x02=GET` |
| `param_id` | 参数编号 |
| `value_len` | value 字节数；GET 时填 0 |
| `value` | 参数值，小端编码；GET 时省略 |

V1.0 参数：

| param_id | 参数名 | 类型 | 说明 |
|---:|---|---|---|
| `0x01` | `log_enable` | uint8 | 日志开关 |
| `0x02` | `log_interval_ms` | uint16 | 日志周期 |
| `0x03` | `status_interval_ms` | uint16 | STATUS 上报周期 |
| `0x10` | `motor_pwm_1` | uint8 | 电机 1 占空比 |
| `0x11` | `motor_pwm_2` | uint8 | 电机 2 占空比 |
| `0x12` | `motor_pwm_3` | uint8 | 电机 3 占空比 |
| `0x13` | `motor_pwm_4` | uint8 | 电机 4 占空比 |
| `0x20` | `device_addr` | uint8 | 本设备 CNS 地址 |
| `0x21` | `lora_channel` | uint8 | LoRa 信道 |
| `0x30` | `alarm_low_battery_mv` | uint16 | 低压阈值 |
| `0x31` | `alarm_high_temp_x10` | int16 | 高温阈值 |

GET 成功时返回 `CNS_MSG_ACK`，ACK 的 `data` 字段携带参数值。

### 5.7 CONTROL (0x12)

```text
cmd(1) + param(1)
```

| cmd | 枚举 | param | 说明 |
|---:|---|---|---|
| `0x01` | `CNS_CMD_START_LOG` | 0 | 开启日志 |
| `0x02` | `CNS_CMD_STOP_LOG` | 0 | 停止日志 |
| `0x03` | `CNS_CMD_CLEAR_ALARM` | 0 | 清除可手动清除的异常 |
| `0x10` | `CNS_CMD_MOTOR_START` | 电机编号 1~4 | 启动指定电机 |
| `0x11` | `CNS_CMD_MOTOR_STOP` | 电机编号 1~4 | 停止指定电机 |
| `0x1F` | `CNS_CMD_MOTOR_ESTOP` | 0 | 全部电机急停 |
| `0x20` | `CNS_CMD_REBOOT` | `0xA5` | 重启设备 |
| `0x30` | `CNS_CMD_ENTER_SELFCHECK` | 0 | 触发自检 |

### 5.8 ACK (0x80)

```text
req_sequence(2) + result(1) + data_len(1) + data(N)
```

| 字段 | 长度 | 说明 |
|---|---:|---|
| `req_sequence` | 2 | 被应答帧的 `sequence` |
| `result` | 1 | `0=OK, 1=ERR_PARAM, 2=ERR_BUSY, 3=ERR_NOT_SUPPORTED, 4=ERR_TIMEOUT, 0xFF=FAIL` |
| `data_len` | 1 | data 字节数 |
| `data` | N | 可选应答数据，例如 CONFIG GET 的返回值 |

---

## 6. CRC

### 6.1 参数

| 参数 | 值 |
|---|---|
| 算法 | CRC-16/CCITT-FALSE |
| 多项式 | `0x1021` |
| 初始值 | `0xFFFF` |
| 输入反射 | 否 |
| 输出反射 | 否 |
| 异或输出 | `0x0000` |

### 6.2 计算范围

CRC 覆盖 `header0` 到 payload 最后 1 字节，不包含 `crc16` 字段。

```text
crc_len = 15 + payload_len
```

payload 为空时，CRC 覆盖固定帧头 15 字节。

### 6.3 参考实现

```c
uint16_t CNS_CalcCRC16(const uint8_t *data, uint16_t len)
{
  uint16_t crc = 0xFFFFU;

  for (uint16_t i = 0U; i < len; i++)
  {
    crc ^= (uint16_t)data[i] << 8;
    for (uint8_t bit = 0U; bit < 8U; bit++)
    {
      if ((crc & 0x8000U) != 0U)
      {
        crc = (uint16_t)((crc << 1) ^ 0x1021U);
      }
      else
      {
        crc <<= 1;
      }
    }
  }

  return crc;
}
```

---

## 7. 接收状态机

```text
IDLE
  收到 0xAA -> WAIT_55

WAIT_55
  收到 0x55 -> READ_HEADER
  收到 0xAA -> 保持 WAIT_55
  其他字节  -> IDLE

READ_HEADER
  读取 13 字节：version 到 payload_len
  校验 version、flags 保留位、payload_len
  payload_len > 128 -> IDLE
  payload_len == 0  -> READ_CRC
  payload_len > 0   -> READ_PAYLOAD

READ_PAYLOAD
  读取 payload_len 字节
  -> READ_CRC

READ_CRC
  读取 2 字节 CRC
  CRC 正确 -> 投递完整帧到业务层
  CRC 错误 -> 丢弃
  -> IDLE
```

接收规则：

- `READ_PAYLOAD` 阶段按长度读取，不扫描帧头。
- 任一状态超时后回到 `IDLE`。
- `version` 不支持时丢弃该帧；如果 `ACK_REQ=1` 且能识别 `source`，可以回复失败 ACK。
- `payload_len` 超过 128 直接丢弃。

---

## 8. 交互时序

### 8.1 上电到运行

```text
STM32 上电
  -> 发送 STATUS(system_phase=INIT)
  -> 模块初始化
  -> 发送 STATUS(system_phase=SELFCHECK)
  -> 执行自检
  -> 发送 SELFCHECK
  -> 周期发送 STATUS(system_phase=RUNNING 或 ERROR)
```

### 8.2 正常运行

| 消息 | 触发方式 | 周期或时机 |
|---|---|---|
| HEARTBEAT | 周期 | 1s |
| STATUS | 周期 | 默认 500ms |
| SENSOR | 周期或查询 | 根据采样周期和链路带宽配置 |
| ALARM | 事件 | 异常触发或恢复时立即发送 |
| SELFCHECK | 事件/查询 | 自检结束或收到查询 |
| QUERY | 请求 | 上层主动下发 |
| CONFIG | 请求 | 参数读取或设置 |
| CONTROL | 请求 | 控制命令 |
| ACK | 应答 | 对 `ACK_REQ` 请求或操作结果回复 |

### 8.3 请求应答

```text
上层 -> 主控:
  msg_type = CONTROL
  flags    = ACK_REQ
  sequence = 0x002A
  payload  = {cmd=START_LOG, param=0}

主控 -> 上层:
  msg_type = ACK
  flags    = IS_ACK
  payload  = {req_sequence=0x002A, result=OK, data_len=0}
```

---

## 9. 后期议题

### 9.1 设备序列号与硬件 UID

主控箱后续接入 Web 端和中心平台时，需要区分通信短地址、产品序列号和硬件唯一 ID。

| 标识 | 来源 | 用途 |
|---|---|---|
| `node_addr` | 参数配置或默认地址 | CNS 帧 `source`/`target`，用于当前通信网络内路由 |
| `device_sn` | 出厂写入、Web 注册下发或 UID 派生 | Web 端、资产台账、二维码、售后和页面展示 |
| `hardware_uid` | STM32 96-bit UID | 硬件指纹、设备绑定、防重复注册和维修追溯 |

会议需要确认：

1. V1.0 是否只使用 STM32 UID 派生临时 `device_sn`。
2. 后续 Web/生产流程是否下发正式 `device_sn` 并写入 Flash。
3. 对外上报是否只携带 `hardware_uid_hash`，避免直接暴露完整 UID。
4. 设备换板维修时，`device_sn` 与新 `hardware_uid` 的重新绑定流程。

建议原则：

- 不把完整 STM32 UID 直接作为对外产品 SN。
- 没有正式 SN 时，可用 UID 计算短 hash 生成临时 SN，例如 `CNS-LA01-MC-3A9F21C7`。
- 已写入正式 SN 时，设备优先使用正式 SN；UID 仅作为硬件绑定和追溯依据。
- `device_sn`、`hardware_uid_hash`、`platform_type`、`device_role`、`firmware_version` 后续可放入 `HEARTBEAT`、`STATUS` 或新增 `HELLO/REGISTER` payload 中。

---

## 10. 代码落点

| 定义 | 建议位置 | 说明 |
|---|---|---|
| `CNS_Frame_t` | `App/Inc/cns_frame.h` | 帧结构体 |
| `CNS_FrameResult_t` | `App/Inc/cns_frame.h` | 编解码返回值 |
| `CNS_MessageType_t` | `App/Inc/cns_message.h` | 消息类型 |
| 地址段宏 | `App/Inc/cns_message.h` | 地址判断 |
| `CNS_CalcCRC16()` | `App/Src/cns_frame.c` | CRC 计算 |
| `CNS_EncodeFrame()` | `App/Src/cns_frame.c` | 帧编码 |
| `CNS_DecodeFrame()` | `App/Src/cns_frame.c` | 帧解码 |
| payload 编码 | `App/Src/cns_payload.c` | STATUS/SENSOR/ALARM 等 payload 组装 |
| 错误码 | `App/Inc/app_error.h` | `App_ErrorCode_t` |

现有 `App_ProtocolBuildFrame()` 只用于调试闭环。正式 CNS 通信实现落到 `cns_frame.*` 和 `cns_payload.*` 后，调试帧不再作为对外协议依据。

---

## 11. 版本记录

| 版本 | 日期 | 内容 |
|---|---|---|
| V1.0 | 2026-06-05 | 初版二进制 CNS 帧格式，固定帧头、统一地址段、STATUS/SENSOR/ALARM/SELFCHECK/QUERY/CONFIG/CONTROL/ACK payload 定义 |
