#include <WiFi.h>
#include <WebServer.h>

// ========== AP 热点配置（完全保留你的原有配置） ==========
const char* ap_ssid = "ESP32-LAB210";
const char* ap_pass = "12345678";  // 至少8位

// ========== LED 与 PWM 配置 ==========
#define LED_PIN 2               // LED连接D2 (GPIO2)
const int pwmFreq = 5000;       // PWM频率 5kHz
const int pwmResolution = 8;    // 8位分辨率 (0-255)

// ========== Web服务器 ==========
WebServer server(80);           // 监听80端口
int currentBrightness = 0;

// ========== 处理根路径：滑动调光网页 ==========
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 滑动无极调光器</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      text-align: center;
      margin-top: 60px;
      background: #f5f5f5;
    }
    h1 { color: #333; }
    .container {
      background: white;
      padding: 40px 30px;
      border-radius: 12px;
      box-shadow: 0 2px 10px rgba(0,0,0,0.1);
      display: inline-block;
      min-width: 320px;
    }
    /* 滑动条专属美化 */
    input[type=range] {
      width: 90%;
      height: 14px;
      border-radius: 7px;
      outline: none;
      -webkit-appearance: none;
      background: linear-gradient(to right, #2196F3 0%, #ddd 0%);
    }
    input[type=range]::-webkit-slider-thumb {
      -webkit-appearance: none;
      width: 28px;
      height: 28px;
      border-radius: 50%;
      background: #2196F3;
      cursor: pointer;
      transition: 0.2s;
    }
    .value-display {
      font-size: 48px;
      font-weight: bold;
      color: #2196F3;
      margin: 25px 0;
    }
    .status {
      color: #666;
      font-size: 14px;
      margin-top: 20px;
      min-height: 20px;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>LED亮度滑动调节</h1>
    <p>拖动滑动条实时调整亮度 0~255</p >
    
    <div class="value-display" id="valBox">0</div>
    <input type="range" min="0" max="255" value="0" id="brightnessSlider">
    
    <div class="status" id="statusText">等待拖动滑动条...</div>
  </div>

  <script>
    const slider = document.getElementById('brightnessSlider');
    const valBox = document.getElementById('valBox');
    const statusText = document.getElementById('statusText');
    
    // 防抖机制：避免拖动时频繁发送HTTP请求，提升流畅度
    let debounceTimer;
    slider.addEventListener('input', () => {
      const val = parseInt(slider.value);
      valBox.innerText = val;
      // 滑动条背景实时跟随进度变色
      slider.style.background = `linear-gradient(to right, #2196F3 ${val/255*100}%, #ddd ${val/255*100}%)`;
      
      clearTimeout(debounceTimer);
      statusText.innerText = "同步亮度中...";
      debounceTimer = setTimeout(() => {
        fetch(`/set?val=${val}`)
          .then(res => {
            if (res.ok) {
              statusText.innerText = `亮度已同步: ${val}`;
              statusText.style.color = '#4CAF50';
            } else throw new Error("请求失败");
          })
          .catch(() => {
            statusText.innerText = "同步出错，请重试";
            statusText.style.color = '#f44336';
          })
      }, 60); // 60ms防抖，兼顾实时性和稳定性
    });
  </script>
</body>
</html>
)rawliteral";

  server.send(200, "text/html; charset=UTF-8", html);
}

// ========== 原有的亮度设置接口完全兼容保留 ==========
void handleSet() {
  if (server.hasArg("val")) {
    String valStr = server.arg("val");
    int brightness = valStr.toInt();
    
    brightness = constrain(brightness, 0, 255);
    currentBrightness = brightness;
    
    ledcWrite(LED_PIN, brightness);
    
    Serial.print("收到亮度值: ");
    Serial.println(brightness);
    
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Missing parameter");
  }
}

// ========== 404处理 ==========
void handleNotFound() {
  server.send(404, "text/plain", "Not Found");
}

void setup() {
  Serial.begin(115200);
  
  // 初始化 PWM
  ledcAttach(LED_PIN, pwmFreq, pwmResolution);
  ledcWrite(LED_PIN, 0);  // 初始熄灭
  
  // 开启 AP 热点模式
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_pass);
  
  Serial.println("AP热点已开启");
  Serial.print("热点名称: ");
  Serial.println(ap_ssid);
  Serial.print("访问地址: http://");
  Serial.println(WiFi.softAPIP());  // 默认192.168.4.1
  
  // 注册路由
  server.on("/", handleRoot);      
  server.on("/set", handleSet);    
  server.onNotFound(handleNotFound);
  
  server.begin();
  Serial.println("Web服务器已启动");
}

void loop() {
  server.handleClient();
}
