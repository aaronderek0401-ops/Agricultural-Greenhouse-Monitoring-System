#include <Wire.h>             // I2C通信库，用于传感器与开发板之间的数据传输
#include <Adafruit_AHTX0.h>   // AHT系列温湿度传感器驱动库（支持AHT10/AHT20/AHT30）
Adafruit_AHTX0 aht;
#define NEW_SDA_PIN 7        // I2C数据引脚(SDA)，连接到开发板的D4引脚
#define NEW_SCL_PIN 6


void initTemperatureHumiditySensor() {
  Serial.begin(9600); // 初始化打印串口，波特率为9600
  Serial.println("AHT30 demo!");
  Wire.begin(NEW_SDA_PIN, NEW_SCL_PIN);    // 初始化I2C总线，指定自定义的SDA和SCL引脚

  if ( !aht.begin()) {                     // 调用begin()方法初始化传感器，返回true表示成功
    while(1){
      Serial.println("没找到AHT30传感器,请检查硬件连接"); // 初始化失败时打印错误信息
      delay(1000);                              // 进入死循环，停止程序运行（方便排查硬件问题）
    }
  }
  Serial.println("AHT30 连接正常");
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
    temperature = 25.0;
    humidity = 60.0;
    return;
  }
  
}
