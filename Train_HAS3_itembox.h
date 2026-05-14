#ifndef _DONE_ITEMBOX_CODE_
#define _DONE_ITEMBOX_CODE_

#include "Library_and_pin.h"

class TelnetDebugConsole : public Stream {
public:
  void begin(unsigned long baud);
  int available() override;
  int read() override;
  int peek() override;
  void flush() override;
  size_t write(uint8_t data) override;
  size_t write(const uint8_t *buffer, size_t size) override;
};
extern HardwareSerial HardwareDebugSerial;
extern TelnetDebugConsole DebugSerial;
#define Serial DebugSerial

const int rfid_num = 2; // 설치된 pn532의 개수

//****************************************WIFI****************************************************************
HAS2_Wifi has2wifi("http://172.30.1.43");
SecureOTA ota(
    "https://raw.githubusercontent.com/Fuzzyline-HAS2/Train_HAS3_itembox/third_store/update.bin",
    "https://raw.githubusercontent.com/Fuzzyline-HAS2/Train_HAS3_itembox/third_store/version.txt",
    "https://raw.githubusercontent.com/Fuzzyline-HAS2/Train_HAS3_itembox/third_store/update.sig",
    HMAC_SECRET,
    FIRMWARE_VER
);
void DataChanged();
void SettingFunc(void);
void ActivateFunc(void);
void ReadyFunc(void);
void OpenFunc(void);
void WaitFunc(void);
void WifiIntervalLoop(unsigned long intervalValue);
unsigned long wifiInterval = 0;
//****************************************Game System****************************************************************
void (*ptrCurrentMode)() = WaitFunc;   //현재모드 저장용 포인터 함수
void (*ptrRfidMode)() = WaitFunc;      //rfid모드 저장용 포인터 함수
//****************************************Game System****************************************************************
unsigned long puzzleStartTime = 0; 
enum {VIBESTREGNTH = 0, ANSWER, RANGE};
enum {ANSWER_CNT = 0, ANSWER_RANGE, VIBRATION_RANGE};
int modeValue[3][5] = { {200,150,100,50,0},     // VIBESTREGNTH (모터 세기): 0~255 설정 가능
                         {13,43,21,0,0},        // ANSWER (Puzzle정답) 0 ~ 95 설정가능
                         {5,2,5,0,0}};          // RANGE {ANSWER_CNT, ANSWER_RANGE, VIBRATION_RANGE } -> 정답개수, 정답핀 범위, 진동 범위

int answerCnt = 0;          //엔코더 정답 맞춘 갯수 카운팅.
bool itemBoxSelfOpen = false;
bool itemBoxUsed = false;
enum {HI1=0,HI2,OI1,OI2,BI1,BI2,GI1,GI2,FI1,FI2};
struct BATTERYPACKRND
{
    String strDevice;
    int nVal;
};
//****************************************SimpleTimer SETUP****************************************************************
SimpleTimer GameTimer;
SimpleTimer WifiTimer;
SimpleTimer BlinkTimer;

void TimerInit();
void WifiIntervalFunc();
void GameTimerFunc();

void BlinkTimerFunc();
void BlinkTimerStart(int Neo, int NeoColor);
int blinkNeo = 0;
int blinkColor = 0;
bool blinkOn = false;

int wifiTimerId;
int gameTimerId;
int blinkTimerId;

unsigned long wifiTime = 2000;        // 2sec
unsigned long puzzleResetTime = 30000; // 퍼즐 리셋 시간 (ms), 서버 수신값으로 갱신됨
unsigned long blinkTime = 1300;        // 1.3sec
//****************************************Nextion SETUP****************************************************************
HardwareSerial nexHwSerial(2);
String nextion_str = "";
int currner_page_num;

