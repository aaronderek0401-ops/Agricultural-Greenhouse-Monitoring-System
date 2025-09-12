// BMP180气压传感器模块
#include <Wire.h>
#include <Adafruit_BMP085.h>

// BMP180 I2C引脚配置 (与AHT30使用不同引脚避免冲突)
#define BMP_SDA_PIN 6    // SDA引脚，连接到开发板的D3引脚
#define BMP_SCL_PIN 5    // SCL引脚,连接到开发板的D2引脚
#define BMP_ADDR 0x77    // BMP180的I2C地址

Adafruit_BMP085 bmp;     // 创建BMP180传感器对象

bool initPressureSensor() {
  Serial.println("BMP180 pressure sensor initializing...");
  Serial.printf("Using pins: SDA=%d, SCL=%d\n", BMP_SDA_PIN, BMP_SCL_PIN);
  
  // 先尝试检测设备是否存在，避免卡死
  Wire.begin(BMP_SDA_PIN, BMP_SCL_PIN);
  delay(100);
  
  // 发送I2C探测信号检查设备是否响应
  Wire.beginTransmission(BMP_ADDR);
  byte error = Wire.endTransmission();
  
  if (error == 0) {
    Serial.println("BMP180 device detected on I2C bus");
    // 设备存在，尝试初始化
    bool success = bmp.begin(BMP_ADDR);
    if (success) {
      Serial.println("BMP180 pressure sensor initialized successfully!");
      return true;
    } else {
      Serial.println("BMP180 device found but initialization failed");
      return false;
    }
  } else {
    Serial.printf("BMP180 not found (I2C error: %d) - will display 'disconnected'\n", error);
    return false;
  }
}

void readPressure(float &temperature, float &pressure, float &elevation) {
  // 读取传感器数据
  temperature = bmp.readTemperature();           // 温度，单位：摄氏度
  pressure = bmp.readPressure() / 100.0F;       // 气压，单位：百帕(hPa)
  elevation = (1013.25 - pressure) * 9;         // 简单海拔估算，单位：米
  
  // 检查读取是否成功
  if (isnan(temperature) || isnan(pressure)) {
    Serial.println("Failed to read from BMP180 sensor!");
    // 使用默认值
    temperature = 25.0;
    pressure = 1013.25;
    elevation = 0;
    return;
  }
  
  Serial.printf("BMP180 - Temp: %.1f°C, Pressure: %.1f hPa, Elevation: %.1f m\n", 
                temperature, pressure, elevation);
}
