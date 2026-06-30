/*
  项目：ESP32触摸自锁开关（点触切换LED状态）
  硬件需求：ESP32开发板 + 内置触摸引脚 + 板载LED/外接LED
  功能：第一次触摸点亮，第二次触摸熄灭，带软件防抖防止误触发
*/

// 引脚定义
const int touchPin = T0;  // 使用触摸引脚0（可根据你的型号修改为T1~T9）
const int ledPin = 2;     // 绝大多数ESP32开发板板载LED为GPIO2

// 状态变量定义
bool ledState = LOW;             // LED当前状态，初始熄灭
bool lastTouchState = LOW;       // 上一次触摸的状态，LOW=未触摸 HIGH=触摸
unsigned long lastDebounceTime = 0;  // 防抖计时变量
const unsigned long debounceDelay = 50;  // 防抖延时，50ms可根据实际情况调整

// 触摸阈值：ESP32触摸传感器默认超过阈值判定为触摸，可根据你的环境调整
const int touchThreshold = 40;

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, ledState);  // 初始状态熄灭LED
}

void loop() {
  // 1. 读取当前触摸值
  int currentTouchValue = touchRead(touchPin);
  // 转换为布尔状态：小于阈值判定为触摸（触摸时电容变化，读数降低）
  bool currentTouchState = (currentTouchValue < touchThreshold) ? HIGH : LOW;

  // 2. 软件防抖：如果触摸状态发生变化，重置计时
  if (currentTouchState != lastTouchState) {
    lastDebounceTime = millis();
  }

  // 3. 状态稳定后，判断是否有有效触摸
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // 检测到触摸的**上升沿（从无到有的瞬间）**
    if (currentTouchState == HIGH && lastTouchState == LOW) {
      // 翻转LED状态
      ledState = !ledState;
      digitalWrite(ledPin, ledState);
      // 串口打印调试信息
      Serial.printf("LED状态已翻转，当前状态：%s\n", ledState ? "点亮" : "熄灭");
    }
  }

  // 保存当前状态，供下一次循环对比
  lastTouchState = currentTouchState;
}
