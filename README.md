# 温室监控系统 - 传感器整合方案

## 项目结构

您的温室监控系统现在采用了模块化的代码结构，主要包含以下文件：

### 主文件
- `LCD_240x320.ino` - 主程序文件，包含LCD显示逻辑和主循环

### 传感器模块
- `sensors_temp_humid.ino` - DHT22温湿度传感器
- `sensors_co2.ino` - MH-Z19B CO2传感器  
- `sensors_soil.ino` - 土壤湿度传感器
- `sensors_light.ino` - BH1750光照传感器

### 控制模块
- `device_control.ino` - 设备控制模块（水泵、风扇、补光灯）

### 配置文件
- `config.ino` - 引脚定义和阈值配置

## 硬件连接

### ESP32引脚分配
```
LCD显示屏:
├── SCLK → GPIO 3
├── MOSI → GPIO 4  
├── DC   → GPIO 12
├── RST  → GPIO 13
└── BLK  → GPIO 14

传感器:
├── DHT22 → GPIO 2
├── 土壤湿度 → A0 (数据) + GPIO 7 (电源控制)
├── MH-Z19B → GPIO 16 (RX) + GPIO 17 (TX)
└── BH1750 → GPIO 21 (SDA) + GPIO 22 (SCL)

设备控制:
├── 水泵继电器 → GPIO 5
├── 风扇继电器 → GPIO 6
└── 补光灯继电器 → GPIO 8
```

## 需要安装的库

在Arduino IDE中安装以下库：

1. **LovyanGFX** - LCD显示库
2. **DHT sensor library** - DHT22温湿度传感器
3. **BH1750** - 光照传感器库
4. **SoftwareSerial** - CO2传感器软件串口（ESP32可能需要EspSoftwareSerial）

## 使用方法

1. **修改引脚配置**: 在 `config.ino` 中根据您的实际硬件连接修改引脚定义

2. **调整传感器阈值**: 在 `config.ino` 中的 `SensorThresholds` 结构体中修改阈值

3. **上传代码**: 在Arduino IDE中打开主文件，所有标签页会自动加载

4. **测试**: 打开串口监视器查看传感器读数和调试信息

## 替换现有传感器代码

如果您已有其他传感器的代码，可以：

1. 将传感器初始化代码放入对应的 `init*Sensor()` 函数
2. 将传感器读取代码放入对应的 `read*()` 函数  
3. 在 `config.ino` 中修改引脚定义
4. 根据需要修改传感器数据类型和阈值

## 优势

✅ **模块化设计** - 每个传感器独立管理，便于维护
✅ **自动编译** - Arduino IDE会自动编译所有.ino文件
✅ **易于扩展** - 添加新传感器只需创建新的标签页
✅ **统一管理** - 所有配置集中在config.ino中
✅ **代码复用** - 传感器模块可在其他项目中重用

## 调试建议

- 串口监视器会显示每个传感器的读数
- 如果某个传感器读取失败，会使用默认值并显示错误信息
- 可以先单独测试每个传感器模块，再整合到主系统中
