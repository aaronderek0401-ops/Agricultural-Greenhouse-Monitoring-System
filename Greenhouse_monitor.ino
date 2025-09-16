// 温室监控系统主程序
// 负责系统初始化、传感器协调、主循环控制

// 包含数据结构定义
struct SensorData {
  float temperature;    // 温度 (°C)
  float humidity;      // 湿度 (%)
  float pressure;      // 气压 (hPa)
  float elevation;     // 海拔高度 (m)
  int co2;            // 二氧化碳浓度 (ppm)
  int tvoc;           // 总挥发性有机化合物 (ppb)
  float soilMoisture; // 土壤湿度 (%)
  float lightIntensity; // 光照强度 (lux)
  bool pumpStatus;    // 水泵状态
  bool fanStatus;     // 风扇状态
  bool lightStatus;   // 补光灯状态
};

// 阈值配置
struct Thresholds {
  float tempMin = 18.0, tempMax = 29.0;
  float humidityMin = 45.0, humidityMax = 80.0;
  int co2Min = 300, co2Max = 1200;
  float soilMin = 40.0, soilMax = 70.0;
} thresholds;

// 全局变量
static SensorData sensorData;

// 传感器连接状态
static bool aht30Connected = false;
static bool bmp180Connected = false;
static bool sgp30Connected = false;
static bool bh1750Connected = false;

// 系统时间管理
static unsigned long lastUpdate = 0;
static const unsigned long UPDATE_INTERVAL = 2000; // 2秒更新一次

// 读取所有传感器数据
void readSensors() {
  // 读取温湿度传感器
  if (aht30Connected) {
    readTemperatureHumidity(sensorData.temperature, sensorData.humidity);
  } else {
    sensorData.temperature = -999;  // 特殊值表示未连接
    sensorData.humidity = -999;
  }
  
  // 读取气压传感器
  if (bmp180Connected) {
    float bmpTemp;
    readPressure(bmpTemp, sensorData.pressure, sensorData.elevation);
  } else {
    sensorData.pressure = -999;     // 特殊值表示未连接
    sensorData.elevation = -999;
  }
  
  // 读取CO2传感器
  if (sgp30Connected) {
    readCO2(sensorData.co2, sensorData.tvoc);
  } else {
    sensorData.co2 = -999;         // 特殊值表示未连接
    sensorData.tvoc = -999;
  }
  
  // 读取光照传感器
  if (bh1750Connected) {
    readLightIntensity(sensorData.lightIntensity);
  } else {
    sensorData.lightIntensity = -999; // 特殊值表示未连接
  }
  
  // 土壤湿度传感器暂时使用模拟数据
  sensorData.soilMoisture = 55.0 + random(-10, 10);
  
  // 模拟设备状态
  sensorData.pumpStatus = (sensorData.soilMoisture < thresholds.soilMin);
  sensorData.fanStatus = (sensorData.temperature > thresholds.tempMax);
  sensorData.lightStatus = (sensorData.lightIntensity < 20000);
}

// 输出调试信息
void printDebugInfo() {
  Serial.print("Sensors: ");
  
  // 温度
  if (sensorData.temperature == -999) {
    Serial.print("Temp:disconnected ");
  } else {
    Serial.printf("Temp:%.1fC ", sensorData.temperature);
  }
  
  // 湿度
  if (sensorData.humidity == -999) {
    Serial.print("Humid:disconnected ");
  } else {
    Serial.printf("Humid:%.1f%% ", sensorData.humidity);
  }
  
  // 气压
  if (sensorData.pressure == -999) {
    Serial.print("Pressure:disconnected ");
  } else {
    Serial.printf("Pressure:%.1fhPa ", sensorData.pressure);
  }
  
  // 海拔
  if (sensorData.elevation == -999) {
    Serial.print("Elevation:disconnected ");
  } else {
    Serial.printf("Elevation:%.1fm ", sensorData.elevation);
  }
  
  // CO2
  if (sensorData.co2 == -999) {
    Serial.print("CO2:disconnected ");
  } else {
    Serial.printf("CO2:%dppm ", sensorData.co2);
  }
  
  // 光照强度
  if (sensorData.lightIntensity == -999) {
    Serial.print("Light:disconnected ");
  } else {
    Serial.printf("Light:%.1flux ", sensorData.lightIntensity);
  }
  
  // 其他传感器（模拟数据）
  Serial.printf("Soil:%.1f%%\n", sensorData.soilMoisture);
}

