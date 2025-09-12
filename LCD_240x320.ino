#include <LovyanGFX.hpp>

// 传感器数据结构
struct SensorData {
  float temperature;    // 温度 (°C)
  float humidity;      // 湿度 (%)
  float pressure;      // 气压 (hPa)
  float elevation;     // 海拔高度 (m)
  int co2;            // 二氧化碳浓度 (ppm)
  float soilMoisture; // 土壤湿度 (%)
  float lightIntensity; // 光照强度 (lux)
  bool pumpStatus;    // 水泵状态
  bool fanStatus;     // 风扇状态
  bool lightStatus;   // 补光灯状态
};

class LGFX : public lgfx::LGFX_Device
{
  lgfx::Panel_ST7789      _panel_instance;  // 使用ST7789屏幕驱动
  lgfx::Bus_SPI        _bus_instance;   // 使用SPI总线驱动

  // 使用PWM控制背光
  lgfx::Light_PWM     _light_instance;

public:
  LGFX(void)
  {
    {
      auto cfg = _bus_instance.config();    // 调用总线配置结构体

      // SPI总线配置
      cfg.spi_host = SPI3_HOST;     // 使用SPI3总线 (可选有SPI2_HOST, SPI3_HOST)
      cfg.spi_mode = 0;             // SPI通信模式 (0 ~ 3)
      cfg.freq_write = 40000000;    // SPI时钟频率 (最大80MHz, 80MHz整数分频率)
      cfg.freq_read  = 16000000;    // SPI读取时的时钟频率 
      cfg.spi_3wire  = true;        // SPI3线模式
      cfg.use_lock   = true;        // 多任务使用总线时请设置为true
      cfg.dma_channel = SPI_DMA_CH_AUTO; // DMA通道设置(0=不使用DMA, 1=使用DMA通道1, 2=使用DMA通道2, SPI_DMA_CH_AUTO=自动选择)
      cfg.pin_sclk = 3;            // SPI的SCLK引脚
      cfg.pin_mosi = 4;            // SPI的MOSI引脚
      cfg.pin_miso = -1;            // SPI的MISO引脚 (-1 = disable)
      cfg.pin_dc   = 12;            // SPI的DC引脚 (-1 = disable)

      _bus_instance.config(cfg);    // 配置总线
      _panel_instance.setBus(&_bus_instance);      // 将总线设置为面板
    }

    {
      auto cfg = _panel_instance.config();    // 调用面板配置结构体

      cfg.pin_cs           =    13;  // CS引脚 (-1 = disable)
      cfg.pin_rst          =    11;  // RST引脚 (-1 = disable)
      cfg.pin_busy         =    -1;  // BUSY引脚 (-1 = disable)
      cfg.panel_width      =   240;  // 面板的实际宽度
      cfg.panel_height     =   320;  // 面板的实际高度
      cfg.offset_x         =     0;  // 偏移量X方向
      cfg.offset_y         =     0;  // 偏移量Y方向
      cfg.offset_rotation  =     0;  // 旋转时的偏移量 0~7
      cfg.dummy_read_pixel =     8;  // 读取像素前的虚拟读取位数
      cfg.dummy_read_bits  =     1;  // 读取位前的虚拟读取位数
      cfg.readable         =  true;  // 是否可读
      cfg.invert           =  true;  // 是否反转
      cfg.rgb_order        = false;  // RGB顺序为false, BGR顺序为true
      cfg.dlen_16bit       = false;  // 16bit数据长度为true, 9bit数据长度为false
      cfg.bus_shared       =  true;  // 总线共享为true, 独占总线为false
      _panel_instance.config(cfg);
    }

    {
      auto cfg = _light_instance.config();    // 调用背光配置结构体

      cfg.pin_bl = 14;              // 背光引脚 (-1 = disable)
      cfg.invert = false;           // 背光反转
      cfg.freq   = 44100;           // PWM频率
      cfg.pwm_channel = 7;          // PWM通道 0~15

      _light_instance.config(cfg);
      _panel_instance.setLight(&_light_instance);  // 将背光设置为面板
      setPanel(&_panel_instance); // 将面板设置为显示设备
    }
  }
};

static LGFX lcd;
static SensorData sensorData;

// 传感器连接状态
static bool aht30Connected = false;
static bool bmp180Connected = false;

static unsigned long lastUpdate = 0;
static const unsigned long UPDATE_INTERVAL = 2000; // 2秒更新一次

