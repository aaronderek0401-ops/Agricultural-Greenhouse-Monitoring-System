// 光照传感器模块 - BH1750数字光照强度传感器
// 负责光照强度检测和数据读取

#include <Wire.h>               // I2C通信库，用于传感器与开发板之间的数据传输
#include <Adafruit_Sensor.h>    // Adafruit传感器抽象库，提供统一的数据格式
#include <BH1750.h>             // BH1750光照传感器驱动库

// 光照传感器实例
static BH1750 lightMeter;

// 硬件配置
#define LIGHT_SDA_PIN 6          // I2C数据引脚(SDA)
#define LIGHT_SCL_PIN 5          // I2C时钟引脚(SCL)
#define LIGHT_ADDR_PIN 8         // BH1750的地址选择引脚

// 传感器状态
static bool lightSensorConnected = false;


// 初始化光照传感器
bool initLightSensor() {
  // 配置BH1750的地址控制引脚
  pinMode(LIGHT_ADDR_PIN, OUTPUT);    // 将地址引脚设置为输出模式
  digitalWrite(LIGHT_ADDR_PIN, LOW);  // 拉低地址引脚，选择BH1750的默认I2C地址0x23
                                      // 若设置为HIGH，则地址为0x5C

  // 初始化I2C总线（使用自定义引脚）
  Wire.begin(LIGHT_SDA_PIN, LIGHT_SCL_PIN);
  
  // 尝试初始化BH1750传感器
  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    lightSensorConnected = true;
    Serial.println("BH1750 光照传感器初始化成功");
    return true;
  } else {
    lightSensorConnected = false;
    Serial.println("BH1750 光照传感器初始化失败 - 请检查硬件连接");
    return false;
  }
}

// 读取光照强度数据
void readLightIntensity(float& lightIntensity) {
  if (!lightSensorConnected) {
    lightIntensity = -999;  // 特殊值表示传感器未连接
    return;
  }
  
  // 读取光照强度值
  float lux = lightMeter.readLightLevel();
  
  // 检查读取是否成功
  if (lux < 0) {
    // 读取失败，可能是传感器断开连接
    Serial.println("BH1750 读取失败 - 传感器可能断开连接");
    lightIntensity = -999;
    lightSensorConnected = false;  // 标记为断开连接
  } else {
    lightIntensity = lux;
    
    // 调试输出
    Serial.printf("光照强度: %.1f lux\n", lightIntensity);
  }
}

// 检查光照传感器连接状态
bool isLightSensorConnected() {
  return lightSensorConnected;
}

// 重新尝试连接光照传感器
bool reconnectLightSensor() {
  return initLightSensor();
}
