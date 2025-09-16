// wifi_manager.ino - WiFi热点Web服务器通信管理
// Arduino Nano ESP32 WiFi热点，用于手机/电脑监控温室数据

#include <WiFi.h>
#include <WebServer.h>
// #include <ArduinoJson.h> // 暂时注释，使用String拼接JSON

// WiFi热点配置
const char* ap_ssid = "ESP32_Greenhouse";
const char* ap_password = "12345678"; // 8位密码

// Web服务器
WebServer server(80);
bool wifiConnected = false;
unsigned long lastDataUpdate = 0;
const unsigned long DATA_UPDATE_INTERVAL = 2000; // 2秒更新一次数据缓存

// 传感器数据缓存
String cachedSensorData = "";

// WiFi热点初始化
void initWiFiHotspot() {
  Serial.println("Initializing WiFi Hotspot...");
  
  // 配置WiFi热点
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_password);
  
  IPAddress IP = WiFi.softAPIP();
  Serial.print("Hotspot IP: ");
  Serial.println(IP);
  Serial.println("Hotspot Name: " + String(ap_ssid));
  Serial.println("Hotspot Password: " + String(ap_password));
  
  // 配置Web服务器路由
  setupWebRoutes();
  
  // 启动Web服务器
  server.begin();
  Serial.println("Web Server Started");
  Serial.println("Access URL: http://" + IP.toString());
  
  wifiConnected = true;
}

// 配置Web服务器路由
void setupWebRoutes() {
  // 主页 - 显示监控界面
  server.on("/", HTTP_GET, handleRoot);
  
  // API - 获取传感器数据(JSON格式)
  server.on("/api/data", HTTP_GET, handleAPIData);
  
  // API - 获取系统状态
  server.on("/api/status", HTTP_GET, handleAPIStatus);
  
  // API - 重启系统
  server.on("/api/restart", HTTP_POST, handleAPIRestart);
  
  // 404页面
  server.onNotFound(handleNotFound);
}

// 主页处理函数
void handleRoot() {
  String html = generateWebPage();
  server.send(200, "text/html", html);
}

// API数据处理函数
void handleAPIData() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", createSensorDataJSON());
}

// API状态处理函数
void handleAPIStatus() {
  String status = "{";
  status += "\"wifi_clients\":" + String(WiFi.softAPgetStationNum()) + ",";
  status += "\"ip\":\"" + WiFi.softAPIP().toString() + "\",";
  status += "\"uptime\":" + String(millis()/1000) + ",";
  status += "\"memory\":" + String(ESP.getFreeHeap());
  status += "}";
  
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", status);
}

// API重启处理函数
void handleAPIRestart() {
  server.send(200, "text/plain", "System Restarting...");
  delay(1000);
  ESP.restart();
}

// 404错误处理
void handleNotFound() {
  server.send(404, "text/plain", "Page Not Found");
}

