# User Dev — 开发总览

> 个人开发空间 | 分支: `senior_GNSS` | 目标: ATGM336H GNSS 模块完整驱动

---

## 环境速查

| 项 | 值 |
|----|-----|
| **MCU** | STM32F407ZGTx (Cortex-M4F, 168MHz, 1MB Flash) |
| **IDE** | Keil MDK-ARM (`MDK-ARM/low_altitude_cns_f407.uvprojx`) |
| **备选 IDE** | EIDE (VSCode) (`.eide/eide.yml`) |
| **编译器** | ARMCC5 / AC5 |
| **RTOS** | FreeRTOS (heap_4, NVIC Group4) |
| **调试器** | ST-Link / J-Link via SWD |
| **烧录地址** | 0x08000000 |

---

## 快速开始

```bash
# 1. 打开 Keil 工程
MDK-ARM/low_altitude_cns_f407.uvprojx

# 2. 编译 (F7)
# 3. 烧录 (F8)

# VSCode + EIDE
code low_altitude_cns_f407.code-workspace
```

```bash
# 分支操作
git checkout senior_GNSS          # 当前 GNSS 开发分支
git pull origin develop           # 拉取主开发线最新
git merge develop                  # 合并主开发线更新
```

---

## 引脚配置

详见根目录 `02_完整引脚分配表.md` + `Bsp/Inc/bsp_config.h`。

GNSS 模块占用：
- **USART2**: PA2(TX) → ATGM336H RX, PA3(RX) ← ATGM336H TX
- **供电控制**: PE6

---

## 本目录文档

| 文档 | 用途 |
|------|------|
| `开发指南.md` | 5 阶段 GNSS 开发路线图 |
| `硬件连接.md` | GNSS 接线、供电、电平 |
| `软件架构.md` | 分层设计、数据结构、接口 |
| `编码规范.md` | 命名、注释、提交约定 |
| `开发日志.md` | 每日改动记录 |

---

## 项目顶层架构

```
App/            ← 应用层 (协议、告警、状态、日志、注册表)
Bsp/            ← 板级支持 (UART/I2C/SPI/CAN/SD 初始化)
Sensor_Driver/  ← 传感器驱动抽象层 (注册表 + Mock)
Display/        ← FSMC 并口屏驱动
Debug/          ← 调试控制台 + 追踪宏
Drivers/        ← STM32F4 HAL + CMSIS
Middlewares/    ← FreeRTOS
Core/           ← 入口、中断、MSP
Tests/          ← 单元测试
Docs/           ← 项目级设计文档
```

## 当前代码完成度

| 模块 | 状态 |
|------|------|
| USART1 调试串口 | ✅ 已实现 |
| bsp_config.h 引脚宏 | ✅ 已生成 |
| GNSS 驱动 | ❌ 待开发 |
| LoRa | ❌ 空桩 |
| HMI | ❌ 空桩 |
| 传感器 | ❌ 空桩 |
| SD 卡 | ❌ 空桩 |
| 电机 | ❌ 空桩 |
| 5G-A | ❌ 空桩 |
