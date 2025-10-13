#include <Servo.h>

// ---------- Pin assignment ----------
#define PIN_LED   9    // LED active-low (digitalWrite만 사용)
#define PIN_TRIG  12   // sonar TRIGGER
#define PIN_ECHO  13   // sonar ECHO
#define PIN_SERVO 10   // servo (Timer1)

// ---------- USS parameters ----------
#define SND_VEL       346.0    // m/s @ 24℃
#define INTERVAL      25       // ms
#define PULSE_DURATION 10      // us
#define _DIST_MIN     180.0    // mm
#define _DIST_MAX     360.0    // mm

#define TIMEOUT  ((INTERVAL / 2) * 1000.0) // us
#define SCALE    (0.001 * 0.5 * SND_VEL)   // mm/us

#define _EMA_ALPHA 0.3  // 0~1

// ---------- Target window ----------
#define _TARGET_LOW   250.0  // mm
#define _TARGET_HIGH  290.0  // mm

// ---------- Servo duty (us) ----------
#define _DUTY_MIN 100
#define _DUTY_NEU 1500
#define _DUTY_MAX 5000

// ---------- Globals ----------
float dist_prev = _DIST_MIN;   // 직전 유효값
float dist_ema  = _DIST_MIN;   // EMA 초기값
unsigned long last_sampling_time;

Servo myservo;

// ---------- Helpers (전방선언 + 단순구현) ----------
float clampf(float x, float a, float b);
float lerp(float a, float b, float t);
float USS_measure(int TRIG, int ECHO);

void setup() {
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  digitalWrite(PIN_TRIG, LOW);

  myservo.attach(PIN_SERVO);
  myservo.writeMicroseconds(_DUTY_NEU);

  Serial.begin(57600);
  last_sampling_time = millis();
}

void loop() {
  // 주기 제어 (오버플로우 안전)
  if ((unsigned long)(millis() - last_sampling_time) < (unsigned long)INTERVAL) return;
  last_sampling_time = millis();

  // 1) 거리 측정
  float dist_raw = USS_measure(PIN_TRIG, PIN_ECHO);

  // 2) 범위 필터: 범위 밖(또는 0) → 직전값 유지
  float dist_filtered;
  if ((dist_raw == 0.0f) || (dist_raw < _DIST_MIN) || (dist_raw > _DIST_MAX)) {
    dist_filtered = dist_prev;
  } else {
    dist_filtered = dist_raw;
    dist_prev     = dist_raw;
  }

  // 3) EMA (← line58 자리)
  dist_ema = (_EMA_ALPHA * dist_filtered) + ((1.0f - _EMA_ALPHA) * dist_ema);

  // 4) 서보 제어: 목표창 기준 선형 맵핑
  int duty_us;
  if (dist_ema < _TARGET_LOW) {
    float span = (_TARGET_LOW - _DIST_MIN);
    float k = (span > 0.0f) ? clampf((_TARGET_LOW - dist_ema) / span, 0.0f, 1.0f) : 1.0f;
    duty_us = (int)(lerp((float)_DUTY_NEU, (float)_DUTY_MIN, k) + 0.5f);
  } else if (dist_ema > _TARGET_HIGH) {
    float span = (_DIST_MAX - _TARGET_HIGH);
    float k = (span > 0.0f) ? clampf((dist_ema - _TARGET_HIGH) / span, 0.0f, 1.0f) : 1.0f;
    duty_us = (int)(lerp((float)_DUTY_NEU, (float)_DUTY_MAX, k) + 0.5f);
  } else {
    duty_us = _DUTY_NEU;
  }
  myservo.writeMicroseconds(duty_us);

  // 5) LED(active-low): 창 안 → 켜짐(LOW)
  if (dist_ema >= _TARGET_LOW && dist_ema <= _TARGET_HIGH) {
    digitalWrite(PIN_LED, LOW);
  } else {
    digitalWrite(PIN_LED, HIGH);
  }

  // 6) 로그 (임시변수 사용해 매크로/오버로드 충돌 회피)
  float raw_shown = dist_raw;
  if (raw_shown > (_DIST_MAX + 100.0f)) raw_shown = _DIST_MAX + 100.0f;

  Serial.print("Min:");    Serial.print(_DIST_MIN);
  Serial.print(",Low:");   Serial.print(_TARGET_LOW);
  Serial.print(",raw:");   Serial.print(raw_shown);
  Serial.print(",filt:");  Serial.print(dist_filtered);
  Serial.print(",ema:");   Serial.print(dist_ema);
  Serial.print(",duty(us):"); Serial.print(duty_us);
  Serial.print(",High:");  Serial.print(_TARGET_HIGH);
  Serial.print(",Max:");   Serial.print(_DIST_MAX);
  Serial.println();
}

// ---------- Implementations ----------
float clampf(float x, float a, float b) {
  if (x < a) return a;
  if (x > b) return b;
  return x;
}
float lerp(float a, float b, float t) {
  return a + (b - a) * t;
}

float USS_measure(int TRIG, int ECHO) {
  // 권장 트리거 시퀀스
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(PULSE_DURATION);
  digitalWrite(TRIG, LOW);

  // echo high duration (us) → 거리(mm)
  unsigned long dur = pulseIn(ECHO, HIGH, (unsigned long)TIMEOUT);
  return (float)dur * (float)SCALE;
}
