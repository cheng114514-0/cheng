const int ledPin = 2;  // ESP32板载LED通常为GPIO2
unsigned long previousMillis = 0;
// 1Hz闪烁，周期1000ms，即每500ms翻转一次状态
const long interval = 500;
bool ledState = LOW;

void setup() {
  pinMode(ledPin, OUTPUT);
}

void loop() {
  unsigned long currentMillis = millis();
  // 达到设定间隔后翻转LED状态
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    ledState = !ledState;
    digitalWrite(ledPin, ledState);
  }
}
