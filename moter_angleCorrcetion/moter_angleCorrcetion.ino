// ==========================
// 핀 설정
// ==========================
const int ENA_PIN = 3;   // L298N ENA (PWM)
const int IN1_PIN = 5;   // L298N IN1
const int IN2_PIN = 6;   // L298N IN2

const int ENC_CLK_PIN = 2; // KY-040 CLK (인터럽트)
const int ENC_DT_PIN  = 4; // KY-040 DT

// ==========================
// 엔코더 관련 변수
// ==========================
volatile long encoderCount = 0;  // 펄스 누적용 (증가 또는 감소)

// 네가 보정 코드로 직접 측정한 값으로 바꿔 넣기!
const float PULSES_PER_REV = 24.0;  // 예시값. 너의 실제 측정값으로 수정.

// 한 펄스당 각도
const float DEG_PER_PULSE = 360.0 / PULSES_PER_REV;

// 위치 제어 관련
float currentAngle = 0.0;
const float TARGET_ANGLE = 90.0;    // 목표 각도 (예: 90도)
const float DEAD_BAND   = 3.0;      // 이 안이면 도착했다고 보고 정지
const float SLOW_BAND   = 15.0;     // 이보다 밖이면 빠른 속도

unsigned long lastPrintTime = 0;
const unsigned long PRINT_INTERVAL_MS = 200;

// 현재 제어 상태 표시용
//  1  → 정방향 회전
// -1  → 역방향 회전
//  0  → 정지
int controlMode = 0;

// ==========================
// 엔코더 인터럽트 서비스 루틴
// ==========================
// CLK가 올라가는 순간에 DT를 읽어서 회전 방향을 판단하는 패턴
void encoderISR() {
  int dtState = digitalRead(ENC_DT_PIN);
  if (dtState == HIGH) {
    encoderCount++;   // 한쪽 방향
  } else {
    encoderCount--;   // 반대 방향
  }
}

// ==========================
// 초기 설정
// ==========================
void setup() {
  Serial.begin(115200);

  // 모터 핀
  pinMode(ENA_PIN, OUTPUT);
  pinMode(IN1_PIN, OUTPUT);
  pinMode(IN2_PIN, OUTPUT);

  // 엔코더 핀
  pinMode(ENC_CLK_PIN, INPUT_PULLUP);
  pinMode(ENC_DT_PIN, INPUT_PULLUP);

  // 인터럽트 연결
  attachInterrupt(digitalPinToInterrupt(ENC_CLK_PIN), encoderISR, RISING);

  // 처음에는 정지 상태
  digitalWrite(IN1_PIN, LOW);
  digitalWrite(IN2_PIN, LOW);
  analogWrite(ENA_PIN, 0);

  Serial.println("=== DC 기어드 모터 위치 제어 실험 시작 ===");
  Serial.println("전원을 켜고 모터를 원하는 기준 위치에 맞춘 뒤 리셋을 눌러 각도 0도로 기준을 잡으세요.");
  Serial.println("TARGET_ANGLE까지 이동하면서 마찰과 백래시로 인한 남는 오차를 관찰합니다.");
  Serial.println();
  Serial.println("time_s\tangle_deg\terror_deg\tpwm\tmode");
}

// ==========================
// 메인 루프
// ==========================
void loop() {
  unsigned long now = millis();

  // 엔코더 카운트를 각도로 변환
  noInterrupts();
  long countCopy = encoderCount;
  interrupts();
  currentAngle = countCopy * DEG_PER_PULSE;

  // 목표 각도와의 오차
  float error = TARGET_ANGLE - currentAngle;
  float absError = abs(error);

  int pwmValue = 0;

  if (absError <= DEAD_BAND) {
    // 목표 근처 → 정지
    controlMode = 0;
    pwmValue = 0;
    digitalWrite(IN1_PIN, LOW);
    digitalWrite(IN2_PIN, LOW);
  } else {
    // 방향 결정
    if (error > 0) {
      // 목표 각도가 현재보다 큼 → 정방향 회전
      controlMode = 1;
      digitalWrite(IN1_PIN, HIGH);
      digitalWrite(IN2_PIN, LOW);
    } else {
      // 목표 각도가 현재보다 작음 → 역방향 회전
      controlMode = -1;
      digitalWrite(IN1_PIN, LOW);
      digitalWrite(IN2_PIN, HIGH);
    }

    // 오차 크기에 따라 속도( PWM ) 선택
    if (absError > SLOW_BAND) {
      pwmValue = 200;   // 멀리 있을 때는 빠르게 이동
    } else {
      pwmValue = 120;   // 가까워지면 천천히 접근
    }
  }

  // PWM 적용
  analogWrite(ENA_PIN, pwmValue);

  // 주기적으로 상태 출력
  if (now - lastPrintTime >= PRINT_INTERVAL_MS) {
    lastPrintTime = now;

    Serial.print(now / 1000.0);  // time_s
    Serial.print("\t");
    Serial.print(currentAngle);  // angle_deg
    Serial.print("\t");
    Serial.print(error);         // error_deg
    Serial.print("\t");
    Serial.print(pwmValue);      // pwm
    Serial.print("\t");
    Serial.println(controlMode); // mode
  }
}
