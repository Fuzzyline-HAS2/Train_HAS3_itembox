# updated_itembox

ESP32 기반 방탈출 아이템박스 펌웨어입니다.
RFID 태그 인식 → 엔코더 퍼즐 풀기 → 박스 개방의 흐름으로 동작하며, WiFi를 통해 서버와 상태를 동기화합니다.

---

## 하드웨어 구성

| 부품 | 역할 |
|------|------|
| ESP32 | 메인 컨트롤러 |
| Adafruit PN532 x2 | 외부 / 내부 RFID 태그 인식 |
| Adafruit NeoPixel x3 | 상태 표시 LED (PN532용 28개, 인코더용 24개, 내부용 24개) |
| 로터리 엔코더 | 퍼즐 입력 |
| 리니어 모터 | 박스 개폐 |
| 진동 모터 | 퍼즐 힌트 (정답에 가까울수록 강하게 진동) |
| Nextion 디스플레이 | 게임 UI 출력 |

### 핀 배치

| 핀 | 기능 | 비고 |
|----|------|------|
| 25 | PN532 NeoPixel | |
| 26 | Encoder NeoPixel | |
| 27 | Inner NeoPixel | |
| 5 / 21 | PN532 SS1 / SS2 | |
| 18 / 19 / 23 | PN532 SCK / MISO / MOSI | |
| 32 / 4 / 22 | Motor INA1 / INA2 / PWM | |
| 13 / 15 | Encoder A / B | |
| 34 | Encoder Button | **input-only 핀 — 외부 풀업 저항 필수** |
| 14 | 진동모터 PWM | |
| 12 | 진동모터 Answer | |
| 33 / 39 | Nextion TX / RX | |
| 36 | Box Switch | **input-only 핀 — 외부 풀업 저항 필수** |

> **주의:** GPIO34, GPIO36은 ESP32 input-only 핀으로 `INPUT_PULLUP`이 동작하지 않습니다. 반드시 외부 풀업 저항(10kΩ)을 연결해야 합니다.

---

## 소프트웨어 구조

```
updated_itembox.ino  - setup() / loop()
updated_itembox.h    - 전역 변수 및 객체 선언
Library_and_pin.h    - 라이브러리 include 및 핀 정의
Wifi.ino             - WiFi 상태 수신 및 게임 상태 전환 함수
neopixel.ino         - NeoPixel 초기화 및 제어
motor.ino            - 리니어모터 / 진동모터 제어
rfid.ino             - RFID 태그 인식 및 퍼즐 흐름
encoder.ino          - 로터리 엔코더 인터럽트 처리
Game_system.ino      - 퍼즐 로직 (엔코더 정답 판정)
timer.ino            - SimpleTimer 기반 주기 함수 관리
nextion.ino          - Nextion 디스플레이 통신
```

---

## 게임 흐름

```
setting → ready → activate → (외부 RFID 태그) → 퍼즐(엔코더) → open → (내부 RFID 태그) → used
```

| 서버 상태 | 동작 |
|-----------|------|
| `setting` | 초기화, 박스 열기, 전체 WHITE LED |
| `ready` | 퍼즐 정답 랜덤 생성 및 서버 전송, 박스 닫기 |
| `activate` | 게임 시작, YELLOW LED, 외부 RFID 대기 |
| `open` | 박스 개방, YELLOW 점멸, 내부 RFID 대기 |
| `used` | 아이템 수령 처리, RED LED |
| `repaired_all` | 전체 수리 완료, BLUE LED, 박스 개방 |
| `player_win` | 플레이어 승리 |
| `player_lose` | 플레이어 패배 |

---

## 퍼즐 정답 랜덤 생성 (ReadyFunc)

- 정답 개수: 3개
- 1번 정답: 10 ~ 23 고정 범위
- 2~3번 정답: 1 ~ 95 범위
- 정답 간격: 최소 10칸, 최대 30칸
- 생성된 정답은 WiFi로 서버에 전송됨

> 배터리팩 기기 간 랜덤 분배는 **서버에서 처리**합니다. 펌웨어의 기기 간 분배 코드는 제거되었습니다.

---

## 주요 구현 주의사항

### BoxOpen / BoxClose 반드시 MotorStop 호출

```cpp
void BoxOpen() {
    ledcWrite(MOTOR_PWMA_PIN, MotorMAX_DUTY_CYCLE - 1);
    digitalWrite(MOTOR_INA1_PIN, HIGH);
    digitalWrite(MOTOR_INA2_PIN, LOW);
    delay(4000);
    MotorStop();  // 반드시 호출 — 미호출 시 모터 과열 및 손상
}
```

`BoxOpen()` / `BoxClose()` 완료 후 **반드시 `MotorStop()`을 호출**해야 합니다.
리니어 모터는 도달 한계를 스스로 감지하지 못하므로, 전원을 끊어주지 않으면 모터가 계속 구동되어 과열 및 파손으로 이어집니다.

### Nextion 페이지 전환 후 딜레이

```cpp
delay(2000);
sendCommand("page pgItemOpen");  // 딜레이 이후 페이지 전환
sendCommand("wOutTagged.en=1");
ExpSend();
BatteryPackSend();
```

`page` 명령은 Nextion이 수신하는 즉시 화면이 전환됩니다. 연출상 딜레이가 필요하다면 **`page` 명령 앞**에 `delay()`를 둬야 합니다. 뒤에 두면 화면은 바로 바뀌고 딜레이만 뒤로 밀립니다.

### Nextion pic ID 관리

`BatteryPackSend()` / `ExpSend()`에서 `picBatteryPack.pic=N` 형태로 pic ID를 하드코딩합니다.
Nextion Editor에서 Picture 라이브러리 이미지를 추가/삭제/재정렬하면 ID가 밀리므로, HMI 수정 후에는 반드시 코드의 pic ID와 일치하는지 확인하세요.

| 변수 | pic ID |
|------|--------|
| exp_pack = 10 | 0 |
| exp_pack = 30 | 1 |
| exp_pack = 50 | 2 |
| battery_pack = 1 | 4 |
| battery_pack = 2 | 5 |
| battery_pack = 3 | 6 |
| battery_pack = 4 | 7 |
| battery_pack = 5 | 7 |

---

## 사용 라이브러리

- `Adafruit_NeoPixel`
- `Adafruit_PN532`
- `ArduinoJson`
- `HAS2_Wifi`
- `SimpleTimer`
- `Nextion` (ITEADLIB)

---

## 빌드 환경

- Board: ESP32 Dev Module
- Arduino IDE
- ESP32 Arduino Core v3.x 이상 (`ledcAttach` API 사용)
