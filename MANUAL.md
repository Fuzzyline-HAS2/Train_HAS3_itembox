# 장치 운용 메뉴얼

---

## 공통 기능

### 밝기 조절 (Brightness)

모든 장치는 아래 표준 구현을 동일하게 사용한다.

통일 전 각 장치의 기본값이 달라 서버에서 같은 값을 보내도 장치마다 밝기가 달랐다.

| 장치 | 통일 전 기본값(raw) | 실제 밝기% | 문제점 |
|------|-----------------|----------|--------|
| updated_itembox | 255 | 100% | — |
| updated_tagmachine_main | 255 | 100% | — |
| updated_generator | 100 (서버단위) → 255 | 100% | 단위 불일치 |
| updated_duct | 색상 20 / 선형 100 | 8% / 39% | 두 개 분리, 값 불일치 |
| updated_escape_main | 20 | 8% | — |
| updated_revival_machine | 20 | 8% | 색 배열에 직접 삽입 |
| updated_temple | 없음 | 0% (서버 0 시 꺼짐) | 기본값 부재 |
| updated_IoTglove | 20 (`standard_neo`) | 8% | 서버 수신 없음 |

**→ 전 장치 기본 밝기 50 (raw 0~255)으로 통일 완료**

#### 표준 전역 변수 (헤더 파일)

대부분의 장치:
```cpp
#define DEFAULT_BRIGHTNESS 50
int ledBrightness = DEFAULT_BRIGHTNESS;
```

**updated_duct 예외**: 라인 LED와 원형/스위치 LED의 역할이 달라 밝기 변수를 두 개 유지한다. 기본값은 동일하게 50.
```cpp
#define DEFAULT_COLOR_BRIGHTNESS 50  // pixels_round, pixels_switch
#define DEFAULT_LINE_BRIGHTNESS  50  // pixels_line
int colorBrightness = DEFAULT_COLOR_BRIGHTNESS;
int lineBrightness  = DEFAULT_LINE_BRIGHTNESS;
```

#### 초기화 — NeopixelInit()

`setup()`에서 호출한다. 모든 NeoPixel 객체에 begin() → setBrightness() → 흰색 점등 순서로 초기화한다.

```cpp
void NeopixelInit() {
    for (int i = 0; i < NeopixelNum; ++i) {
        pixels[i].begin();
        pixels[i].setBrightness(ledBrightness);
    }
    for (int i = 0; i < NeopixelNum; ++i) {
        lightColor(pixels[i], color[WHITE]);
    }
}
```

#### 밝기 갱신 — UpdateBrightness()

서버 값이 바뀔 때 `DataChanged()` 안에서 호출한다.

```cpp
void UpdateBrightness() {
    int serverBrightness = my["brightness"].as<int>();
    if (serverBrightness <= 0 || serverBrightness > 100) {
        ledBrightness = DEFAULT_BRIGHTNESS;
    } else {
        ledBrightness = map(serverBrightness, 1, 100, 1, 255);
    }
    for (int i = 0; i < NeopixelNum; ++i) {
        pixels[i].setBrightness(ledBrightness);
        pixels[i].show();
    }
}
```

- 서버 수신 범위: 1~100
- 0 이하이거나 100 초과이면 `DEFAULT_BRIGHTNESS` 적용
- 내부 범위: 1~255 (`map` 변환)

#### DataChanged()에서 호출 방법

```cpp
void DataChanged() {
    int serverBrightness = my["brightness"].as<int>();
    if (serverBrightness != cur["brightness"].as<int>()) {
        UpdateBrightness();
    }
    // ... 이하 각 장치별 처리
}
```

값이 변경된 경우에만 `UpdateBrightness()`를 호출한다.

---

### 부팅 시 nullptr 크래시

부팅 직후 두 가지 조건에서 장치가 크래시(재시작 루프)에 빠질 수 있다.

#### 원인 1 — WiFi 연결 실패

`has2wifi.Setup(theme)`에서 해당 테마의 SSID 목록을 순서대로 시도한다. 각 SSID당 약 2초(100ms × 20회) 내에 연결되지 않으면 다음 SSID로 넘어가고, 목록을 모두 소진하면 `ESP.restart()`로 자동 재시작된다.

```
badland_ruins → 실패
badland_shoot → 실패
...
badland_auto  → 실패  →  ESP.restart()
```

- 재시작이 반복되면 전원 공급 문제 또는 AP 미동작 상태이므로 **라우터/AP 상태를 먼저 확인**한다.
- `has2wifi.Setup("badland")`와 같이 테마 문자열이 오타가 나면 SSID 목록 자체를 건너뛰어 즉시 재시작 루프에 빠진다.

#### 원인 2 — device_name null (서버 미등록)

