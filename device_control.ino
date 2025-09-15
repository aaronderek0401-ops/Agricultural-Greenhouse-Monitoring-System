// 设备控制模块 - 蜂鸣器控制
// 负责系统提示音和报警功能

#define BUZZER_PIN 9          // 蜂鸣器引脚D6

// 蜂鸣器状态
static bool buzzerEnabled = true;

// 初始化蜂鸣器
void initDeviceControl() {
  pinMode(BUZZER_PIN, OUTPUT);
  Serial.println("蜂鸣器初始化完成");
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
