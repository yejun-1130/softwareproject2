// ===== LED on GPIO 7: 1초 켜기 → 1초 동안 5회 깜빡임 → 꺼지고 정지 =====
#define PIN_LED 7

// ★Active-Low 결선일 경우 true (핀 LOW → LED ON)
const bool LED_ACTIVE_LOW = true;

// 켜짐/꺼짐 레벨
const int LED_ON  = LED_ACTIVE_LOW ? LOW  : HIGH;
const int LED_OFF = LED_ACTIVE_LOW ? HIGH : LOW;

const unsigned long HOLD_ON_MS      = 1000; // 처음 켜두는 시간
const unsigned long BLINK_WINDOW_MS = 1000; // 깜빡임 총 시간
const int           BLINK_COUNT     = 5;    // 깜빡임 횟수

void setup() {
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LED_OFF); // 시작은 꺼짐
}

void loop() {
  // 1) 처음 1초 동안 LED 켜기
  digitalWrite(PIN_LED, LED_ON);
  delay(HOLD_ON_MS);

  // 2) 다음 1초 동안 5회 깜빡이기
  unsigned long halfPeriod = BLINK_WINDOW_MS / (BLINK_COUNT * 2UL);
  for (int i = 0; i < BLINK_COUNT; i++) {
    digitalWrite(PIN_LED, LED_ON);  delay(halfPeriod);
    digitalWrite(PIN_LED, LED_OFF); delay(halfPeriod);
  }

  // 3) LED 끄기
  digitalWrite(PIN_LED, LED_OFF);

  // 4) 무한 정지
  while (true) { /* do nothing */ }
}

