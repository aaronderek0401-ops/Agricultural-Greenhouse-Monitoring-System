// 时间管理模块
// 提供简单的时间显示选项

// 时间显示模式
enum TimeDisplayMode {
  MODE_RUNTIME,    // 显示运行时间（从启动开始）
  MODE_SIMPLE      // 显示简单计数时间
};

// 当前时间显示模式
TimeDisplayMode currentTimeMode = MODE_RUNTIME;

// 初始化时间系统
void initTimeSystem() {
  Serial.println("Time system initialized - showing runtime");
}

// 获取格式化的时间字符串
String getFormattedTime() {
  switch (currentTimeMode) {
    case MODE_RUNTIME: {
      // 显示从启动开始的运行时间
      unsigned long totalSeconds = millis() / 1000;
      unsigned long hours = (totalSeconds / 3600) % 24;
      unsigned long minutes = (totalSeconds / 60) % 60;
      return String(hours < 10 ? "0" : "") + String(hours) + ":" + 
             String(minutes < 10 ? "0" : "") + String(minutes);
    }
    
    case MODE_SIMPLE: {
      // 简单的循环计数（0-23小时，0-59分钟）
      unsigned long totalMinutes = (millis() / 60000) % 1440; // 一天1440分钟
      unsigned long hours = totalMinutes / 60;
      unsigned long minutes = totalMinutes % 60;
      return String(hours < 10 ? "0" : "") + String(hours) + ":" + 
             String(minutes < 10 ? "0" : "") + String(minutes);
    }
    
    default:
      return "00:00";
  }
}

// 切换时间显示模式
void switchTimeMode() {
  switch (currentTimeMode) {
    case MODE_RUNTIME:
      currentTimeMode = MODE_SIMPLE;
      Serial.println("Switched to simple time mode");
      break;
    case MODE_SIMPLE:
      currentTimeMode = MODE_RUNTIME;
      Serial.println("Switched to runtime mode");
      break;
  }
}
