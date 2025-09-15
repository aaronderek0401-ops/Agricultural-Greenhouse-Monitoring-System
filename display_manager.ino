// 显示管理器 - 负责所有LCD相关功能
#include <LovyanGFX.hpp>

// LCD硬件配置类
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

// 显示相关全局变量
static LGFX lcd;

// 颜色定义 (RGB565格式) - 严格按照原文件
#define BG_COLOR     0x0000      // 白色背景
#define HEADER_COLOR 0xFFFF      // 黑色标题
#define TEXT_COLOR   0xFFFF      // 黑色文字
#define VALUE_COLOR  0xFFFF      // 黑色数值 √
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

// 显示管理器初始化
void initDisplay() {
  lcd.init();
  lcd.setRotation(0); // 设置竖屏
  lcd.fillScreen(BG_COLOR); // 清屏
  
  // 初始化图表数据
  for (int i = 0; i < MAX_POINTS; i++) {
    tempHistory[i] = 0;
  }
  historyIndex = 0;
  historyFull = false;
  lastGraphUpdate = millis();
}

// 显示启动画面
void showStartupScreen() {
  drawHeader();
  lcd.setTextColor(TEXT_COLOR);
  lcd.setTextSize(1.5);
  lcd.setCursor(20, 100);
  lcd.print("System Starting...");
  lcd.setCursor(20, 130);
  lcd.print("Initializing Sensors");
}

// 显示传感器初始化状态
void showSensorStatus(const char* sensor, int y) {
  lcd.setCursor(20, y);
  lcd.print(sensor);
}

// 显示初始化完成状态
void showInitComplete(bool aht30, bool bmp180, bool sgp30, bool bh1750) {
  lcd.setCursor(20, 240);
  lcd.setTextSize(2);
  
  int connectedCount = 0;
  if (aht30) connectedCount++;
  if (bmp180) connectedCount++;
  if (sgp30) connectedCount++;
  if (bh1750) connectedCount++;
  
  if (connectedCount == 4) {
    lcd.setTextColor(GOOD_COLOR);
    lcd.print("All sensors ready!");
  } else if (connectedCount > 0) {
    lcd.setTextColor(WARNING_COLOR);
    lcd.printf("%d/%d sensors ready", connectedCount, 4);
  } else {
    lcd.setTextColor(ALARM_COLOR);
    lcd.print("No sensors found");
  }
}

// 获取状态颜色
uint16_t getStatusColor(float value, float min, float max) {
  if (value < min || value > max) return ALARM_COLOR;
  if (value < min + (max-min)*0.1 || value > max - (max-min)*0.1) return WARNING_COLOR;
  return GOOD_COLOR;
}

uint16_t getCO2StatusColor(int value, struct Thresholds& thresholds) {
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
    lcd.setCursor(RIGHT_COL, y + 8);
    lcd.printf("%.1f%s", value, unit);
    
    // 绘制状态指示器
    lcd.fillCircle(STATUS_COL, y + 12, 5, statusColor);
  }
}

// 绘制整数传感器数据项
void drawSensorItemInt(int y, const char* label, int value, const char* unit, uint16_t statusColor) {
  lcd.fillRect(0, y, lcd.width(), ITEM_HEIGHT, BG_COLOR);
  
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
    lcd.fillCircle(STATUS_COL, y + 15, 5, ALARM_COLOR);
  } else {
    // 正常显示数值
    lcd.setTextColor(VALUE_COLOR);
    lcd.setTextSize(1.7);
    lcd.setCursor(RIGHT_COL, y + 8);
    lcd.printf("%d%s", value, unit);
    
    // 绘制状态指示器
    lcd.fillCircle(STATUS_COL, y + 15, 5, statusColor);
  }
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

// 更新完整显示 - 主要的显示更新函数
void updateDisplay(SensorData& data, struct Thresholds& thresholds) {
  int y = HEADER_HEIGHT + 5;
  
  // 绘制各个传感器数据
  uint16_t tempColor = (data.temperature == -999) ? ALARM_COLOR : 
                       getStatusColor(data.temperature, thresholds.tempMin, thresholds.tempMax);
  drawSensorItem(y, "Temperature", data.temperature, "C", tempColor);
  y += ITEM_HEIGHT;
  
  uint16_t humidColor = (data.humidity == -999) ? ALARM_COLOR : 
                        getStatusColor(data.humidity, thresholds.humidityMin, thresholds.humidityMax);
  drawSensorItem(y, "Humidity", data.humidity, "%", humidColor);
  y += ITEM_HEIGHT;
  
  uint16_t pressureColor = (data.pressure == -999) ? ALARM_COLOR : GOOD_COLOR;
  drawSensorItem(y, "Pressure", data.pressure, "hPa", pressureColor);
  y += ITEM_HEIGHT;
  
  uint16_t co2Color = (data.co2 == -999) ? ALARM_COLOR : 
                      getCO2StatusColor(data.co2, thresholds);
  drawSensorItemInt(y, "CO2", data.co2, "ppm", co2Color);
  y += ITEM_HEIGHT;
  
  drawSensorItem(y, "Light(lux)", data.lightIntensity, " ", GOOD_COLOR);
  y += ITEM_HEIGHT;
}

// 处理图表更新
void handleGraphUpdate(unsigned long currentTime, SensorData& data) {
  // 每10秒记录一次温度数据用于图表
  if (currentTime - lastGraphUpdate >= GRAPH_UPDATE_INTERVAL) {
    lastGraphUpdate = currentTime;
    addTemperatureData(data.temperature);
  }
  
  // 绘制温度折线图
  drawTemperatureGraph();
}

// 完整的界面重绘
void refreshDisplay() {
  lcd.fillScreen(BG_COLOR);
  drawHeader();
}
