# 温室监控系统 - DHT22温湿度传感器版本

## 项目概述

这是一个基于ESP32和240x320 LCD显示屏的温室监控系统，目前主要功能是读取DHT22温湿度传感器数据并在屏幕上显示。其他传感器数据（CO2、土壤湿度、光照强度）暂时使用模拟数据显示。

## 当前文件结构

```
LCD_240x320/
├── LCD_240x320.ino          # 主程序文件（LCD显示 + 主循环）
├── sensors_temp_humid.ino   # DHT22温湿度传感器模块
├── config.ino              # 引脚配置和阈值设置
└── README.md               # 说明文档
```

## 硬件连接

### ESP32引脚分配
```
LCD显示屏 (ST7789):
├── SCLK → GPIO 3
├── MOSI → GPIO 4  
├── DC   → GPIO 12
├── RST  → GPIO 11  
├── CS   → GPIO 13
└── BLK  → GPIO 14

DHT22温湿度传感器:
└── DATA → GPIO 2
```

### 电源连接
- LCD显示屏：3.3V
- DHT22传感器：3.3V或5V

## 需要安装的库

在Arduino IDE中安装以下库：

1. **LovyanGFX** - LCD显示库
   - 工具 → 管理库 → 搜索"LovyanGFX"

2. **DHT sensor library** - DHT22温湿度传感器
   - 工具 → 管理库 → 搜索"DHT sensor library" (by Adafruit)

## 功能特性

### ✅ 已实现功能
- 240x320 LCD显示界面
- DHT22温湿度传感器实时读取
- 传感器数据状态指示（正常/警告/异常）
- 温湿度阈值监控
- 串口调试输出

### 🔄 模拟显示功能
- CO2浓度显示（模拟数据）
- 土壤湿度显示（模拟数据）
- 光照强度显示（模拟数据）
- 设备状态显示（水泵、风扇、补光灯）

## 使用方法

1. **硬件连接**：按照上面的引脚分配连接LCD和DHT22传感器

2. **安装库**：在Arduino IDE中安装必要的库

3. **配置引脚**：在`config.ino`中检查DHT_PIN设置是否正确（默认GPIO 2）

4. **上传代码**：在Arduino IDE中打开`LCD_240x320.ino`，上传到ESP32

5. **查看效果**：
   - LCD屏幕会显示温湿度数据和状态
   - 串口监视器会输出调试信息

## 显示界面说明

```
┌─────────────────────────┐
│ Greenhouse Monitor 12:34│  ← 标题栏
├─────────────────────────┤
│ Temperature  25.3°C  ●  │  ← 温度（实际读取）
│ Humidity     65.2%   ●  │  ← 湿度（实际读取）
│ CO2          800ppm  ●  │  ← CO2（模拟数据）
│ Soil Moist   55.0%   ●  │  ← 土壤湿度（模拟）
│ Light        25000lux●  │  ← 光照（模拟数据）
├─────────────────────────┤
│     Device Status       │
│ Pump:OFF    Fan:OFF     │  ← 设备状态（模拟）
│ Light:OFF               │
└─────────────────────────┘
```

### 状态指示器颜色
- 🟢 绿色：数值正常
- 🟡 黄色：数值接近阈值边界
- 🔴 红色：数值超出正常范围

## 配置说明

在`config.ino`中可以修改：

```cpp
// DHT22传感器引脚
#define DHT_PIN 2        // 数据引脚
#define DHT_TYPE DHT22   // 传感器类型

// 温湿度阈值
struct SensorThresholds {
  float tempMin = 18.0;     // 最低温度 (°C)
  float tempMax = 28.0;     // 最高温度 (°C)
  float humidityMin = 60.0; // 最低湿度 (%)
  float humidityMax = 80.0; // 最高湿度 (%)
};
```

## 调试方法

1. **串口监视器**：波特率115200，会显示：
   ```
   DHT22 sensor initialized
   Greenhouse Monitoring System Started
   DHT22 - Temp: 25.3°C, Humidity: 65.2%
   Temp:25.3C Humid:65.2% CO2:800ppm Soil:55.0% Light:25000lux
   ```

2. **常见问题**：
   - DHT22读取失败：检查接线，确认库已安装
   - LCD无显示：检查SPI接线和电源
   - 数据异常：查看串口输出确认传感器工作状态

## 扩展计划

后续可以添加的传感器模块：
- MH-Z19B CO2传感器
- 土壤湿度传感器
- BH1750光照传感器
- 继电器控制模块（水泵、风扇、补光灯）

每个新传感器只需要：
1. 创建对应的`.ino`文件
2. 在主文件的`readSensors()`中调用
3. 在`setup()`中初始化

## 技术规格

- **微控制器**：ESP32
- **显示屏**：240x320 ST7789 LCD
- **传感器**：DHT22（AM2302）
- **更新频率**：2秒
- **工作电压**：3.3V
- **通信协议**：SPI（LCD），单总线（DHT22）
