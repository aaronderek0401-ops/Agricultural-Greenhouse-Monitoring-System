# 温室监控系统阈值参数修改完成

## 修改内容

### 1. 更新了阈值参数结构
- **移除参数**: 土壤湿度 (soilMin, soilMax)
- **新增参数**: 
  - 气压 (pressureMin, pressureMax) - 单位: hPa
  - 光强 (lightMin, lightMax) - 单位: lux

### 2. 新的参数列表
1. **温度** (tempMin, tempMax) - 单位: °C
2. **湿度** (humidityMin, humidityMax) - 单位: %
3. **CO2浓度** (co2Min, co2Max) - 单位: ppm
4. **气压** (pressureMin, pressureMax) - 单位: hPa
5. **光强度** (lightMin, lightMax) - 单位: lux

### 3. 修改的文件

#### Greenhouse_monitor.ino
- 更新 `Thresholds` 结构体定义
- 新的默认值:
  - 气压: 1000.0 - 1030.0 hPa
  - 光强: 5000 - 50000 lux

#### wifi_manager.ino
- 更新 HTML 阈值设置界面
- 更新 JavaScript 函数 (loadThresholds, saveThresholds, applyPreset)
- 更新 API 处理函数 (handleGetThresholds, handleSetThresholds)
- 更新植物预设配置，包含新的参数

### 4. 植物预设参数

| 植物 | 温度(°C) | 湿度(%) | CO2(ppm) | 气压(hPa) | 光强(lux) |
|------|----------|---------|----------|-----------|-----------|
| 🍅 番茄 | 18-26 | 60-80 | 400-1000 | 1000-1030 | 10000-50000 |
| 🥒 黄瓜 | 20-28 | 70-85 | 400-1200 | 1000-1030 | 8000-40000 |
| 🥬 生菜 | 15-22 | 50-70 | 300-800 | 1000-1030 | 5000-25000 |
| 🌶️ 辣椒 | 22-30 | 55-75 | 400-1000 | 1000-1030 | 12000-60000 |
| 🌿 香草 | 16-24 | 40-60 | 300-800 | 1000-1030 | 6000-30000 |

### 5. Web界面功能
- 阈值设置面板包含新的5个参数
- 植物预设按钮可一键应用对应植物的最佳阈值
- 支持独立设置每个参数的最小值和最大值
- 实时保存和加载当前阈值设置

## 使用说明

1. 连接到 WiFi 热点 "ESP32_Greenhouse"
2. 在浏览器中打开捕获门户页面
3. 点击 "设置阈值" 按钮
4. 选择植物预设或手动设置各参数的阈值范围
5. 点击 "保存设置" 按钮应用新的阈值

## 技术特性

- RESTful API: `/api/thresholds` (GET/POST)
- JSON 数据格式交换
- 前端 JavaScript 动态更新
- 响应式 CSS 设计
- 实时参数验证和保存

修改已完成，系统现在支持5个核心环境参数的阈值管理。