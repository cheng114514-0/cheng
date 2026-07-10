#include <WiFi.h>
#include <WebServer.h>

// ========== AP 热点配置（和你之前的项目保持一致） ==========
const char* ap_ssid = "ESP32-LAB210";
const char* ap_pass = "12345678";  // 热点密码

// ========== 硬件引脚配置 ==========
#define ALARM_LED_PIN 2       // 报警指示灯接GPIO2
#define TOUCH_PIN 4           // 触摸检测引脚接GPIO4 (T0)

// ========== 全局状态标记 ==========
bool systemArmed = false;      // 系统布防状态：默认撤防
bool alarmTriggered = false;   // 报警触发标记：默认未报警

// ========== 计时变量（用非阻塞millis()实现LED狂闪） ==========
unsigned long lastFlashTime = 0;
const int flashInterval = 80;  // 报警时LED每80ms翻转一次，实现高频狂闪

WebServer server(80);

// ========== 网页端：布防/撤防控制页面 ==========
void handleRoot() {
  String currentStatusText = systemArmed ? "当前状态：⚠️ 已布防" : "当前状态：✅ 已撤防";
  if (alarmTriggered) currentStatusText = "🚨 警报触发！系统锁定报警中";

  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 安防报警主机</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      text-align: center;
      margin-top: 70px;
      background: #f5f5f5;
    }
    h1 { color: #2c3e50; }
    .status-card {
      background: white;
      padding: 40px 30px;
      border-radius: 12px;
      box-shadow: 0 2px 12px rgba(0,0,0,0.1);
      display: inline-block;
      min-width: 340px;
    }
    .status-box {
      font-size: 22px;
      font-weight: bold;
      padding: 20px;
      border-radius: 8px;
      margin-bottom: 30px;
      background: #e8f5e9;
      color: #2e7d32;
    }
    .alarm-active {
      background: #ffebee !important;
      color: #c62828 !important;
      animation: blink 1s infinite;
    }
    @keyframes blink { 50% {opacity: 0.7;} }
    .btn-group { display: flex; gap: 20px; justify-content: center; }
    button {
      padding: 18px 36px;
      border: none;
      border-radius: 8px;
      font-size: 18px;
      font-weight: bold;
      cursor: pointer;
      color: white;
      transition: 0.2s;
    }
    #armBtn { background: #2196F3; }
    #disarmBtn { background: #f44336; }
    button:active { transform: scale(0.95); }
  </style>
</head>
<body>
  <div class="status-card">
    <h1>🏠 智能安防主机</h1>
    <div class="status-box" id="statusBox">%STATUS%</div>
    <div class="btn-group">
      <button id="armBtn">布防 (Arm)</button>
      <button id="disarmBtn">撤防 (Disarm)</button>
    </div>
  </div>

  <script>
    const statusBox = document.getElementById('statusBox');
    // 布防请求
    document.getElementById('armBtn').onclick = () => {
      fetch('/arm').then(res => res.text()).then(msg => {
        statusBox.innerText = msg;
        statusBox.className = 'status-box';
      })
    };
    // 撤防请求
    document.getElementById('disarmBtn').onclick = () => {
      fetch('/disarm').then(res => res.text()).then(msg => {
        statusBox.innerText = msg;
        statusBox.className = 'status-box';
        if(msg.includes("警报触发")) statusBox.classList.add("alarm-active");
      })
    };
  </script>
</body>
</html>
)rawliteral";

  // 网页内容里的状态占位符动态替换
  html.replace("%STATUS%", currentStatusText);
  server.send(200, "text/html; charset=UTF-8", html);
}

// ========== 布防接口 ==========
void handleArm() {
  systemArmed = true;
  Serial.println("✅ 系统已进入布防状态");
  server.send(200, "text/plain", "当前状态：⚠️ 已布防");
}

// ========== 撤防接口：同时重置所有报警状态 ==========
void handleDisarm() {
  systemArmed = false;
  alarmTriggered = false;
  digitalWrite(ALARM_LED_PIN, LOW); // 撤防立刻熄灭LED
  Serial.println("✅ 系统已撤防，报警状态重置");
  server.send(200, "text/plain", "当前状态：✅ 已撤防");
}

void handleNotFound() {
  server.send(404, "text/plain", "Not Found");
}

void setup() {
  Serial.begin(115200);
  
  // 初始化LED引脚为输出，初始熄灭
  pinMode(ALARM_LED_PIN, OUTPUT);
  digitalWrite(ALARM_LED_PIN, LOW);

  // 开启AP热点模式
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_pass);
  
  Serial.println("安防主机AP热点已开启");
  Serial.print("热点名称: ");
  Serial.println(ap_ssid);
  Serial.print("管理后台地址: http://");
  Serial.println(WiFi.softAPIP());

  // 注册Web路由
  server.on("/", handleRoot);
  server.on("/arm", handleArm);
  server.on("/disarm", handleDisarm);
  server.onNotFound(handleNotFound);
  
  server.begin();
  Serial.println("Web服务启动完成");
}

void loop() {
  server.handleClient();

  // 核心安防逻辑
  unsigned long now = millis();

  // 仅当系统处于【已布防且未触发报警】状态时，才检测触摸
  if (systemArmed && !alarmTriggered) {
    // ESP32触摸检测：触摸值小于阈值40判定为有人触碰
    if (touchRead(TOUCH_PIN) < 40) {
      alarmTriggered = true;
      Serial.println("🚨 检测到入侵！报警已触发并锁定");
    }
  }

  // 一旦报警触发，LED进入高频狂闪，直到撤防才会停止
  if (alarmTriggered) {
    if (now - lastFlashTime >= flashInterval) {
      lastFlashTime = now;
      digitalWrite(ALARM_LED_PIN, !digitalRead(ALARM_LED_PIN));
    }
  }
}
