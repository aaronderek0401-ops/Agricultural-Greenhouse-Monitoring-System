# 温室监控系统 - DHT22温湿度传感器版本

## 项目概述

这是一个基于ESP32和240x320 LCD显示屏的温室监控系统，目前1. **硬件连接**：按照上面的引脚分配连接LCD和传感器

2. **安装库**：在Arduino IDE中安装必要的库

3. **修改引脚**：如需修改引脚，在对应的传感器文件中修改：
   - AHT30引脚：在 `sensors_temp_humid.ino` 中修改 `NEW_SDA_PIN` 和 `NEW_SCL_PIN`
   - BMP180引脚：在 `sensors_pressure.ino` 中修改 `BMP_SDA_PIN` 和 `BMP_SCL_PIN`

4. **上传代码**：在Arduino IDE中打开`LCD_240x320.ino`，上传到ESP32DHT22温湿度传感器数据并在屏幕上显示。其他传感器数据（CO2、土壤湿度、光照强度）暂时使用模拟数据显示。

## 当前文件结构

```
LCD_240x320/
├── LCD_240x320.ino          # 主程序文件（LCD显示 + 主循环）
├── sensors_temp_humid.ino   # AHT30温湿度传感器模块
├── sensors_pressure.ino     # BMP180气压传感器模块
├── time_manager.ino         # 时间管理模块
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

AHT30温湿度传感器 (I2C):
├── SDA → GPIO 7
└── SCL → GPIO 6

BMP180气压传感器 (I2C):
├── SDA → GPIO 5
└── SCL → GPIO 4
```

### 电源连接
- LCD显示屏：3.3V
- AHT30传感器：3.3V
- BMP180传感器：3.3V

## 需要安装的库

在Arduino IDE中安装以下库：

1. **LovyanGFX** - LCD显示库
   - 工具 → 管理库 → 搜索"LovyanGFX"

2. **Adafruit AHTX0** - AHT30温湿度传感器
   - 工具 → 管理库 → 搜索"Adafruit AHTX0"

3. **Adafruit BMP085 Library** - BMP180气压传感器
   - 工具 → 管理库 → 搜索"Adafruit BMP085"

## 功能特性

### ✅ 已实现功能
- 240x320 LCD显示界面
- **非阻塞传感器初始化**（3秒超时保护，不会卡住）
- AHT30温湿度传感器实时读取
- BMP180气压传感器实时读取（气压值和海拔估算）
- **传感器连接状态检测**（显示"disconnected"当传感器未连接）
- **启动状态显示**（显示传感器初始化进度）
- 传感器数据状态指示（正常/警告/异常/未连接）
- 温湿度阈值监控
- 运行时间显示（从设备启动开始计算）
- 串口调试输出

### 🔄 模拟显示功能
- CO2浓度显示（模拟数据）
- 土壤湿度显示（模拟数据）
- 光照强度显示（模拟数据）
- 设备状态显示（水泵、风扇、补光灯）

## 启动流程

1. **LCD优先初始化** - 确保屏幕能立即显示状态
2. **显示启动界面** - "System Starting..." 和传感器初始化进度
3. **传感器初始化** - 每个传感器有3秒超时保护
4. **状态反馈** - 显示哪些传感器成功/失败
5. **进入正常运行** - 即使部分传感器失败也能正常工作

### 启动界面显示
```
Greenhouse Monitor  00:00
System Starting...
Initializing Sensors
AHT30...
BMP180...
All sensors ready!    (或 "Some sensors ready" / "No sensors found")
```

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
│ Greenhouse Monitor 01:23│  ← 标题栏
├─────────────────────────┤
│ Temperature  25.3°C  ●  │  ← 温度（实际读取）
│ Humidity     65.2%   ●  │  ← 湿度（实际读取）
│ Pressure     1013hPa ●  │  ← 气压（实际读取）
│ Elevation    100.5m  ●  │  ← 海拔（计算值）
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
- 🔴 红色：数值超出正常范围或传感器未连接
- **disconnected**：传感器初始化失败或未连接

## 配置说明

如需修改传感器引脚，直接在对应的传感器文件中修改：

### AHT30温湿度传感器 (`sensors_temp_humid.ino`)
```cpp
#define NEW_SDA_PIN 6        // SDA引脚
#define NEW_SCL_PIN 5        // SCL引脚
```

### BMP180气压传感器 (`sensors_pressure.ino`)
```cpp
#define BMP_SDA_PIN 6        // SDA引脚
#define BMP_SCL_PIN 5        // SCL引脚
```

### 传感器阈值
温湿度的正常范围阈值在主文件的 `thresholds` 结构体中定义：
```cpp
struct Thresholds {
  float tempMin = 18.0, tempMax = 28.0;     // 温度范围
  float humidityMin = 60.0, humidityMax = 80.0; // 湿度范围
  // ...
} thresholds;
```

## 时间显示说明

### 当前时间显示模式
默认显示的是**运行时间**，即从ESP32启动开始计算的时间，格式为 `HH:MM`。

### 时间显示选项

1. **运行时间模式**（默认）
   - 显示从设备启动开始的累计时间
   - 格式：00:00 ~ 23:59，然后重新循环
   - 简单可靠，不需要外部依赖

2. **简单计数模式**
   - 基于毫秒计数的简单时间显示
   - 可通过串口命令切换模式

### 切换时间模式
可以在代码中调用 `switchTimeMode()` 函数来切换显示模式，或者在串口监视器中观察不同模式的输出。

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