WiFi 연결 성공 후 `ReceiveMine()`을 호출하여 자신의 MAC 주소로 서버 DB에서 장치 정보를 받아온다. 서버에 해당 MAC이 등록되어 있지 않거나 응답이 비정상이면 `my["device_name"]`이 null이 된다.

이후 코드 어디서든 `(const char*)my["device_name"]`를 역참조하면 nullptr가 되어 크래시한다.

```cpp
// 크래시 발생 지점 예시
has2wifi.Send((String)(const char*)my["device_name"], "device_state", "setting");
//            ↑ my["device_name"]이 null이면 nullptr → String 생성 시 크래시
```

**확인 방법**: Serial 모니터에서 부팅 로그 확인

```
DeviceName : null       ← 이 줄이 출력되면 서버 미등록 상태
```

**해결**: 서버 DB에 해당 장치의 MAC 주소와 `device_name`을 등록한다. MAC 주소는 부팅 로그에서 확인할 수 있다.

```
MY MAC=AA:BB:CC:DD:EE:FF   ← 이 값을 서버에 등록
```

---

### 색상 변경 (lightColor)

모든 장치에서 동일한 시그니처를 사용한다.

```cpp
void lightColor(Adafruit_NeoPixel &neo, int c[3]) {
    neo.fill(neo.Color(c[0], c[1], c[2]));
    neo.show();
}
```

| 파라미터 | 타입 | 설명 |
|----------|------|------|
| `neo` | `Adafruit_NeoPixel &` | 색상을 적용할 NeoPixel 객체 참조 |
| `c[3]` | `int[3]` | RGB 배열 `{R, G, B}` (각 0~255) |

`fill()`로 전체 픽셀을 단번에 설정하고 `show()`로 출력한다.

**사용 예시**

```cpp
int red[3] = {255, 0, 0};
lightColor(pixels[0], red);          // pixels[0] 전체를 빨간색으로
lightColor(pixels[0], color[WHITE]); // 미리 정의된 WHITE 색상 배열 사용
```

---

---

## 아이템박스 전용 기능

### 모터 초기화

`setup()`에서 `MotorInit()`을 호출한다. 내부적으로 핀 모드 설정 → LEDC 채널 연결 → 박스 닫기 순으로 동작한다.

| 핀 상수 | 핀 번호 | 역할 |
|---------|---------|------|
| MOTOR_INA1_PIN | 32 | 방향 제어 A (H브리지 입력) |
| MOTOR_INA2_PIN | 4 | 방향 제어 B (H브리지 입력) |
| MOTOR_PWMA_PIN | 22 | PWM 속도 제어 (LEDC 채널) |
| BOXSWITCH_PIN | 36 | 마이크로스위치 입력 (닫힘 감지) |
| VIBRATION_ANSWER_PIN | 12 | 진동 모터 (정답 피드백) |
| VIBRATION_RANGE_PIN | 14 | 진동 모터 (범위 피드백) |

---

### 모터 작동 원리

선형 모터는 H브리지를 통해 INA1/INA2 방향 핀과 PWM 신호로 제어한다.

| 동작 | INA1 | INA2 | PWM |
|------|------|------|-----|
| 열기 | HIGH | LOW | MAX |
| 닫기 | LOW | HIGH | MAX |
| 정지 (브레이크) | LOW | LOW | 유지 |

- **열기**: 타이머 3,000ms 만료 시 `MotorStop()` 자동 호출
- **닫기**: 마이크로스위치(BOXSWITCH_PIN) HIGH 감지 시 `MotorStop()` 자동 호출

---

### 모터 사용법

| 함수 | 동작 |
|------|------|
| `BoxOpen()` | 핀 모드·LEDC 재설정 후 열기 방향으로 구동 |
| `BoxClose()` | 핀 모드·LEDC 재설정 후 닫기 방향으로 구동 |
| `MotorStop()` | INA1/INA2 모두 LOW (브레이크 모드, PWM은 건드리지 않음) |
| `EncoderVibrationStrength()` | 엔코더 값과 정답의 차이에 따라 진동 강도 4단계 조절 |

#### 주의: BoxOpen / BoxClose 호출 시 매번 핀 복구가 필요한 이유

ESP32에서 두 가지 하드웨어 부작용이 확인되었다.

**부작용 1 — `ledcWrite(0)`이 LEDC 채널을 핀에서 분리함**

ESP32 Arduino Core 일부 버전에서 `ledcWrite(pin, 0)` 호출 시 duty를 0으로 내리는 것에 더해 해당 핀을 LEDC 채널에서 분리하고 일반 GPIO 모드로 전환한다. 이후 `ledcWrite(pin, MAX)`를 호출해도 레지스터만 업데이트될 뿐 물리적 핀 전압은 변화하지 않아 모터가 무반응 상태가 된다.

→ `MotorStop()`에서 `ledcWrite(0)`을 제거하고, `BoxOpen()`/`BoxClose()` 호출 시마다 `ledcAttach()`로 채널을 명시적으로 재연결한다.