// 检查异常情况并发出蜂鸣器警告
void checkAndAlertAbnormalConditions() {
  static unsigned long lastAlarmTime = 0;
  unsigned long currentTime = millis();
  bool hasAlarm = false;
  bool hasWarning = false;
  
  // 检查温度异常
  if (sensorData.temperature != -999) {
    if (sensorData.temperature < thresholds.tempMin || sensorData.temperature > thresholds.tempMax) {
      if (sensorData.temperature < thresholds.tempMin - 3 || sensorData.temperature > thresholds.tempMax + 3) {
        hasAlarm = true; // 严重异常
      } else {
        hasWarning = true; // 轻微异常
      }
    }
  }
  
  // 检查湿度异常
  if (sensorData.humidity != -999) {
    if (sensorData.humidity < thresholds.humidityMin || sensorData.humidity > thresholds.humidityMax) {
      if (sensorData.humidity < thresholds.humidityMin - 10 || sensorData.humidity > thresholds.humidityMax + 10) {
        hasAlarm = true; // 严重异常
      } else {
        hasWarning = true; // 轻微异常
      }
    }
  }
  
  // 检查CO2异常
  if (sensorData.co2 != -999) {
    if (sensorData.co2 > thresholds.co2Max) {
      if (sensorData.co2 > thresholds.co2Max + 300) {
        hasAlarm = true; // 严重异常
      } else {
        hasWarning = true; // 轻微异常
      }
    }
  }
  
  // 播放相应的蜂鸣器提示音（避免频繁播放）
  if (hasAlarm && currentTime - lastAlarmTime > 30000) { // 30秒间隔
    beepAlarm();
    lastAlarmTime = currentTime;
    Serial.println("警告: 检测到严重异常情况!");
  } else if (hasWarning && currentTime - lastAlarmTime > 10000) { // 10秒间隔
    beepWarning();
    lastAlarmTime = currentTime;
    Serial.println("注意: 检测到轻微异常情况");
  }
}

void setup(void)
{
  Serial.begin(115200);
  delay(1000); // 等待串口稳定
  Serial.println();
  Serial.println("======================");
  Serial.println("ESP32 STARTED!");
  Serial.println("======================");
  Serial.println("=== Greenhouse Monitoring System ===");
  Serial.println("Initializing...");
  
  // 初始化显示系统
  initDisplay();
  showStartupScreen();
  
  // 初始化设备控制（蜂鸣器）
  initDeviceControl();
  
  // 初始化时间系统
  initTimeSystem();
  
  // 传感器初始化
  Serial.println("Sensor initialization mode: ALL SENSORS ENABLED");
  
  // 尝试初始化温湿度传感器
  Serial.println("Attempting AHT30 init...");
  aht30Connected = initTemperatureHumiditySensor();
  showSensorStatus("AHT30...", 160, aht30Connected);
  Serial.printf("AHT30 result: %s\n", aht30Connected ? "SUCCESS" : "FAILED");
  // if (aht30Connected) beepSuccess(); else beepError();
  
  // 尝试初始化气压传感器
  Serial.println("Attempting BMP180 init...");
  bmp180Connected = initPressureSensor();
  showSensorStatus("BMP180...", 180, bmp180Connected);
  Serial.printf("BMP180 result: %s\n", bmp180Connected ? "SUCCESS" : "FAILED");
  // if (bmp180Connected) beepSuccess(); else beepError();

  // 尝试初始化二氧化碳传感器
  Serial.println("Attempting SGP30 init...");
  sgp30Connected = initCO2Sensor();
  showSensorStatus("SGP30...", 200, sgp30Connected);
  Serial.printf("SGP30 result: %s\n", sgp30Connected ? "SUCCESS" : "FAILED");
  // if (sgp30Connected) beepSuccess(); else beepError();
  
  // 尝试初始化光照传感器
  Serial.println("Attempting BH1750 init...");
  bh1750Connected = initLightSensor();
  showSensorStatus("BH1750...", 220, bh1750Connected);
  Serial.printf("BH1750 result: %s\n", bh1750Connected ? "SUCCESS" : "FAILED");
  // if (bh1750Connected) beepSuccess(); else beepError();
  
  // 初始化WiFi热点系统
  Serial.println("Attempting WiFi Hotspot init...");
  initWiFiHotspot();
  showSensorStatus("WiFi Hotspot...", 240, true); // WiFi初始化通常会成功
  Serial.println("WiFi Hotspot result: SUCCESS");
  
  // 显示初始化结果
  showInitComplete(aht30Connected, bmp180Connected, sgp30Connected, bh1750Connected);
  
  // 播放启动完成提示音
  beepStartup();
  
  delay(5000); // 显示状态5秒

  // 切换到正常运行界面
  refreshDisplay();
  
  // 初始化传感器数据
  readSensors();
  updateDisplay(sensorData, thresholds);
  
  Serial.println("Greenhouse Monitoring System Started");
  Serial.printf("Sensors: AHT30=%s, BMP180=%s, SGP30=%s, BH1750=%s\n", 
                aht30Connected ? "OK" : "FAIL", 
                bmp180Connected ? "OK" : "FAIL",
                sgp30Connected ? "OK" : "FAIL",
                bh1750Connected ? "OK" : "FAIL");
}

void loop(void)
{
  unsigned long currentTime = millis();
  
  // 每2秒更新一次数据
  if (currentTime - lastUpdate >= UPDATE_INTERVAL) {
    lastUpdate = currentTime;
    
    // 读取传感器数据
    readSensors();
    
    // 检查异常情况并发出蜂鸣器警告
    checkAndAlertAbnormalConditions();
    
    // 更新显示 - 先绘制固定内容
    drawHeader();
    updateDisplay(sensorData, thresholds);
    
    // 处理图表更新 - 最后绘制图表，避免被遮盖
    handleGraphUpdate(currentTime, sensorData);
    
    // 串口输出调试信息
    printDebugInfo();
  }
  
  // 处理WiFi通信（每次循环都检查）
  wifiLoop();
  
  delay(100); // 短暂延时，避免过度占用CPU
}
