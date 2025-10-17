#include <Wire.h>             // I2C通信库，用于传感器与开发板之间的数据传输
#include <Adafruit_AHTX0.h>   // AHT系列温湿度传感器驱动库（支持AHT10/AHT20/AHT30）


#define NEW_SDA_PIN 6        // I2C数据引脚(SDA)，连接到开发板的D3引脚
#define NEW_SCL_PIN 5        // I2C时钟引脚(SCL)，连接到开发板的D2引脚
Adafruit_AHTX0 aht;

bool initTemperatureHumiditySensor() {
  Serial.println("AHT30 initializing...");
  // Serial.println("Step 1: Starting Wire.begin()");
  
  Wire.begin(NEW_SDA_PIN, NEW_SCL_PIN);    // 初始化I2C总线，指定自定义的SDA和SCL引脚
  // Serial.println("Step 2: Wire.begin() completed");
  
  delay(100); // 短暂延时让I2C稳定
  // Serial.println("Step 3: About to call aht.begin()");
  
  // 尝试初始化，如果卡住就说明aht.begin()有问题
  bool success = aht.begin();
  // Serial.printf("Step 4: aht.begin() returned %s\n", success ? "true" : "false");
  
  if (success) {
    Serial.println("AHT30 connected successfully");
    return true;
  } else {
    Serial.println("AHT30 sensor not found! Skipping...");
    return false;
  }
}

void readTemperatureHumidity(float &temperature, float &humidity) {
  sensors_event_t humidity_, temp; // 定义存储温湿度数据的结构体（来自Adafruit_Sensor库）
  aht.getEvent(&humidity_, &temp); // 从传感器获取数据：湿度存入humidity，温度存入temp

  Serial.print("Temperature: "); Serial.print(temp.temperature); Serial.println(" degrees C");
  Serial.print("Humidity: "); Serial.print(humidity_.relative_humidity); Serial.println("% rH");

  // 读取湿度
  humidity = humidity_.relative_humidity;
  // 读取温度（摄氏度）
  temperature = temp.temperature;
  
  // 检查读取是否失败
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    // 使用默认值
    temperature = -999;
    humidity = -999;
    return;
  }
  
}