**부작용 2 — WiFi 전송 후 GPIO4(MOTOR_INA2_PIN)가 OUTPUT 모드를 잃음**

`has2wifi.Send()` (HTTP 요청) 실행 후 ESP32 WiFi 스택이 내부 동작 중 GPIO4의 핀 모드를 변경하는 것으로 추정된다. 이 상태에서 `digitalWrite(MOTOR_INA2_PIN, HIGH)`를 호출해도 외부 풀다운에 의해 핀이 LOW로 고착되어, INA1=LOW·INA2=LOW인 브레이크 모드가 되어 모터가 완전히 무반응이 된다.

→ `BoxOpen()`/`BoxClose()` 첫 줄에 `pinMode(MOTOR_INA1_PIN, OUTPUT)` / `pinMode(MOTOR_INA2_PIN, OUTPUT)`을 명시적으로 재설정한다.

```cpp
void BoxOpen() {
    pinMode(MOTOR_INA1_PIN, OUTPUT);
    pinMode(MOTOR_INA2_PIN, OUTPUT);
    ledcAttach(MOTOR_PWMA_PIN, MotorFreq, MotorResolution);
    ledcWrite(MOTOR_PWMA_PIN, MotorMAX_DUTY_CYCLE - 1);
    digitalWrite(MOTOR_INA1_PIN, HIGH);
    digitalWrite(MOTOR_INA2_PIN, LOW);
}

void BoxClose() {
    pinMode(MOTOR_INA1_PIN, OUTPUT);
    pinMode(MOTOR_INA2_PIN, OUTPUT);
    ledcAttach(MOTOR_PWMA_PIN, MotorFreq, MotorResolution);
    ledcWrite(MOTOR_PWMA_PIN, MotorMAX_DUTY_CYCLE - 1);
    digitalWrite(MOTOR_INA1_PIN, LOW);
    digitalWrite(MOTOR_INA2_PIN, HIGH);
}

void MotorStop() {
    digitalWrite(MOTOR_INA1_PIN, LOW);
    digitalWrite(MOTOR_INA2_PIN, LOW);
    // ledcWrite(0) 사용 금지 — LEDC 채널 분리 부작용 발생
}
```

---

### 진동 모터

VIBRATION_RANGE_PIN의 PWM 채널로 세기를 제어하며, 엔코더 값과 정답의 차이에 따라 4단계로 출력이 조절된다.

---

### 마이크로스위치 동작

- **핀**: BOXSWITCH_PIN (36번, INPUT)
- **HIGH**: 박스 완전히 닫힘 → `MotorStop()` 호출
- **LOW**: 박스 열려 있음

`BoxClose()` 진행 중 마이크로스위치가 HIGH를 감지하는 순간 모터를 정지시켜 과부하를 방지한다.

---

### 변경된 기획 내용

#### 1. 외부 RFID + 엔코더 동시 조건

퍼즐 진행 중에는 **외부 RFID를 지속적으로 태그하고 있는 동안에만** 엔코더 입력이 유효하다.

- RFID를 2초 이상 이탈하면 퍼즐이 일시 중지되고 엔코더 입력이 비활성화된다.
- RFID를 다시 태그하면 퍼즐이 재개된다.

#### 2. 서버에서 퍼즐 정답 랜덤 수신

서버 필드 `puzzle_answer_1`, `puzzle_answer_2`, `puzzle_answer_3` 값을 수신하여 정답으로 사용한다.

- 서버가 보내는 값은 0~95 범위의 정수
- 값이 변경될 때만 내부 정답 배열(`modeValue[ANSWER]`)을 갱신
- 정답 개수는 `puzzle_answer_count` 필드로 제어 (기본 3개)

#### 3. 퍼즐 풀이 도중 서버 상태 수신

퍼즐 진행 중에도 2초 주기로 서버 데이터를 수신한다.

- 수신 직전 엔코더 인터럽트를 해제 → 수신 후 인터럽트 재연결 (엔코더 값 보존)
- 수신 가능한 상태값: `game_state`, `device_state`, `brightness`, `puzzle_answer_*`
- 상태 변화에 따라 퍼즐 중단/재개/강제 종료 등이 즉각 반영된다.

#### 4. 퍼즐 완료 후 외부 RFID 불필요

모든 정답을 맞추면 외부 RFID 없이 박스가 자동으로 열린다.

1. 정답 개수 충족 → `PuzzleSolved()` 호출
2. `BoxOpen()` 자동 실행, 서버에 `device_state = "open"` 전송
3. 외부 RFID(OUTPN532) 비활성화
4. 내부 RFID(INPN532) 활성화 → 플레이어가 배터리팩/경험치 수령 시 태그
5. 수령 완료 후 모든 RFID 기능 종료
