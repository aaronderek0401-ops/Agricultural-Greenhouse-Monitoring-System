// SGP30 CO2传感器模块
#include <Wire.h>
#include "Adafruit_SGP30.h"

// SGP30 I2C引脚配置
#define CO2_SDA_PIN 6        // I2C数据引脚(SDA)，连接到开发板的D3引脚
#define CO2_SCL_PIN 5        // I2C时钟引脚(SCL)，连接到开发板的D2引脚

Adafruit_SGP30 sgp;
static bool sgpInitialized = false;
static unsigned long sgpStartTime = 0;

bool initCO2Sensor() {
  Serial.println("SGP30 CO2 sensor initializing...");
  Serial.println("Step 1: Starting Wire.begin() for CO2 sensor");
  
  // 初始化I2C总线，使用专用引脚
  Wire.begin(CO2_SDA_PIN, CO2_SCL_PIN);
  Serial.println("Step 2: Wire.begin() completed for CO2 sensor");
  
  delay(100); // 短暂延时让I2C稳定
  Serial.println("Step 3: About to call sgp.begin()");
  
  // 尝试初始化SGP30传感器
  bool success = sgp.begin();
  Serial.printf("Step 4: sgp.begin() returned %s\n", success ? "true" : "false");
  
  if (success) {
    Serial.println("SGP30 CO2 sensor connected successfully");
    sgpInitialized = true;
    sgpStartTime = millis();
    Serial.println("SGP30 needs 15 seconds warm-up time");
    return true;
  } else {
    Serial.println("SGP30 sensor not found! Skipping...");
    sgpInitialized = false;
    return false;
  }
}

void readCO2(int &co2, int &tvoc) {
  // 检查传感器是否已初始化
  if (!sgpInitialized) {
    co2 = -999;   // 表示传感器未连接
    tvoc = -999;
    return;
  }
  
  // 检查是否已预热15秒
  if (millis() - sgpStartTime < 15000) {
    co2 = -888;   // 表示传感器预热中
    tvoc = -888;
    Serial.println("SGP30 still warming up...");
    return;
  }
  
  // 尝试读取数据
  if (!sgp.IAQmeasure()) {
    Serial.println("Failed to read from SGP30 sensor!");
    // 使用默认值
    co2 = -777;
    tvoc = -777;
    return;
  }
  
  // 读取成功
  co2 = sgp.eCO2;
  tvoc = sgp.TVOC;
  
  Serial.printf("SGP30 - CO2: %d ppm, TVOC: %d ppb\n", co2, tvoc);
  
  // 可选：获取基线值用于长期运行
  static unsigned long lastBaseline = 0;
  if (millis() - lastBaseline > 60000) { // 每分钟打印一次基线
    lastBaseline = millis();
    uint16_t eco2_base, tvoc_base;
    if (sgp.getIAQBaseline(&eco2_base, &tvoc_base)) {
      Serial.printf("SGP30 Baseline - eCO2: %d, TVOC: %d\n", eco2_base, tvoc_base);
    }
  }
}