void NextionInit();
void DisplayCheck();
void NextionReceived();
void ExpSend();
void BatteryPackSend();
//****************************************Neopixel SETUP****************************************************************
void NeopixelInit();
void EncoderNeopixelOn();
void NeoBlink(int neo, int neoColor, int cnt, int blinkTime);
void UpdateBrightness();
#define DEFAULT_BRIGHTNESS 50
int ledBrightness = DEFAULT_BRIGHTNESS;
const int NumPixels[3] = {28,24,24};
const int NeopixelNum = 3;
enum {PN532 = 0, INNER, ENCODER};
enum {WHITE = 0, RED, YELLOW, GREEN, BLUE, PURPLE, BLACK, BLUE0, BLUE1, BLUE2, BLUE3};
// Neopixel 색상정보
int color[11][3] = {{255, 255, 255}, //WHITE
                    {255, 0,   0  }, //RED
                    {255, 255, 0  }, //YELLOW
                    {0,   255, 0  }, //GREEN
                    {0,   0,   255}, //BLUE
                    {255, 0,   255}, //PURPLE
                    {0,   0,   0  }, //BLACK
                    {0,   0,   64 }, //ENCODERBLUE0
                    {0,   0,   128}, //ENCODERBLUE1
                    {0,   0,   192}, //ENCODERBLUE2
                    {0,   0,   255}}; //ENCODERBLUE3

const int neopixel_num = 3; // 설치된 네오픽셀의 개수

Adafruit_NeoPixel pixels[NeopixelNum] = {Adafruit_NeoPixel(NumPixels[PN532], PN532_NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800),
                                         Adafruit_NeoPixel(NumPixels[ENCODER], ENCODER_NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800),
                                         Adafruit_NeoPixel(NumPixels[INNER], INNER_NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800)};
                                         
//****************************************RFID SETUP****************************************************************
enum {OUTPN532 = 0, INPN532};
//const int rfid_num = 2; // 설치된 pn532의 개수

Adafruit_PN532 nfc[2] = {Adafruit_PN532(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS1),
                                Adafruit_PN532(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS2)};

bool rfid_tag;
bool rfid_timer_assess;

bool rfid_init_complete[2];
unsigned long rfidLastSeenTime = 0;            // 퍼즐 중 외부 RFID 마지막 감지 시각
const unsigned long rfidPuzzleTimeout = 2000;  // 퍼즐 RFID 이탈 타임아웃 (ms)
void RfidInit(void);
void RfidLoopInner();
void RfidLoopOutter();
void CheckingPlayers(uint8_t rfidData[32]);
void StartPuzzle();
void ResumePuzzle();
void PuzzleSolved();
void ItemTook();


//****************************************MOTOR SETUP****************************************************************
bool puzzleMode = false;         // 퍼즐 진행 중 여부 (WiFi 수신 시 노이즈 차단용)
bool boxMotorRunning = false;    // 모터 동작 중 여부
bool boxClosing = false;         // true=닫히는 중, false=열리는 중
bool pendingOpenScreen = false;  // BOX Opened 후 Nextion 화면 전환 대기
unsigned long motorStartTime = 0;            // 모터 구동 시작 시각
const unsigned long motorOpenDuration = 3000; // 열기 모터 구동 시간(ms) - 스위치 미사용, 타이머로 정지
const int MotorFreq = 5000;
const int MotorResolution = 8;
const int MotorLedChannel = 3;
const int VibrationLedChannel = 4;
const int MotorMAX_DUTY_CYCLE = (int)(pow(2, MotorResolution) - 1);
const int VibtationMAX_DUTY_CYCLE = (int)(pow(2, MotorResolution) - 1);
void MotorInit();
void BoxOpen();
void BoxClose();
void MotorStop();
void EncoderVibrationStrength(int answer);

//****************************************ENCODER SETUP****************************************************************
void EncoderInit();
long readEncoderValue(void);
void updateEncoder();

volatile int lastEncoded = 0;       //직전 엔코더 값
volatile long encoderValue = 165;     //현재 엔코더 값

long lastencoderValue = 0;

int lastMSB = 0;
int lastLSB = 0;

#endif