// 生成Web监控页面
String generateWebPage() {
  String html = "";
  html += "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>ESP32 Greenhouse Monitor</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 20px; background: #f0f0f0; }";
  html += ".container { max-width: 800px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; }";
  html += ".header { text-align: center; color: #2c3e50; margin-bottom: 30px; }";
  html += ".sensor-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 15px; }";
  html += ".sensor-card { background: #ecf0f1; padding: 15px; border-radius: 8px; text-align: center; }";
  html += ".sensor-value { font-size: 24px; font-weight: bold; color: #2980b9; }";
  html += ".sensor-label { color: #7f8c8d; margin-top: 5px; }";
  html += ".status { padding: 10px; margin: 20px 0; border-radius: 5px; text-align: center; }";
  html += ".status.normal { background: #d5f4e6; color: #27ae60; }";
  html += ".status.warning { background: #fdeaa7; color: #f39c12; }";
  html += ".status.error { background: #fadbd8; color: #e74c3c; }";
  html += ".controls { text-align: center; margin-top: 20px; }";
  html += ".btn { padding: 10px 20px; margin: 5px; background: #3498db; color: white; border: none; border-radius: 5px; cursor: pointer; }";
  html += ".btn:hover { background: #2980b9; }";
  html += ".refresh-info { text-align: center; color: #7f8c8d; margin-top: 10px; }";
  html += "</style></head><body>";
  
  html += "<div class='container'>";
  html += "<div class='header'>";
  html += "<h1>🌱 ESP32 Greenhouse Monitor</h1>";
  html += "<p>Real-time Sensor Data Monitoring</p>";
  html += "</div>";
  
  html += "<div id='sensorData' class='sensor-grid'>";
  html += "<div class='sensor-card'><div class='sensor-value' id='temperature'>--</div><div class='sensor-label'>Temperature (C)</div></div>";
  html += "<div class='sensor-card'><div class='sensor-value' id='humidity'>--</div><div class='sensor-label'>Humidity (%)</div></div>";
  html += "<div class='sensor-card'><div class='sensor-value' id='pressure'>--</div><div class='sensor-label'>Pressure (hPa)</div></div>";
  html += "<div class='sensor-card'><div class='sensor-value' id='co2'>--</div><div class='sensor-label'>CO2 (ppm)</div></div>";
  html += "<div class='sensor-card'><div class='sensor-value' id='light'>--</div><div class='sensor-label'>Light (lux)</div></div>";
  html += "<div class='sensor-card'><div class='sensor-value' id='uptime'>--</div><div class='sensor-label'>Uptime (sec)</div></div>";
  html += "</div>";
  
  html += "<div id='systemStatus' class='status normal'>System Status: Normal</div>";
  
  html += "<div class='controls'>";
  html += "<button class='btn' onclick='refreshData()'>Refresh Data</button>";
  html += "<button class='btn' onclick='toggleAutoRefresh()'>Auto Refresh: <span id='autoStatus'>ON</span></button>";
  html += "</div>";
  
  html += "<div class='refresh-info'>";
  html += "<p>Last Update: <span id='lastUpdate'>--</span></p>";
  html += "<p>Connected Devices: <span id='clientCount'>--</span></p>";
  html += "</div>";
  html += "</div>";
  
  // JavaScript部分
  html += "<script>";
  html += "var autoRefresh = true;";
  html += "var refreshInterval;";
  
  html += "function updateSensorData() {";
  html += "fetch('/api/data').then(function(response) { return response.json(); }).then(function(data) {";
  html += "document.getElementById('temperature').textContent = data.temperature !== null ? data.temperature.toFixed(1) : '--';";
  html += "document.getElementById('humidity').textContent = data.humidity !== null ? data.humidity.toFixed(1) : '--';";
  html += "document.getElementById('pressure').textContent = data.pressure !== null ? data.pressure.toFixed(1) : '--';";
  html += "document.getElementById('co2').textContent = data.co2 !== null ? data.co2 : '--';";
  html += "document.getElementById('light').textContent = data.light !== null ? data.light.toFixed(0) : '--';";
  html += "document.getElementById('uptime').textContent = data.uptime || '--';";
  
  html += "var statusDiv = document.getElementById('systemStatus');";
  html += "if (data.status === 'normal') {";
  html += "statusDiv.className = 'status normal';";
  html += "statusDiv.textContent = 'System Status: Normal';";
  html += "} else {";
  html += "statusDiv.className = 'status warning';";
  html += "statusDiv.textContent = 'System Status: Warning Detected';";
  html += "}";
  
  html += "document.getElementById('lastUpdate').textContent = new Date().toLocaleTimeString();";
  html += "}).catch(function(error) {";
  html += "console.error('Data fetch failed:', error);";
  html += "document.getElementById('systemStatus').className = 'status error';";
  html += "document.getElementById('systemStatus').textContent = 'System Status: Connection Error';";
  html += "});";
  
  html += "fetch('/api/status').then(function(response) { return response.json(); }).then(function(data) {";
  html += "document.getElementById('clientCount').textContent = data.wifi_clients || 0;";
  html += "}).catch(function(error) { console.error('Status fetch failed:', error); });";
  html += "}";
  
  html += "function refreshData() { updateSensorData(); }";
  
  html += "function toggleAutoRefresh() {";
  html += "autoRefresh = !autoRefresh;";
  html += "document.getElementById('autoStatus').textContent = autoRefresh ? 'ON' : 'OFF';";
  html += "if (autoRefresh) {";
  html += "refreshInterval = setInterval(updateSensorData, 3000);";
  html += "} else {";
  html += "clearInterval(refreshInterval);";
  html += "}";
  html += "}";
  
  html += "window.onload = function() {";
  html += "updateSensorData();";
  html += "refreshInterval = setInterval(updateSensorData, 3000);";
  html += "};";
  html += "</script>";
  
  html += "</body></html>";
  
  return html;
}

