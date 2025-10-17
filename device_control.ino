// 设备控制模块 - 蜂鸣器控制
// 负责系统提示音和报警功能

#define BUZZER_PIN 9          // 蜂鸣器引脚D6

// 蜂鸣器状态
static bool buzzerEnabled = true;

// 初始化蜂鸣器
void initDeviceControl() {
  pinMode(BUZZER_PIN, OUTPUT);
  Serial.println("buzzer started");
}

// 基础蜂鸣器控制
void buzzBeep(int duration) {
  if (!buzzerEnabled) return;
  for (char i = 0; i < 80; i++)
  {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(duration);
    digitalWrite(BUZZER_PIN, LOW);
    delay(duration);

  }

}

// 不同类型的提示音
void beepStartup() {
  // 开机提示音 - 短-长-短
  buzzBeep(1);
  delay(100);
  buzzBeep(3);
  delay(100);
  buzzBeep(1);
}

void beepWarning() {
  // 警告音 - 三次短促音
  for (int i = 0; i < 3; i++) {
    buzzBeep(2);
    delay(150);
  }
}

void beepAlarm() {
  // 报警音 - 长音
  buzzBeep(1);
}

void beepSuccess() {
  // 成功提示音 - 两次短音
  buzzBeep(1);
  delay(100);
  buzzBeep(1);
}

void beepError() {
  // 错误提示音 - 长-短-长
  buzzBeep(3);
  delay(100);
  buzzBeep(1);
  delay(100);
  buzzBeep(3);
}

// 蜂鸣器开关控制
void setBuzzerEnabled(bool enabled) {
  buzzerEnabled = enabled;
  if (!enabled) {
    digitalWrite(BUZZER_PIN, LOW);
  }
  Serial.printf("蜂鸣器: %s\n", enabled ? "启用" : "禁用");
}

// 检查传感器异常状态（使用统一判断逻辑）
void checkSensorAlarms(const SensorData& data) {
  if (!buzzerEnabled) return;
  
  static unsigned long lastAlarmTime = 0;
  const unsigned long ALARM_INTERVAL = 30000; // 30秒报警间隔
  
  unsigned long currentTime = millis();
  if (currentTime - lastAlarmTime < ALARM_INTERVAL) return;
  
  bool hasWarning = false;
  bool hasCritical = false;
  
  // 检查温度状态
  SensorStatus tempStatus = getSensorStatus(data.temperature, thresholds.tempMin, thresholds.tempMax);
  if (tempStatus == SENSOR_CRITICAL) hasCritical = true;
  else if (tempStatus == SENSOR_WARNING) hasWarning = true;
  
  // 检查湿度状态
  SensorStatus humidStatus = getSensorStatus(data.humidity, thresholds.humidityMin, thresholds.humidityMax);
  if (humidStatus == SENSOR_CRITICAL) hasCritical = true;
  else if (humidStatus == SENSOR_WARNING) hasWarning = true;
  
  // 检查CO2状态
  SensorStatus co2Status = getCO2SensorStatus(data.co2, thresholds.co2Min, thresholds.co2Max);
  if (co2Status == SENSOR_CRITICAL) hasCritical = true;
  else if (co2Status == SENSOR_WARNING) hasWarning = true;
  
  // 检查气压状态
  SensorStatus pressureStatus = getSensorStatus(data.pressure, thresholds.pressureMin, thresholds.pressureMax);
  if (pressureStatus == SENSOR_CRITICAL) hasCritical = true;
  else if (pressureStatus == SENSOR_WARNING) hasWarning = true;
  
  // 检查光强状态
  SensorStatus lightStatus = getSensorStatus(data.lightIntensity, thresholds.lightMin, thresholds.lightMax);
  if (lightStatus == SENSOR_CRITICAL) hasCritical = true;
  else if (lightStatus == SENSOR_WARNING) hasWarning = true;
  
  // 根据最严重的状态播放警报
  if (hasCritical) {
    beepAlarm();
    Serial.println("蜂鸣器: 危险状态警报");
    lastAlarmTime = currentTime;
  } else if (hasWarning) {
    beepWarning();
    Serial.println("蜂鸣器: 警告状态提示");
    lastAlarmTime = currentTime;
  }
}