// 颜色定义 (RGB565格式)
#define BG_COLOR     0x0000      // 白色背景
#define HEADER_COLOR 0xFFFF      // 绿色标题
#define TEXT_COLOR   0xFFFF      // 黑色文字
#define VALUE_COLOR  0xFFFF      // 黑色数值
#define ALARM_COLOR  0x07FF      // 红色警告 √
#define GOOD_COLOR   0xF81F      // 绿色正常 √
#define WARNING_COLOR 0x001F     // 黄色警告 √

// 0xFFE0 蓝色
// 0xF800 天青色
// 0x07E0 紫色

// 显示布局参数
#define HEADER_HEIGHT 30
#define ITEM_HEIGHT 35
#define LEFT_COL 10
#define RIGHT_COL 130
#define STATUS_COL 230

// 折线图参数
#define GRAPH_X 10           // 图表左边距
#define GRAPH_Y 220          // 图表顶部位置
#define GRAPH_WIDTH 220      // 图表宽度
#define GRAPH_HEIGHT 80      // 图表高度
#define MAX_POINTS 50        // 最大数据点数量

// 温度历史数据
float tempHistory[MAX_POINTS];
int historyIndex = 0;
bool historyFull = false;
unsigned long lastGraphUpdate = 0;
#define GRAPH_UPDATE_INTERVAL 10000  // 10秒更新一次图表数据

// 阈值设定
struct Thresholds {
  float tempMin = 18.0, tempMax = 28.0;
  float humidityMin = 50.0, humidityMax = 80.0;
  int co2Min = 400, co2Max = 1200;
  float soilMin = 40.0, soilMax = 70.0;
} thresholds;

// 读取传感器数据 (温湿度 + 气压)
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
  
  // 其他传感器暂时使用模拟数据
  sensorData.co2 = 800 + random(-100, 100);
  sensorData.soilMoisture = 55.0 + random(-10, 10);
  sensorData.lightIntensity = 25000 + random(-5000, 5000);
  
  // 模拟设备状态
  sensorData.pumpStatus = (sensorData.soilMoisture < thresholds.soilMin);
  sensorData.fanStatus = (sensorData.temperature > thresholds.tempMax);
  sensorData.lightStatus = (sensorData.lightIntensity < 20000);
}

// 获取状态颜色
uint16_t getStatusColor(float value, float min, float max) {
  if (value < min || value > max) return ALARM_COLOR;
  if (value < min + (max-min)*0.1 || value > max - (max-min)*0.1) return WARNING_COLOR;
  return GOOD_COLOR;
}

uint16_t getCO2StatusColor(int value) {
  if (value < thresholds.co2Min || value > thresholds.co2Max) return ALARM_COLOR;
  if (value < thresholds.co2Min + 100 || value > thresholds.co2Max - 100) return WARNING_COLOR;
  return GOOD_COLOR;
}

// 绘制标题栏
void drawHeader() {
  lcd.fillRect(0, 0, lcd.width(), HEADER_HEIGHT, HEADER_COLOR);
  lcd.setTextColor(BG_COLOR);
  lcd.setTextSize(1.5);
  lcd.setCursor(10, 8);
  lcd.print("Greenhouse Monitor");
  
  // 显示时间
  lcd.setTextSize(1);
  lcd.setCursor(lcd.width() - 50, 12);
  String timeStr = getFormattedTime();
  lcd.print(timeStr);
}

// 绘制传感器数据项
void drawSensorItem(int y, const char* label, float value, const char* unit, uint16_t statusColor) {
  // 清除行背景
  lcd.fillRect(0, y, lcd.width(), ITEM_HEIGHT, BG_COLOR);
  
  // 绘制标签
  lcd.setTextColor(TEXT_COLOR);
  lcd.setTextSize(1.5);
  lcd.setCursor(LEFT_COL, y + 8);
  lcd.print(label);
  
  // 检查是否为未连接状态
  if (value == -999) {
    // 显示disconnected
    lcd.setTextColor(ALARM_COLOR);
    lcd.setTextSize(1.2);
    lcd.setCursor(RIGHT_COL, y + 8);
    lcd.print("disconnected");
    
    // 绘制红色状态指示器
    lcd.fillCircle(STATUS_COL, y + 12, 5, ALARM_COLOR);
  } else {
    // 正常显示数值
    lcd.setTextColor(VALUE_COLOR);
    lcd.setTextSize(1.7);
    lcd.setCursor(RIGHT_COL, y + 5);
    lcd.printf("%.1f%s", value, unit);
    
    // 绘制状态指示器
    lcd.fillCircle(STATUS_COL, y + 12, 5, statusColor);
  }
}

