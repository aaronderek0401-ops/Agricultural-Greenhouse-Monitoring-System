// wifi_manager.ino - WiFi热点Web服务器通信管理
// Arduino Nano ESP32 WiFi热点，用于手机/电脑监控温室数据

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
// #include <ArduinoJson.h> // 暂时注释，使用String拼接JSON

// 外部变量声明 - 引用主文件中的阈值配置
extern struct Thresholds thresholds;

// WiFi热点配置
const char* ap_ssid = "ESP32_Greenhouse";
const char* ap_password = "12345678"; // 8位密码

// IP地址配置
IPAddress local_IP(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

// Web服务器和DNS服务器
WebServer server(80);
DNSServer dnsServer;
const byte DNS_PORT = 53;

bool wifiConnected = false;
unsigned long lastDataUpdate = 0;
const unsigned long DATA_UPDATE_INTERVAL = 1000; // 1秒更新一次数据缓存

// 传感器数据缓存
String cachedSensorData = "";

// WiFi热点初始化
void initWiFiHotspot() {
  Serial.println("Initializing WiFi Hotspot...");
  
  // 配置IP地址
  WiFi.softAPConfig(local_IP, gateway, subnet);
  
  // 配置WiFi热点
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_password);
  
  IPAddress IP = WiFi.softAPIP();
  Serial.print("Hotspot IP: ");
  Serial.println(IP);
  Serial.println("Hotspot Name: " + String(ap_ssid));
  Serial.println("Hotspot Password: " + String(ap_password));
  
  // 启动DNS服务器（强制门户）
  dnsServer.start(DNS_PORT, "*", local_IP);
  Serial.println("DNS Server started for Captive Portal");
  
  // 配置Web服务器路由
  setupWebRoutes();
  
  // 启动Web服务器
  server.begin();
  Serial.println("Web Server Started");
  Serial.println("Access URL: http://" + IP.toString());
  Serial.println("Connect to WiFi and your browser should automatically open the monitoring page!");
  
  wifiConnected = true;
}

// 检查是否为强制门户请求
bool isCaptivePortalRequest() {
  String host = server.hostHeader();
  // 如果请求的主机不是我们的IP地址，就重定向到监控页面
  return (host != local_IP.toString());
}

// 强制门户重定向处理
void handleCaptivePortal() {
  if (isCaptivePortalRequest()) {
    // 重定向到监控页面
    String redirectURL = "http://" + local_IP.toString() + "/";
    server.sendHeader("Location", redirectURL);
    server.send(302, "text/plain", "Redirecting to monitoring page...");
  } else {
    // 直接显示监控页面
    handleRoot();
  }
}

// 配置Web服务器路由
void setupWebRoutes() {
  // 主页 - 显示监控界面（支持强制门户）
  server.on("/", HTTP_GET, handleCaptivePortal);
  
  // API - 获取传感器数据(JSON格式)
  server.on("/api/data", HTTP_GET, handleAPIData);
  
  // API - 获取系统状态
  server.on("/api/status", HTTP_GET, handleAPIStatus);
  
  // API - 获取阈值设置
  server.on("/api/thresholds", HTTP_GET, handleGetThresholds);
  
  // API - 设置阈值
  server.on("/api/thresholds", HTTP_POST, handleSetThresholds);
  
  // 处理CORS预检请求
  server.on("/api/thresholds", HTTP_OPTIONS, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
    server.send(200, "text/plain", "");
  });
  
  // API - 重启系统
  server.on("/api/restart", HTTP_POST, handleAPIRestart);
  
  // 强制门户：捕获所有其他请求并重定向
  server.onNotFound([](){
    if (isCaptivePortalRequest()) {
      String redirectURL = "http://" + local_IP.toString() + "/";
      server.sendHeader("Location", redirectURL);
      server.send(302, "text/plain", "Redirecting to monitoring page...");
    } else {
      handleNotFound();
    }
  });
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

