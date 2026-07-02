/**
 * @brief 내부 외부 pn532 초기활성화 및 실패시  goto문 반복
 */
void RfidInit()
{
  for (int i = 0; i < rfid_num; ++i)
  {
    nfc[i].begin();
    if (!(nfc[i].getFirmwareVersion()))
    {
      Serial.println("PN532 연결실패 : " + String(i));
      rfid_init_complete[i] = false;
    }
    else
    {
      nfc[i].SAMConfig();
      nfc[i].setPassiveActivationRetries(0x01); // 기본값(무한 재시도)이면 카드 없을 때 InList 응답 대기로 ~1초씩 블로킹됨
      Serial.println("PN532 연결성공 : " + String(i));
      rfid_init_complete[i] = true;
      AllNeoOn(YELLOW);
    }
    delay(100);
  }
}
/**
 * @brief 아이템박스 내부 pn532 태그 읽어와서 CheckingPlayer로 전송
 */
void RfidLoopInner()
{
  uint8_t uid[7] = {0};   // Buffer to store the returned UID
  uint8_t uidLength = 0;  // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
  uint8_t data[32];
  // readPassiveTargetID: InList 응답을 끝까지 소비하고 타겟 수를 확인.
  // setPassiveActivationRetries(1) 덕분에 카드가 없어도 150ms 안에 반환됨 (기존 패턴은 ~1초 블로킹)
  if (nfc[INPN532].readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 150))
  {
    if (nfc[INPN532].ntag2xx_ReadPage(7, data))
    {                                                                       // ntag 데이터에 접근해서 불러와서 data행열에 저장
      Serial.println("TAGGGED");
      CheckingPlayers(data);
    }
  }
  // TODO InnerRFID 루프시 연결 안되면 워치독
  // else(
  //   ESP.restart();
  // )
}
/**
 * @brief 아이템박스 외부 pn532 태그 읽어와서 CheckingPlayer로 전송
 */
void RfidLoopOutter()
{
  uint8_t uid[7] = {0};   // Buffer to store the returned UID
  uint8_t uidLength = 0;  // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
  uint8_t data[32];
  // readPassiveTargetID: InList 응답을 끝까지 소비하고 타겟 수를 확인.
  // setPassiveActivationRetries(1) 덕분에 카드가 없어도 150ms 안에 반환됨 (기존 패턴은 ~1초 블로킹)
  if (nfc[OUTPN532].readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 150))
  {
    if (nfc[OUTPN532].ntag2xx_ReadPage(7, data))
    {                                                                       // ntag 데이터에 접근해서 불러와서 data행열에 저장
      Serial.println("TAGGGED");
      CheckingPlayers(data);
    }
  }
  // TODO OutterRFID 루프시 연결 안되면 워치독
  // else(
  //   ESP.restart();
  // )
}

/**
 * @brief 내외부에서 태그한 카드데이터 string으로 변환후 ID 기반 고정 역할 판단 후 ptrRfidMode로 전송
 * G9P1=술래, G9P2=유령, G9P3~G9P9=생존자
 */
void CheckingPlayers(uint8_t rfidData[32])                // 어떤 카드가 들어왔는지 확인용
{
  String tagUser = "";                                    // 읽어온 uint_8t값 string으로 변환하기 위한 String 변수
  for (int i = 0; i < 4; i++)                             // GxPx 데이터만 배열에서 추출해서 string으로 저장
    tagUser += (char)rfidData[i];
  Serial.println("tag_user_data : " + tagUser);
  if (tagUser == "MMMM")
  {                                                       //"MMMM"일경우 DB요청 하지 않고 바로 watchdog 실행(DB에 MMMM 플레이어는 존재하지 않아서 요청하면 오류 발생)
    ESP.restart();
  }
  if (ptrRfidMode != ResumePuzzle)                        // 퍼즐 재진입은 블로킹 HTTP 없이 즉시 복귀 (역할 판단은 아래 ID 기반이라 서버 조회 불필요)
    has2wifi.Receive(tagUser);                            // 플레이어 데이터 수신

  // ID 기반 고정 역할 판단
  if (tagUser == "G9P1")                                  // [TEMP] 술래 → 생존자처럼 동작
  {
    Serial.println("Tagger Tagged (TEMP: treated as survivor)");
    ptrRfidMode();
  }
  else if (tagUser == "G9P2")                             // 유령 → 퍼즐 진입 불가
  {
    Serial.println("Ghost Tagged (ignored)");
  }
  else if (tagUser.startsWith("G9P") && tagUser[3] >= '3' && tagUser[3] <= '9') // 생존자
  {
    Serial.println("Player Tagged");
    ptrRfidMode();
  }
  else                                                    // 예외 처리
    Serial.println("Wrong TAG");
}