// 绘制整数传感器数据项
void drawSensorItemInt(int y, const char* label, int value, const char* unit, uint16_t statusColor) {
  lcd.fillRect(0, y, lcd.width(), ITEM_HEIGHT, BG_COLOR);
  
  lcd.setTextColor(TEXT_COLOR);
  lcd.setTextSize(1.7);
  lcd.setCursor(LEFT_COL, y + 8);
  lcd.print(label);
  
  lcd.setTextColor(VALUE_COLOR);
  lcd.setTextSize(2);
  lcd.setCursor(RIGHT_COL, y + 5);
  lcd.printf("%d%s", value, unit);
  
  lcd.fillCircle(STATUS_COL, y + 12, 5, statusColor);
}

// 添加温度数据到历史记录
void addTemperatureData(float temp) {
  if (temp == -999) return; // 跳过无效数据
  
  tempHistory[historyIndex] = temp;
  historyIndex++;
  
  if (historyIndex >= MAX_POINTS) {
    historyIndex = 0;
    historyFull = true;
  }
}

// 绘制温度折线图
void drawTemperatureGraph() {
  // 清除图表区域
  lcd.fillRect(GRAPH_X, GRAPH_Y, GRAPH_WIDTH, GRAPH_HEIGHT, BG_COLOR);
  
  // 绘制图表边框
  lcd.drawRect(GRAPH_X, GRAPH_Y, GRAPH_WIDTH, GRAPH_HEIGHT, TEXT_COLOR);
  
  // 图表标题
  lcd.setTextColor(HEADER_COLOR);
  lcd.setTextSize(1);
  lcd.setCursor(GRAPH_X + 5, GRAPH_Y - 15);
  lcd.print("Temperature Trend (Celsius)");
  
  // 如果没有足够的数据，直接返回
  int dataCount = historyFull ? MAX_POINTS : historyIndex;
  if (dataCount < 2) return;
  
  // 计算温度范围
  float minTemp = 999, maxTemp = -999;
  float mintemp_ = 999, maxtemp_ = -999;

  for (int i = 0; i < dataCount; i++) {
    int idx = historyFull ? (historyIndex + i) % MAX_POINTS : i;
    if (tempHistory[idx] < minTemp) minTemp = tempHistory[idx];
    if (tempHistory[idx] > maxTemp) maxTemp = tempHistory[idx];
  }
  
  // 添加一些边距
  float tempRange = maxTemp - minTemp;
  if (tempRange < 5) {
    // 如果变化太小，固定显示范围
    float center = (maxTemp + minTemp) / 2;
    mintemp_ = center - 2.5;
    maxtemp_ = center + 2.5;

    // maxTemp = center + 2.5;
    tempRange = 5;
  } else {
    mintemp_ = minTemp - tempRange * 0.1;
    maxtemp_ = maxTemp + tempRange * 0.1;
      // minTemp -= tempRange * 0.1;
      // maxTemp += tempRange * 0.1;
    tempRange = maxtemp_ - mintemp_;
  }
  
  // 绘制Y轴刻度
  // lcd.setTextColor(TEXT_COLOR);
  // lcd.setTextSize(0.8);
  // lcd.setCursor(GRAPH_X - 25, GRAPH_Y + 2);
  // lcd.printf("%.1f", maxTemp);
  // lcd.setCursor(GRAPH_X - 25, GRAPH_Y + GRAPH_HEIGHT - 8);
  // lcd.printf("%.1f", minTemp);
  
  // 绘制数据点和连线
  for (int i = 1; i < dataCount; i++) {
    int idx1 = historyFull ? (historyIndex + i - 1) % MAX_POINTS : i - 1;
    int idx2 = historyFull ? (historyIndex + i) % MAX_POINTS : i;
    
    // 计算屏幕坐标
    int x1 = GRAPH_X + ((i - 1) * GRAPH_WIDTH) / (dataCount - 1);
    int x2 = GRAPH_X + (i * GRAPH_WIDTH) / (dataCount - 1);
    
    int y1 = GRAPH_Y + GRAPH_HEIGHT - ((tempHistory[idx1] - mintemp_) / tempRange) * GRAPH_HEIGHT;
    int y2 = GRAPH_Y + GRAPH_HEIGHT - ((tempHistory[idx2] - mintemp_) / tempRange) * GRAPH_HEIGHT;
    
    // 绘制连线
    lcd.drawLine(x1, y1, x2, y2, VALUE_COLOR);
    
    // 绘制数据点
    lcd.fillCircle(x2, y2, 1, GOOD_COLOR);
  }
  
  // 显示图中最高温度值
  if (dataCount > 0) {
    lcd.fillRect(0, GRAPH_Y + GRAPH_HEIGHT + 5, lcd.width(), ITEM_HEIGHT, BG_COLOR);

    lcd.setTextColor(VALUE_COLOR);
    lcd.setTextSize(1.7);
    lcd.setCursor(GRAPH_X + GRAPH_WIDTH - 140, GRAPH_Y + GRAPH_HEIGHT + 5);
    lcd.printf("Highest: %.1fC", maxTemp);
  }
}

