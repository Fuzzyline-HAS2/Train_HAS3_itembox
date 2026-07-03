void DataChanged()
{
  static StaticJsonDocument<1000> cur;   //저장되어 있는 cur과 읽어온 my 값과 비교후 실행
  bool forceAnswerUpdate = false;
    if((String)(const char*)my["game_state"] != (String)(const char*)cur["game_state"])
    {
        if((String)(const char*)my["game_state"] == "setting"){
            forceAnswerUpdate = true;
            SettingFunc();
        }
        else if((String)(const char*)my["game_state"] == "ready"){
            forceAnswerUpdate = true;
            ReadyFunc();
        }
        else if((String)(const char*)my["game_state"] == "activate"){
            forceAnswerUpdate = true;
            ActivateFunc();
        }
    }
    if((String)(const char*)my["device_state"] != (String)(const char*)cur["device_state"])
    {
        if((String)(const char*)my["device_state"] == "activate"){
            ActivateFunc();
        }
        else if((String)(const char*)my["device_state"] == "used"){
            if(itemBoxUsed == false){
                BoxOpen();
                AllNeoOn(RED);
                sendCommand("page pgItemTaken");
                ptrCurrentMode = WaitFunc;
                ptrRfidMode = WaitFunc; 
                itemBoxUsed = true;
                BlinkTimer.deleteTimer(blinkTimerId);
                GameTimer.deleteTimer(gameTimerId);
            }
        }
        else if((String)(const char*)my["device_state"] == "open"){ //노브 puzzle 끝난상태
            if(itemBoxSelfOpen == false){                           //플레이어가 스스로 열었으면 true 여서 이미 열었다는 의미
                Serial.println("PuzzleSolved");
                AllNeoOn(BLUE);
                delay(2000);
                sendCommand("page pgItemOpen");
                sendCommand("wOutTagged.en=1");
                ExpSend();
                BatteryPackSend();
                BoxOpen();
                lightColor(pixels[INNER], color[YELLOW]);
                ptrCurrentMode = RfidLoopInner;
                ptrRfidMode = ItemTook;
                BlinkTimer.deleteTimer(blinkTimerId);
                BlinkTimerStart(INNER, YELLOW);                     //내부태그 노란색 점멸 시작
                GameTimer.deleteTimer(gameTimerId);                 // 엔코더 다 푼 이후에는 로그아웃 없이 현 상태 유지
                itemBoxSelfOpen = true;
            }
        }
        else if((String)(const char*)my["device_state"] == "close"){ 
            BoxClose();
        }
        else if((String)(const char*)my["device_state"] == "repaired_all"){ 
            ptrCurrentMode = WaitFunc;
            ptrRfidMode = WaitFunc;
            AllNeoOn(BLUE);
            BoxOpen();
            sendCommand("page pgEscapeOpen");
        }
        else if((String)(const char*)my["device_state"] == "player_win"){ 
            ptrCurrentMode = WaitFunc;
            ptrRfidMode = WaitFunc;
            AllNeoOn(BLUE);
            BoxOpen();
            sendCommand("page pgPlayerWin");
        }
        else if((String)(const char*)my["device_state"] == "player_lose"){
            ptrCurrentMode = WaitFunc;
            ptrRfidMode = WaitFunc;
            AllNeoOn(RED);
            BoxOpen();
            sendCommand("page pgPlayerLose");
        }
        else if((String)(const char*)my["device_state"] == "github") {
            ota.check();
        }

    }
  // 퍼즐 정답 서버 수신
  const char* answerKeys[] = {"puzzle_answer_1", "puzzle_answer_2", "puzzle_answer_3"};
  int totalAnswers = modeValue[RANGE][ANSWER_CNT];
  for (int i = 0; i < totalAnswers; i++) {
      int serverVal = my[answerKeys[i]].as<int>();
      int prevVal = cur[answerKeys[i]].as<int>();
      if (serverVal != 0 && (forceAnswerUpdate || serverVal != prevVal)) {
          modeValue[ANSWER][i] = serverVal;
          Serial.println(String(answerKeys[i]) + " 서버 수신: " + String(serverVal));
      }
  }

  // puzzle_reset_time 서버 수신
  int serverResetSec = my["puzzle_reset_time"].as<int>();
  if (serverResetSec != 0 && (forceAnswerUpdate || serverResetSec != (int)(cur["puzzle_reset_time"] | 0))) {
      puzzleResetTime = (unsigned long)serverResetSec;
      Serial.println("puzzle_reset_time 서버 수신: " + String(serverResetSec) + "ms");
  }
  
  // brightness 서버 수신 및 적용
  int serverBrightness = my["brightness"].as<int>();
  int prevBrightness = cur["brightness"].as<int>();
  if (serverBrightness != prevBrightness) {
      UpdateBrightness();
  }

  cur = my; // cur 데이터 그룹에 현재 읽어온 데이터 저장
}
void WaitFunc(void)
{

}
void SettingFunc(void)
{
    sendCommand("page pgWait");
    Serial.println("SETTING");
    UpdateBrightness();
    AllNeoOn(WHITE);
    BoxOpen();
    encoderValue = 165;
    answerCnt = 0;
    ptrCurrentMode = WaitFunc;
    ptrRfidMode = WaitFunc;
    itemBoxSelfOpen = false;    //퍼즐함수 성공했는지 확인하는 변수초기화
    itemBoxUsed = false;        //박스 사용했는지 확인하는 변수 초기화
    BlinkTimer.deleteTimer(blinkTimerId);
    GameTimer.deleteTimer(gameTimerId);
    ledcWrite(VIBRATION_RANGE_PIN, 0);
}
void ActivateFunc(void)
{
    sendCommand("page pgWait");
    encoderValue = 165;
    answerCnt = 0;
    Serial.println("ACTIVATE");
    UpdateBrightness();
    AllNeoOn(YELLOW);
    BoxClose();
    ptrCurrentMode = RfidLoopOutter;
    ptrRfidMode = StartPuzzle;
    itemBoxSelfOpen = false;    //퍼즐함수 성공했는지 확인하는 변수초기화
    itemBoxUsed = false;        //박스 사용했는지 확인하는 변수 초기화
    BlinkTimer.deleteTimer(blinkTimerId);
    GameTimer.deleteTimer(gameTimerId);
    ledcWrite(VIBRATION_RANGE_PIN, 0);
}
void ReadyFunc(void)
{
    sendCommand("page pgWait");
    Serial.println("READY");
    UpdateBrightness();
    AllNeoOn(RED);
    BoxClose();
    ptrCurrentMode = WaitFunc;
    ptrRfidMode = WaitFunc;
    itemBoxSelfOpen = false;    //퍼즐함수 성공했는지 확인하는 변수초기화
    itemBoxUsed = false;        //박스 사용했는지 확인하는 변수 초기화
    BlinkTimer.deleteTimer(blinkTimerId);
    GameTimer.deleteTimer(gameTimerId);
    ledcWrite(VIBRATION_RANGE_PIN, 0);
}