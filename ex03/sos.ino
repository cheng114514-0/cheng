const int ledPin = 2;  // ESP32板载LED引脚

// 定义各阶段时间(单位：ms)
const int shortBlink = 300;   // 短闪点亮时长
const int longBlink = 900;    // 长闪点亮时长
const int gap = 300;          // 同组内闪之间的间隔
const int endPause = 3000;    // 一轮SOS结束后的停顿时间

// SOS状态序列，true代表点亮，false代表熄灭/间隔
bool sequence[] = {
  // 三次短闪
  1,0, 1,0, 1,0,
  // 间隔
  0,
  // 三次长闪
  1,0, 1,0, 1,0,
  // 间隔
  0,
  // 三次短闪
  1,0, 1,0, 1,0,
  // 结束停顿
  0
};

int currentStep = 0;          // 当前执行到序列的第几步
unsigned long previousMillis = 0;
unsigned long stepDuration = 0; // 当前步骤的持续时间

void setup() {
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
}

void loop() {
  unsigned long currentMillis = millis();
  
  // 当前步骤时间到，进入下一步
  if (currentMillis - previousMillis >= stepDuration) {
    previousMillis = currentMillis;
    
    // 根据当前状态设置LED
    digitalWrite(ledPin, sequence[currentStep]);
    
    // 根据当前步骤类型设置下一步的时长
    if (sequence[currentStep]) {
      // 点亮状态根据所属阶段设置长短
      stepDuration = (currentStep < 6 || (currentStep > 11 && currentStep < 18)) ? shortBlink : longBlink;
    } else {
      // 熄灭状态：如果是最后一步，为结束停顿，否则是普通间隔
      stepDuration = (currentStep == sizeof(sequence)/sizeof(sequence[0]) - 1) ? endPause : gap;
    }
    
    // 移动到下一步，到序列末尾后从头开始循环
    currentStep++;
    if (currentStep >= sizeof(sequence)/sizeof(sequence[0])) {
      currentStep = 0;
    }
  }
}