/*
// 绘制设备状态
void drawDeviceStatus() {
  int y = HEADER_HEIGHT + 6 * ITEM_HEIGHT + 10;
  
  // 标题
  lcd.fillRect(0, y, lcd.width(), 25, 0x2104);
  lcd.setTextColor(TEXT_COLOR);
  lcd.setTextSize(1.5);
  lcd.setCursor(LEFT_COL, y + 5);
  lcd.print("Device Status");
  
  y += 30;
  
  // 水泵状态
  lcd.fillRect(0, y, lcd.width(), 25, BG_COLOR);
  lcd.setTextColor(TEXT_COLOR);
  lcd.setTextSize(1);
  lcd.setCursor(LEFT_COL, y + 8);
  lcd.print("Pump:");
  lcd.setTextColor(sensorData.pumpStatus ? GOOD_COLOR : TEXT_COLOR);
  lcd.setCursor(50, y + 8);
  lcd.print(sensorData.pumpStatus ? "ON " : "OFF");
  
  // 风扇状态
  lcd.setTextColor(TEXT_COLOR);
  lcd.setCursor(120, y + 8);
  lcd.print("Fan:");
  lcd.setTextColor(sensorData.fanStatus ? GOOD_COLOR : TEXT_COLOR);
  lcd.setCursor(150, y + 8);
  lcd.print(sensorData.fanStatus ? "ON " : "OFF");
  
  y += 20;
  
  // 补光灯状态
  lcd.fillRect(0, y, lcd.width(), 25, BG_COLOR);
  lcd.setTextColor(TEXT_COLOR);
  lcd.setCursor(LEFT_COL, y + 8);
  lcd.print("Light:");
  lcd.setTextColor(sensorData.lightStatus ? GOOD_COLOR : TEXT_COLOR);
  lcd.setCursor(60, y + 8);
  lcd.print(sensorData.lightStatus ? "ON " : "OFF");
}
*/

// 更新显示
void updateDisplay() {
  int y = HEADER_HEIGHT + 5;
  
  // 绘制各个传感器数据
  uint16_t tempColor = (sensorData.temperature == -999) ? ALARM_COLOR : 
                       getStatusColor(sensorData.temperature, thresholds.tempMin, thresholds.tempMax);
  drawSensorItem(y, "Temperature", sensorData.temperature, "C", tempColor);
  y += ITEM_HEIGHT;
  
  uint16_t humidColor = (sensorData.humidity == -999) ? ALARM_COLOR : 
                        getStatusColor(sensorData.humidity, thresholds.humidityMin, thresholds.humidityMax);
  drawSensorItem(y, "Humidity", sensorData.humidity, "%", humidColor);
  y += ITEM_HEIGHT;
  
  uint16_t pressureColor = (sensorData.pressure == -999) ? ALARM_COLOR : GOOD_COLOR;
  drawSensorItem(y, "Pressure", sensorData.pressure, "hPa", pressureColor);
  y += ITEM_HEIGHT;
  
  // uint16_t elevationColor = (sensorData.elevation == -999) ? ALARM_COLOR : GOOD_COLOR;
  // drawSensorItem(y, "Elevation", sensorData.elevation, "m", elevationColor);
  // y += ITEM_HEIGHT;
  
  drawSensorItemInt(y, "CO2", sensorData.co2, "ppm", getCO2StatusColor(sensorData.co2));
  y += ITEM_HEIGHT;
  
  // drawSensorItem(y, "Soil Moist", sensorData.soilMoisture, "%", 
  //                getStatusColor(sensorData.soilMoisture, thresholds.soilMin, thresholds.soilMax));
  // y += ITEM_HEIGHT;
  
  drawSensorItem(y, "Light", sensorData.lightIntensity, "lux", GOOD_COLOR);
  y += ITEM_HEIGHT;
  
  // 绘制设备状态
  // drawDeviceStatus();

  // 绘制初始图表框架
  drawTemperatureGraph();
}

