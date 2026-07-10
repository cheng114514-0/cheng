#include <WiFi.h>
#include <WebServer.h>

// ========== AP 热点配置 ==========
const char* ap_ssid = "ESP32-LAB210";
const char* ap_pass = "12345678";

#define TOUCH_PIN 4   // 触摸检测引脚 GPIO4(T0)

WebServer server(80);

// ========== 根页面：带实时数值面板 + AJAX自动刷新 ==========
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 触摸传感器实时监控面板</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      margin: 0;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      min-height: 100vh;
      display: flex;
      align-items: center;
      justify-content: center;
    }
    .panel {
      background: white;
      padding: 60px 40px;
      border-radius: 24px;
      box-shadow: 0 10px 40px rgba(0,0,0,0.2);
      text-align: center;
      min-width: 400px;
    }
    h1 {
      color: #2d3748;
      margin-bottom: 40px;
      font-size: 28px;
    }
    .value-display {
      background: #1a202c;
      color: #48bb78;
      font-size: 88px;
      font-weight: bold;
      font-family: 'Courier New', monospace;
      padding: 30px;
      border-radius: 16px;
      margin-bottom: 30px;
    }
    .tips {
      color: #718096;
      font-size: 16px;
    }
  </style>
</head>
<body>
  <div class="panel">
    <h1>🖐️ 触摸传感器实时数值监控</h1>
    <div class="value-display" id="touchValue">--</div>
    <div class="tips">手靠近引脚数值变小，松开后数值回升</div>
  </div>

  <script>
    // 每100ms拉取一次最新数值，实现流畅跳动
    function fetchTouchData() {
      fetch('/getTouchValue')
        .then(res => res.text())
        .then(data => {
          document.getElementById('touchValue').innerText = data;
        })
    }
    // 页面加载就启动轮询
    setInterval(fetchTouchData, 100);
  </script>
</body>
</html>
)rawliteral";
  server.send(200, "text/html; charset=UTF-8", html);
}

// 新增API接口：直接返回当前触摸传感器的原始模拟量数值
void handleGetTouchValue() {
  int currentTouchVal = touchRead(TOUCH_PIN);
  server.send(200, "text/plain", String(currentTouchVal));
}

void setup() {
  Serial.begin(115200);

  // 开启AP热点
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_pass);
  
  Serial.println("触摸监控服务已启动");
  Serial.print("管理后台地址：http://");
  Serial.println(WiFi.softAPIP());

  // 注册路由
  server.on("/", handleRoot);
  server.on("/getTouchValue", handleGetTouchValue); // 新增数值上报路由
  server.onNotFound([](){
    server.send(404, "text/plain", "Not Found");
  });

  server.begin();
}

void loop() {
  server.handleClient();
}