// 获取阈值设置API
void handleGetThresholds() {
  String json = "{";
  json += "\"temperature\":{\"min\":" + String(thresholds.tempMin) + ",\"max\":" + String(thresholds.tempMax) + "},";
  json += "\"humidity\":{\"min\":" + String(thresholds.humidityMin) + ",\"max\":" + String(thresholds.humidityMax) + "},";
  json += "\"co2\":{\"min\":" + String(thresholds.co2Min) + ",\"max\":" + String(thresholds.co2Max) + "},";
  json += "\"pressure\":{\"min\":" + String(thresholds.pressureMin) + ",\"max\":" + String(thresholds.pressureMax) + "},";
  json += "\"light\":{\"min\":" + String(thresholds.lightMin) + ",\"max\":" + String(thresholds.lightMax) + "}";
  json += "}";
  
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

// 设置阈值API
void handleSetThresholds() {
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed");
    return;
  }
  
  String body = server.arg("plain");
  Serial.println("Received threshold update: " + body);
  
  // 简单的JSON解析（手动解析避免引入额外库）
  if (body.indexOf("tempMin") != -1) {
    int start = body.indexOf("\"tempMin\":") + 10;
    int end = body.indexOf(",", start);
    if (end == -1) end = body.indexOf("}", start);
    thresholds.tempMin = body.substring(start, end).toFloat();
  }
  
  if (body.indexOf("tempMax") != -1) {
    int start = body.indexOf("\"tempMax\":") + 10;
    int end = body.indexOf(",", start);
    if (end == -1) end = body.indexOf("}", start);
    thresholds.tempMax = body.substring(start, end).toFloat();
  }
  
  if (body.indexOf("humidityMin") != -1) {
    int start = body.indexOf("\"humidityMin\":") + 14;
    int end = body.indexOf(",", start);
    if (end == -1) end = body.indexOf("}", start);
    thresholds.humidityMin = body.substring(start, end).toFloat();
  }
  
  if (body.indexOf("humidityMax") != -1) {
    int start = body.indexOf("\"humidityMax\":") + 14;
    int end = body.indexOf(",", start);
    if (end == -1) end = body.indexOf("}", start);
    thresholds.humidityMax = body.substring(start, end).toFloat();
  }
  
  if (body.indexOf("co2Min") != -1) {
    int start = body.indexOf("\"co2Min\":") + 9;
    int end = body.indexOf(",", start);
    if (end == -1) end = body.indexOf("}", start);
    thresholds.co2Min = body.substring(start, end).toInt();
  }
  
  if (body.indexOf("co2Max") != -1) {
    int start = body.indexOf("\"co2Max\":") + 9;
    int end = body.indexOf(",", start);
    if (end == -1) end = body.indexOf("}", start);
    thresholds.co2Max = body.substring(start, end).toInt();
  }
  
  if (body.indexOf("pressureMin") != -1) {
    int start = body.indexOf("\"pressureMin\":") + 14;
    int end = body.indexOf(",", start);
    if (end == -1) end = body.indexOf("}", start);
    thresholds.pressureMin = body.substring(start, end).toFloat();
  }
  
  if (body.indexOf("pressureMax") != -1) {
    int start = body.indexOf("\"pressureMax\":") + 14;
    int end = body.indexOf(",", start);
    if (end == -1) end = body.indexOf("}", start);
    thresholds.pressureMax = body.substring(start, end).toFloat();
  }
  
  if (body.indexOf("lightMin") != -1) {
    int start = body.indexOf("\"lightMin\":") + 11;
    int end = body.indexOf(",", start);
    if (end == -1) end = body.indexOf("}", start);
    thresholds.lightMin = body.substring(start, end).toInt();
  }
  
  if (body.indexOf("lightMax") != -1) {
    int start = body.indexOf("\"lightMax\":") + 11;
    int end = body.indexOf(",", start);
    if (end == -1) end = body.indexOf("}", start);
    thresholds.lightMax = body.substring(start, end).toInt();
  }
  
  Serial.println("Updated thresholds:");
  Serial.println("Temperature: " + String(thresholds.tempMin) + "-" + String(thresholds.tempMax));
  Serial.println("Humidity: " + String(thresholds.humidityMin) + "-" + String(thresholds.humidityMax));
  Serial.println("CO2: " + String(thresholds.co2Min) + "-" + String(thresholds.co2Max));
  Serial.println("Pressure: " + String(thresholds.pressureMin) + "-" + String(thresholds.pressureMax));
  Serial.println("Light: " + String(thresholds.lightMin) + "-" + String(thresholds.lightMax));
  
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"Thresholds updated successfully\"}");
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
  html += ".btn.secondary { background: #95a5a6; }";
  html += ".btn.secondary:hover { background: #7f8c8d; }";
  html += ".refresh-info { text-align: center; color: #7f8c8d; margin-top: 10px; }";
  html += ".threshold-panel { background: #f8f9fa; padding: 15px; margin: 15px 0; border-radius: 8px; display: none; }";
  html += ".threshold-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(220px, 1fr)); gap: 12px; }";
  html += ".threshold-item { background: white; padding: 12px; border-radius: 5px; border: 1px solid #ddd; }";
  html += ".threshold-item h4 { margin: 0 0 10px 0; color: #2c3e50; }";
  html += ".threshold-inputs { display: flex; gap: 8px; align-items: center; }";
  html += ".threshold-inputs input { width: 80px; padding: 6px; border: 1px solid #ddd; border-radius: 3px; font-size: 14px; }";
  html += ".threshold-inputs span { color: #7f8c8d; font-size: 14px; min-width: 20px; text-align: center; }";
  html += ".preset-section { margin: 15px 0; padding: 15px; background: #e8f5e8; border-radius: 5px; border-left: 4px solid #27ae60; }";
  html += ".preset-section h5 { margin: 0 0 10px 0; color: #27ae60; font-size: 16px; }";
  html += ".preset-buttons { display: flex; flex-wrap: wrap; gap: 8px; justify-content: center; }";
  html += ".btn.preset { background: #27ae60; font-size: 13px; padding: 8px 15px; }";
  html += ".btn.preset:hover { background: #219a52; }";
  html += "</style></head><body>";
  
  html += "<div class='container'>";
  html += "<div class='header'>";
  html += "<h1>🌱 ESP32 Greenhouse Monitor</h1>";
  html += "<p>Real-time Sensor Data Monitoring</p>";
  html += "<p style='color: #27ae60; font-size: 14px;'>✅ Auto-opened via Captive Portal</p>";
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
  html += "<button class='btn secondary' onclick='toggleThresholds()'>Settings</button>";
  html += "</div>";
  
  // 阈值设置面板
  html += "<div id='thresholdPanel' class='threshold-panel'>";
  html += "<h3 style='text-align: center; color: #2c3e50; margin-bottom: 20px;'>🔧 Sensor Thresholds</h3>";
  html += "<div class='threshold-grid'>";
  
  html += "<div class='threshold-item'>";
  html += "<h4>🌡️ Temperature (°C)</h4>";
  html += "<div class='threshold-inputs'>";
  html += "<input type='number' id='tempMin' step='0.1' placeholder='Min'>";
  html += "<span>to</span>";
  html += "<input type='number' id='tempMax' step='0.1' placeholder='Max'>";
  html += "</div></div>";
  
  html += "<div class='threshold-item'>";
  html += "<h4>💧 Humidity (%)</h4>";
  html += "<div class='threshold-inputs'>";
  html += "<input type='number' id='humidityMin' step='0.1' placeholder='Min'>";
  html += "<span>to</span>";
  html += "<input type='number' id='humidityMax' step='0.1' placeholder='Max'>";
  html += "</div></div>";
  
  html += "<div class='threshold-item'>";
  html += "<h4>🌬️ CO2 (ppm)</h4>";
  html += "<div class='threshold-inputs'>";
  html += "<input type='number' id='co2Min' placeholder='Min'>";
  html += "<span>to</span>";
  html += "<input type='number' id='co2Max' placeholder='Max'>";
  html += "</div></div>";
  
  html += "<div class='threshold-item'>";
  html += "<h4>🍐 Pressure (hPa)</h4>";
  html += "<div class='threshold-inputs'>";
  html += "<input type='number' id='pressureMin' step='0.1' placeholder='Min'>";
  html += "<span>to</span>";
  html += "<input type='number' id='pressureMax' step='0.1' placeholder='Max'>";
  html += "</div></div>";
  
  html += "<div class='threshold-item'>";
  html += "<h4>☀️ Light (lux)</h4>";
  html += "<div class='threshold-inputs'>";
  html += "<input type='number' id='lightMin' placeholder='Min'>";
  html += "<span>to</span>";
  html += "<input type='number' id='lightMax' placeholder='Max'>";
  html += "</div></div>";
  
  html += "</div>";
  
  // 预设设置部分
  html += "<div class='preset-section'>";
  html += "<h5>🌟 Quick Presets</h5>";
  html += "<div class='preset-buttons'>";
  html += "<button class='btn preset' onclick='applyPreset(\"tomato\")'>🍅 Tomato</button>";
  html += "<button class='btn preset' onclick='applyPreset(\"cucumber\")'>🥒 Cucumber</button>";
  html += "<button class='btn preset' onclick='applyPreset(\"lettuce\")'>🥬 Lettuce</button>";
  html += "<button class='btn preset' onclick='applyPreset(\"pepper\")'>🌶️ Pepper</button>";
  html += "<button class='btn preset' onclick='applyPreset(\"herb\")'>🌿 Herbs</button>";
  html += "</div>";
  html += "</div>";
  
  html += "<div style='text-align: center; margin-top: 20px;'>";
  html += "<button class='btn' onclick='saveThresholds()'>Save Settings</button>";
  html += "<button class='btn secondary' onclick='loadThresholds()'>Reset to Current</button>";
  html += "</div>";
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
  
  // 阈值管理函数
  html += "function toggleThresholds() {";
  html += "var panel = document.getElementById('thresholdPanel');";
  html += "if (panel.style.display === 'none' || panel.style.display === '') {";
  html += "panel.style.display = 'block';";
  html += "loadThresholds();";
  html += "} else {";
  html += "panel.style.display = 'none';";
  html += "}";
  html += "}";
  
  html += "function loadThresholds() {";
  html += "fetch('/api/thresholds').then(function(response) { return response.json(); }).then(function(data) {";
  html += "document.getElementById('tempMin').value = data.temperature.min;";
  html += "document.getElementById('tempMax').value = data.temperature.max;";
  html += "document.getElementById('humidityMin').value = data.humidity.min;";
  html += "document.getElementById('humidityMax').value = data.humidity.max;";
  html += "document.getElementById('co2Min').value = data.co2.min;";
  html += "document.getElementById('co2Max').value = data.co2.max;";
  html += "document.getElementById('pressureMin').value = data.pressure.min;";
  html += "document.getElementById('pressureMax').value = data.pressure.max;";
  html += "document.getElementById('lightMin').value = data.light.min;";
  html += "document.getElementById('lightMax').value = data.light.max;";
  html += "}).catch(function(error) { console.error('Failed to load thresholds:', error); });";
  html += "}";
  
  html += "function saveThresholds() {";
  html += "var thresholds = {";
  html += "tempMin: parseFloat(document.getElementById('tempMin').value),";
  html += "tempMax: parseFloat(document.getElementById('tempMax').value),";
  html += "humidityMin: parseFloat(document.getElementById('humidityMin').value),";
  html += "humidityMax: parseFloat(document.getElementById('humidityMax').value),";
  html += "co2Min: parseInt(document.getElementById('co2Min').value),";
  html += "co2Max: parseInt(document.getElementById('co2Max').value),";
  html += "pressureMin: parseFloat(document.getElementById('pressureMin').value),";
  html += "pressureMax: parseFloat(document.getElementById('pressureMax').value),";
  html += "lightMin: parseInt(document.getElementById('lightMin').value),";
  html += "lightMax: parseInt(document.getElementById('lightMax').value)";
  html += "};";
  
  html += "fetch('/api/thresholds', {";
  html += "method: 'POST',";
  html += "headers: { 'Content-Type': 'application/json' },";
  html += "body: JSON.stringify(thresholds)";
  html += "}).then(function(response) { return response.json(); }).then(function(data) {";
  html += "if (data.status === 'success') {";
  html += "alert('Thresholds saved successfully!');";
  html += "document.getElementById('thresholdPanel').style.display = 'none';";
  html += "} else {";
  html += "alert('Failed to save thresholds: ' + data.message);";
  html += "}";
  html += "}).catch(function(error) {";
  html += "console.error('Failed to save thresholds:', error);";
  html += "alert('Error saving thresholds');";
  html += "});";
  html += "}";
  
  // 预设设置函数
  html += "function applyPreset(type) {";
  html += "var presets = {";
  html += "tomato: { tempMin: 18, tempMax: 26, humidityMin: 60, humidityMax: 80, co2Min: 400, co2Max: 1000, pressureMin: 1000, pressureMax: 1030, lightMin: 10000, lightMax: 50000 },";
  html += "cucumber: { tempMin: 20, tempMax: 28, humidityMin: 70, humidityMax: 85, co2Min: 400, co2Max: 1200, pressureMin: 1000, pressureMax: 1030, lightMin: 8000, lightMax: 40000 },";
  html += "lettuce: { tempMin: 15, tempMax: 22, humidityMin: 50, humidityMax: 70, co2Min: 300, co2Max: 800, pressureMin: 1000, pressureMax: 1030, lightMin: 5000, lightMax: 25000 },";
  html += "pepper: { tempMin: 22, tempMax: 30, humidityMin: 55, humidityMax: 75, co2Min: 400, co2Max: 1000, pressureMin: 1000, pressureMax: 1030, lightMin: 12000, lightMax: 60000 },";
  html += "herb: { tempMin: 16, tempMax: 24, humidityMin: 40, humidityMax: 60, co2Min: 300, co2Max: 800, pressureMin: 1000, pressureMax: 1030, lightMin: 6000, lightMax: 30000 }";
  html += "};";
  
  html += "if (presets[type]) {";
  html += "var preset = presets[type];";
  html += "document.getElementById('tempMin').value = preset.tempMin;";
  html += "document.getElementById('tempMax').value = preset.tempMax;";
  html += "document.getElementById('humidityMin').value = preset.humidityMin;";
  html += "document.getElementById('humidityMax').value = preset.humidityMax;";
  html += "document.getElementById('co2Min').value = preset.co2Min;";
  html += "document.getElementById('co2Max').value = preset.co2Max;";
  html += "document.getElementById('pressureMin').value = preset.pressureMin;";
  html += "document.getElementById('pressureMax').value = preset.pressureMax;";
  html += "document.getElementById('lightMin').value = preset.lightMin;";
  html += "document.getElementById('lightMax').value = preset.lightMax;";
  html += "alert('Applied ' + type + ' preset settings!');";
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
  // 处理DNS服务器请求（强制门户）
  dnsServer.processNextRequest();
  
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