void setup(void)
{
  Serial.begin(115200);
  Serial.println("=== Greenhouse Monitoring System ===");
  Serial.println("Initializing...");
  
  // 首先初始化LCD显示屏
  lcd.init();
  lcd.setRotation(0); // 设置竖屏
  lcd.fillScreen(BG_COLOR); // 清屏
  
  // 立即显示启动界面
  drawHeader();
  lcd.setTextColor(TEXT_COLOR);
  lcd.setTextSize(1.5);
  lcd.setCursor(20, 100);
  lcd.print("System Starting...");
  lcd.setCursor(20, 130);
  lcd.print("Initializing Sensors");
  
  // 初始化时间系统
  initTimeSystem();
  
  // 现在尝试初始化传感器（启用温湿度传感器）
  Serial.println("Sensor initialization mode: ALL SENSORS ENABLED");
  
  // 尝试初始化温湿度传感器
  lcd.setCursor(20, 160);
  lcd.print("AHT30...");
  Serial.println("Attempting AHT30 init...");
  aht30Connected = initTemperatureHumiditySensor();  // 温湿度传感器
  Serial.printf("AHT30 result: %s\n", aht30Connected ? "SUCCESS" : "FAILED");
  
  // 尝试初始化气压传感器
  lcd.setCursor(20, 180);
  lcd.print("BMP180...");
  Serial.println("Attempting BMP180 init...");
  bmp180Connected = initPressureSensor();  // 气压传感器
  Serial.printf("BMP180 result: %s\n", bmp180Connected ? "SUCCESS" : "FAILED");

  
  // 显示传感器状态
  lcd.setCursor(20, 210);
  lcd.setTextSize(2);
  if (aht30Connected && bmp180Connected) {
    lcd.setTextColor(GOOD_COLOR);
    lcd.print("All sensors ready!");
  } else if (aht30Connected || bmp180Connected) {
    lcd.setTextColor(WARNING_COLOR);
    lcd.print("Some sensors ready");
  } else {
    lcd.setTextColor(ALARM_COLOR);
    lcd.print("No sensors found");
  }

  delay(5000); // 显示状态5秒

  // 清屏并绘制正常界面
  lcd.fillScreen(BG_COLOR);
  drawHeader();
  
  // 初始化图表数据
  for (int i = 0; i < MAX_POINTS; i++) {
    tempHistory[i] = 0;
  }
  historyIndex = 0;
  historyFull = false;
  lastGraphUpdate = millis();
  
  // 初始化传感器数据
  readSensors();
  updateDisplay();
  
 
  
  Serial.println("Greenhouse Monitoring System Started");
  Serial.printf("Sensors: AHT30=%s, BMP180=%s\n", 
                aht30Connected ? "OK" : "FAIL", 
                bmp180Connected ? "OK" : "FAIL");
}

void loop(void)
{
  unsigned long currentTime = millis();
  
  // 每2秒更新一次数据
  if (currentTime - lastUpdate >= UPDATE_INTERVAL) {
    lastUpdate = currentTime;
    
    // 读取传感器数据
    readSensors();
    
    // 每10秒记录一次温度数据用于图表
    if (currentTime - lastGraphUpdate >= GRAPH_UPDATE_INTERVAL) {
      lastGraphUpdate = currentTime;
      addTemperatureData(sensorData.temperature);
    }
    
    // 更新标题栏（时间）
    drawHeader();
    
    // 更新显示
    updateDisplay();
    
    // 绘制温度折线图
    drawTemperatureGraph();
    
    // 串口输出数据用于调试
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
    
    // 其他传感器（模拟数据）
    Serial.printf("CO2:%dppm Soil:%.1f%% Light:%.1flux\n", 
                  sensorData.co2, sensorData.soilMoisture, sensorData.lightIntensity);
  }
  
  delay(100); // 短暂延时，避免过度占用CPU
}
