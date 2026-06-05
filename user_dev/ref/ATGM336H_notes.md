# ATGM336H 速查笔记

> 来源: ATGM336H 数据手册 (中科微电子)
> 用途: GNSS 驱动开发参考

---

## 基本参数

| 参数 | 值 |
|------|-----|
| 芯片 | ATGM336H-5N (杭州中科微) |
| 定位系统 | GPS L1 + BDS B1 (双模, 默认开机) |
| 输出协议 | NMEA 0183 |
| 默认波特率 | **9600 bps** |
| 更新频率 | **1 Hz** (默认), 最高 10Hz |
| 首次定位 (TTFF) | 冷启动 ~30s, 热启动 ~1s |
| 灵敏度 | -162dBm (跟踪), -148dBm (冷启动) |
| 供电 | 3.3V ~ 5.5V (板载 LDO) |
| 工作电流 | ~25mA (连续跟踪) |
| 引脚 | VCC, GND, TXD, RXD, PPS (6+1 孔) |

---

## NMEA 语句 (默认输出)

### GGA — 定位信息 (最常用)

```
$GNGGA,hhmmss.ss,llll.ll,a,yyyyy.yy,a,x,xx,x.x,x.x,M,x.x,M,x.x,xxxx*hh
│       │         │     │ │     │ │ │ │  │   │ │   │ │   │    │
│       UTC       Lat  N/S  Lon E/W质量数HDOP 高度  M 大地水准面  校验
                                        │  │    (米)     高度(M)
                                        卫星数
```

解析示例:
```
$GNGGA,065520.000,3017.2345,N,12004.5678,E,1,08,1.2,50.5,M,10.0,M,,*6F
→ UTC 06:55:20, 纬度 30°17.2345'N, 经度 120°04.5678'E
  定位质量 1(GPS修正), 8颗卫星, HDOP 1.2, 海拔 50.5m
```

### RMC — 推荐最小数据

```
$GNRMC,hhmmss.ss,A,llll.ll,a,yyyyy.yy,a,x.x,x.x,ddmmyy,,,A*hh
│       │       │ │     │ │     │ │  │   │   │      │
│       UTC    A=有效 Lat Lon  地面速度航向 日期   自动模式
│               V=无效           (节)    (度) (DDMMYY)
```

### GSV — 可见卫星

```
$GNGSV,n,m,ss,sv1,ele1,az1,snr1,sv2,...*hh
│      │ │ │  │   │    │    │
│      总语句数当前句总数 卫星号仰角方位角信噪比
│             卫星数
```

最多 4 颗卫星/句，总共可能多句 (GSV, n=1,2,3...)。

---

## NMEA 校验算法

```c
uint8_t nmea_checksum(const char *sentence)
{
    // sentence 从 '$' 之后 '*' 之前 (不含$*)
    uint8_t cs = 0;
    sentence++;  // 跳过 '$'
    while (*sentence && *sentence != '*') {
        cs ^= (uint8_t)*sentence++;
    }
    return cs;
}
```

---

## 常用配置指令

| 指令 | 功能 |
|------|------|
| `$PCAS01,1*1D` | 重启 GPS |
| `$PCAS02,1000*1E` | 设置波特率 9600 (1000=9600) |
| `$PCAS03,0*1C` | 无需发送更新频率 |
| `$PCAS04,2*1E` | BD 定位模式 (1=GPS 2=BD 3=GPS+BD) |

---

## PPS (秒脉冲) 引脚

- 定位成功时输出 100ms 宽脉冲，周期 1s
- V1.0 暂不接，留待后续精确校时
- 如用，可接 MCU GPIO 中断 + TIM 捕获