// 检查WiFi连接状态
bool isWiFiConnected() {
  return wifiConnected && (WiFi.softAPgetStationNum() > 0);
}

// WiFi管理主循环
void wifiLoop() {
  // 处理Web服务器请求
  server.handleClient();
  
  // 更新传感器数据缓存
  unsigned long currentTime = millis();
  if (currentTime - lastDataUpdate >= DATA_UPDATE_INTERVAL) {
    cachedSensorData = createSensorDataJSON();
    lastDataUpdate = currentTime;
  }
}

// 创建JSON格式的传感器数据
String createSensorDataJSON() {
  String json = "{";
  
  // 添加时间戳
  json += "\"timestamp\":\"" + String(millis()/1000) + "s\",";
  
  // 温湿度数据
  if (aht30Connected && sensorData.temperature != -999) {
    json += "\"temperature\":" + String(sensorData.temperature, 1) + ",";
    json += "\"humidity\":" + String(sensorData.humidity, 1) + ",";
  } else {
    json += "\"temperature\":null,";
    json += "\"humidity\":null,";
  }
  
  // 气压数据
  if (bmp180Connected && sensorData.pressure != -999) {
    json += "\"pressure\":" + String(sensorData.pressure, 2) + ",";
    json += "\"elevation\":" + String(sensorData.elevation, 1) + ",";
  } else {
    json += "\"pressure\":null,";
    json += "\"elevation\":null,";
  }
  
  // CO2数据
  if (sgp30Connected && sensorData.co2 != -999) {
    json += "\"co2\":" + String(sensorData.co2) + ",";
    json += "\"tvoc\":" + String(sensorData.tvoc) + ",";
  } else {
    json += "\"co2\":null,";
    json += "\"tvoc\":null,";
  }
  
  // 光照数据
  if (bh1750Connected && sensorData.lightIntensity != -999) {
    json += "\"light\":" + String(sensorData.lightIntensity, 0) + ",";
  } else {
    json += "\"light\":null,";
  }
  
  // 系统状态
  json += "\"sensors\":{";
  json += "\"aht30\":" + String(aht30Connected ? "true" : "false") + ",";
  json += "\"bmp180\":" + String(bmp180Connected ? "true" : "false") + ",";
  json += "\"sgp30\":" + String(sgp30Connected ? "true" : "false") + ",";
  json += "\"bh1750\":" + String(bh1750Connected ? "true" : "false");
  json += "},";
  
  // 运行状态
  json += "\"status\":\"";
  bool hasAbnormal = false;
  if (aht30Connected && (sensorData.temperature < thresholds.tempMin || sensorData.temperature > thresholds.tempMax)) hasAbnormal = true;
  if (aht30Connected && (sensorData.humidity < thresholds.humidityMin || sensorData.humidity > thresholds.humidityMax)) hasAbnormal = true;
  if (sgp30Connected && (sensorData.co2 < thresholds.co2Min || sensorData.co2 > thresholds.co2Max)) hasAbnormal = true;
  
  if (hasAbnormal) {
    json += "abnormal";
  } else {
    json += "normal";
  }
  json += "\",";
  
  // 内存和运行时间
  json += "\"memory\":" + String(ESP.getFreeHeap()) + ",";
  json += "\"uptime\":" + String(millis()/1000);
  
  json += "}";
  
  return json;
}

// 获取WiFi连接状态字符串
String getWiFiStatusString() {
  if (wifiConnected) {
    int clients = WiFi.softAPgetStationNum();
    return "WiFi: " + String(clients) + " clients";
  } else {
    return "WiFi: Not Started";
  }
}