/**
 * @brief Activate 상황에서 외부 pn532 태그시 엔코더 활성화 후 RFID 중지 Puzzle함수 실행
 */
void StartPuzzle()
{
  Serial.println("StartPuzzle");
  puzzleMode = true;
  WifiTimer.deleteTimer(wifiTimerId); // 퍼즐 진입 시 WiFi 완전 차단
  answerCnt = 0;
  rfidLastSeenTime = millis();        // RFID 이탈 감지 기준 시각 초기화
  GameTimer.deleteTimer(gameTimerId);
  gameTimerId = GameTimer.setInterval(puzzleResetTime, GameTimerFunc); // 퍼즐 진입 시 비입력 리셋 타이머 시작
  ptrCurrentMode = Puzzle;                                      // ptr함수의 주소를 RFIDOuter -> Puzzle로 변환
  AllNeoOn(BLUE);                                               // puzzle 함수 진행동안 전체 네오픽셀 파란색 유지
  attachInterrupt(encoderPinA, updateEncoder, CHANGE);          // 엔코더 하드웨어 인터럽트 활성화
  attachInterrupt(encoderPinB, updateEncoder, CHANGE);
}

/**
 * @brief 퍼즐 중 RFID 이탈 후 다시 태그했을때 실행 - answerCnt 유지한 채 퍼즐 재진입
 */
void ResumePuzzle()
{
  Serial.println("ResumePuzzle - answerCnt: " + String(answerCnt));
  puzzleMode = true;
  WifiTimer.deleteTimer(wifiTimerId); // 퍼즐 재진입 시 WiFi 완전 차단
  rfidLastSeenTime = millis();
  GameTimer.deleteTimer(gameTimerId);
  gameTimerId = GameTimer.setInterval(puzzleResetTime, GameTimerFunc); // 재진입 시 비입력 타이머 재시작
  ptrCurrentMode = Puzzle;
  AllNeoOn(BLUE);
  attachInterrupt(encoderPinA, updateEncoder, CHANGE);
  attachInterrupt(encoderPinB, updateEncoder, CHANGE);
}

/**
 * @brief Puzzle함수로 문제를 다 맞춘 후 완료하는 태그를 실행했을때 실행되는 함수
 */
void PuzzleSolved()
{
  itemBoxSelfOpen = true;                                                         // 태그하면 아이템박스가 open 상태 임으로 메인에서 open 명령 들어와도 재실행되지 않게 제한하는 bool 변수
  Serial.println("PuzzleSolved");
  AllNeoOn(BLUE);
  BoxOpen();                        // 아박 오픈 (논블로킹). 모터를 먼저 돌려 박스가 즉시 열리기 시작한다.
  BatteryPackSend();
  sendCommand("page pgItemOpen");   // 모터 구동과 동시에 즉시 넥션 화면 전환
  SendLanguage();
  ExpSend();
  if ((String)(const char*)shift_machine["selected_language"] != "EN")
      sendCommand("wQuizSolved.en=1");
  else
      sendCommand("wEQuizSolved.en=1");
  pendingOpenScreen = true;             // 모터 종료 후 서버 보고 + RfidLoopInner 활성화 예약
  BlinkTimer.deleteTimer(blinkTimerId); // 전에 사용된 BlinkTimer를 초기화하고 다시 시작하기 위해 종료
  BlinkTimerStart(INNER, YELLOW);       // 내부태그 네오픽셀 노란색 점멸 시작
  GameTimer.deleteTimer(gameTimerId);   // Puzzle함수 -> PuzzleSolved함수 진행되면 이후로는 Activate로 초기화 되지 않게 타이머 종료(기획대로)
  nfc[OUTPN532].SAMConfig();            // InListPassiveTarget 잔류 타겟 해제 - 다음 게임 Activate 시 RfidLoopOutter 정상 동작 보장
  ptrCurrentMode = WaitFunc;            // 모터 구동 중에는 내부 태그를 받지 않음. loop()에서 모터 정지+안정화+open 보고가 끝난 뒤 RfidLoopInner로 전환
  ptrRfidMode = ItemTook;
}

/**
 * @brief PuzzleSolved 함수 실행후 내부 RIFD태그 되어있을때 실행되는 함수 (UIUX만 바뀌고 실제로 배터리와 경험치는 보내지 않음)
 */
void ItemTook()
{
  Serial.println("ItemTook");
  sendCommand("page pgItemTaken");
  AllNeoOn(BLUE);
  has2wifi.Send((String)(const char *)my["device_name"], "device_state", "used");
  BlinkTimer.deleteTimer(blinkTimerId);
  wifiTimerId = WifiTimer.setInterval(wifiTime, WifiIntervalFunc); // 아이템 획득 완료 후 WiFi 타이머 재개
  itemBoxUsed = true;
  ptrCurrentMode = WaitFunc;
  ptrRfidMode = WaitFunc;
}