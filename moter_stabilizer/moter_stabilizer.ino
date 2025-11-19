// ==========================
// 핀 설정
// ==========================
const int ENA_PIN = 3;   // L298N ENA (PWM)
const int IN1_PIN = 5;   // L298N IN1
const int IN2_PIN = 6;   // L298N IN2

const int ENC_CLK_PIN = 2; // KY-040 CLK (인터럽트)
const int ENC_DT_PIN  = 4; // KY-040 DT (지금은 사용 X)

// ==========================
// 엔코더 관련 변수
// ==========================
volatile long encoderPulses = 0;  // 인터럽트에서 증가시킬 펄스 카운트

// 샘플링 간격 (ms)
const unsigned long SAMPLE_INTERVAL_MS = 200;  // 0.2초마다 rpm 계산

//KY-040 한 바퀴당 펄스 수 
const float PULSES_PER_REV = 20.0;  
// ==========================
// 제어 관련 변수
// ==========================
float currentRPM = 0.0;

const float TARGET_RPM = 180.0;     // 목표 속도
const float RPM_TOLERANCE = 10.0;   // 허용 오차 범위. 이 안에 들어오면 유지 모드
const int PWM_STEP = 5;            // 한 번 조정할 때 PWM 변화량

int pwmValue = 180;                // 초기 PWM

unsigned long lastSampleTime = 0;

// mode 표시용 변수
//  1  → 가속 모드 (PWM 증가)
//  0  → 유지 모드 (PWM 그대로)
// -1  → 감속 모드 (PWM 감소)
int controlMode = 0;

// ==========================
// 엔코더 인터럽트 서비스 루틴
// ==========================
void encoderISR() {
  encoderPulses++;   // CLK이 RISING될 때마다 1씩 증가
}

// ==========================
// 초기 설정
// ==========================
void setup() {
  Serial.begin(9600);

  // 모터 핀 설정
  pinMode(ENA_PIN, OUTPUT);
  pinMode(IN1_PIN, OUTPUT);
  pinMode(IN2_PIN, OUTPUT);

  // 엔코더 핀 설정
  pinMode(ENC_CLK_PIN, INPUT_PULLUP);
  pinMode(ENC_DT_PIN, INPUT_PULLUP);

  // 인터럽트 등록 (CLK 핀 기준)
  attachInterrupt(digitalPinToInterrupt(ENC_CLK_PIN), encoderISR, RISING);

  // 모터 정방향 설정
  digitalWrite(IN1_PIN, HIGH);
  digitalWrite(IN2_PIN, LOW);

  // 초기 PWM 출력
  analogWrite(ENA_PIN, pwmValue);

  Serial.println("=== DC 모터 속도 자동 보정 실험 시작 ===");
  Serial.println("오차에 따라 자동으로 가속/감속/유지 모드를 선택합니다.");
  Serial.println();
  Serial.println("time_s\tRPM\tPWM\tmode"); // Serial Plotter용 헤더
}

// ==========================
// 메인 루프
// ==========================
void loop() {
  unsigned long now = millis();

  // 일정 시간마다 RPM 계산 및 제어
  if (now - lastSampleTime >= SAMPLE_INTERVAL_MS) {
    lastSampleTime = now;

    // 인터럽트에서 읽던 펄스 값 안전하게 복사
    noInterrupts();
    long pulses = encoderPulses;
    encoderPulses = 0;
    interrupts();

    // 샘플링 구간 동안 회전수 계산
    float intervalSec = SAMPLE_INTERVAL_MS / 1000.0;   // ms → s
    float revolutions = pulses / PULSES_PER_REV;       

    // RPM 계산
    currentRPM = (revolutions / intervalSec) * 60.0;

    // 오차 계산
    float error = TARGET_RPM - currentRPM;

    // 오차 기준으로 모드 결정
    if (error > RPM_TOLERANCE) {
      // 목표보다 많이 느림 → 가속
      pwmValue += PWM_STEP;
      controlMode = 1;
    } else if (error < -RPM_TOLERANCE) {
      // 목표보다 많이 빠름 → 감속
      pwmValue -= PWM_STEP;
      controlMode = -1;
    } else {
      // 목표 근처 → 유지
      controlMode = 0;
      // pwmValue 그대로
    }

    // PWM 범위 제한
    if (pwmValue > 255) pwmValue = 255;
    if (pwmValue < 0)   pwmValue = 0;

    // 모터에 PWM 적용
    analogWrite(ENA_PIN, pwmValue);

    // 상태 출력 (Serial Plotter에서 그래프 보기)
    Serial.print(now / 1000.0);   // 시간 (초)
    Serial.print("\t");
    Serial.print(currentRPM);     // 현재 RPM
    Serial.print("\t");
    Serial.print(pwmValue);       // 현재 PWM 값
    Serial.print("\t");
    Serial.println(controlMode);  // 모드: 1, 0, -1
  }
}
