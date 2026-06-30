// 定义两个LED连接的GPIO
const int ledPinA = 2; 
const int ledPinB = 4; 

// PWM参数配置
const int freq = 5000;      // 5kHz频率，人眼无频闪
const int resolution = 8;   // 8位分辨率，占空比范围0~255

void setup() {
  Serial.begin(115200);
  // ESP32 3.x新版API直接绑定引脚，无需手动设置通道
  ledcAttach(ledPinA, freq, resolution);
  ledcAttach(ledPinB, freq, resolution);
}

void loop() {
  // 阶段1：A渐亮 / B渐暗
  for (int dutyCycle = 0; dutyCycle <= 255; dutyCycle++) {
    ledcWrite(ledPinA, dutyCycle);
    ledcWrite(ledPinB, 255 - dutyCycle);
    delay(5);
  }

  // 阶段2：A渐暗 / B渐亮
  for (int dutyCycle = 255; dutyCycle >= 0; dutyCycle--) {
    ledcWrite(ledPinA, dutyCycle);
    ledcWrite(ledPinB, 255 - dutyCycle);
    delay(5);
  }
}


