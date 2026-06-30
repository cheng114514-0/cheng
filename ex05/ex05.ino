// 定义引脚
#define TOUCH_PIN 4   // 触摸引脚 T0 (GPIO 4)
#define LED_PIN 2     // LED引脚 (GPIO 2，板载LED一般是GPIO2)

// 定义阈值，需根据实际串口监视器数值调整
// 通常未触摸时数值较大(如60+)，触摸时会变小(如20以下)
int threshold = 30;

// 状态变量
int currentGear = 1;        // 当前速度档位：1/2/3档循环
bool isTouching = false;      // 当前是否处于被触摸状态（用于边缘检测）
unsigned long lastDebounceTime = 0; // 上次状态改变的时间（用于防抖）
unsigned long debounceDelay = 300;  // 防抖延迟时间（触摸切换档位，防抖稍长一点更稳定）

// PWM呼吸灯变量
int ledValue = 0;          // 当前PWM占空比 (0-255)
int stepDirection = 1;     // 变化方向：1=变亮，-1=变暗

// 只替换setup()即可，其余代码和上面一致
void setup() {
  Serial.begin(115200);
  // 新版ESP32核心使用 ledcAttach 直接绑定引脚
  ledcAttach(LED_PIN, 5000, 8);
}


void loop() {
  // 1. 读取触摸值
  int touchValue = touchRead(TOUCH_PIN);

  // 打印数值以便调试（可选）
  // Serial.print("Touch Value: ");
  // Serial.println(touchValue);

  // 2. 判断当前物理状态 (触摸值小于阈值视为按下)
  bool currentTouchStatus = (touchValue < threshold);

  // 3. 边缘检测与防抖逻辑
  // 只有当“当前是按下”且“上一刻是松开”时（上升沿），才触发动作
  if (currentTouchStatus == true && isTouching == false) {
    // 简单的防抖检查：距离上次动作必须超过 debounceDelay 毫秒
    if (millis() - lastDebounceTime > debounceDelay) {
      // --- 执行切换档位动作：1→2→3→1循环 ---
      currentGear = currentGear + 1;
      if (currentGear > 3) {
        currentGear = 1;
      }
      // 更新防抖时间戳
      lastDebounceTime = millis();
      Serial.printf("当前切换到%d档\n", currentGear);
    }
  }

  // 4. 更新上一刻的状态记录
  // 注意：这里直接更新，但动作只在上面的 if 中触发
  isTouching = currentTouchStatus;

  // 5. 根据当前档位更新呼吸灯
  int step = 1;
  // 根据档位设置呼吸步长，档位越高步长越大，呼吸越快
  switch(currentGear) {
    case 1: step = 1; break;  // 1档：缓慢呼吸
    case 2: step = 3; break;  // 2档：中等速度
    case 3: step = 6; break;  // 3档：急促呼吸
  }

  // 更新占空比，改变亮度
  ledValue += stepDirection * step;
  if (ledValue >= 255) {
    ledValue = 255;
    stepDirection = -1;
  }
  if (ledValue <= 0) {
    ledValue = 0;
    stepDirection = 1;
  }

  // 输出PWM实现呼吸效果
  ledcWrite(0, ledValue);
  delay(10); // 短暂延时，降低CPU占用
}
