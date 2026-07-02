/**
 * @brief 현재 풀고있는 문제에서 엔코더 값의 변화에 따라 네오픽셀 진동모터 정답카운팅을 관장하는 함수
 */
void Puzzle(void)
{
    int currentAnswer = modeValue[ANSWER][answerCnt];
    EncoderNeopixelOn();
    EncoderVibrationStrength(currentAnswer);

    // 외부 RFID 태그 유지 확인 + 감지 시 리셋 타이머 갱신 (100ms마다)
    // readPassiveTargetID는 InList 응답을 끝까지 읽고 타겟 수까지 확인함.
    // startPassiveTargetIDDetection은 응답을 안 읽어 칩 상태가 꼬이고,
    // setPassiveActivationRetries(1) 적용 시 카드가 없어도 true를 반환해 이탈 감지가 깨짐.
    static unsigned long lastRfidPollTime = 0;
    if (millis() - lastRfidPollTime >= 100) {
        lastRfidPollTime = millis();
        uint8_t uid[7];
        uint8_t uidLength;
        if (nfc[OUTPN532].readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 150)) {
            rfidLastSeenTime = millis();
            GameTimer.deleteTimer(gameTimerId);
            gameTimerId = GameTimer.setInterval(puzzleResetTime, GameTimerFunc);
        }
    }
    if (millis() - rfidLastSeenTime > rfidPuzzleTimeout) {
        Serial.println("Puzzle Paused: RFID 태그 없음");
        puzzleMode = false;
        WifiTimer.deleteTimer(wifiTimerId);
        wifiTimerId = WifiTimer.setInterval(wifiTime, WifiIntervalFunc); // 퍼즐 중단 시 WiFi 재개
        ledcWrite(VIBRATION_RANGE_PIN, 0);
        AllNeoOn(YELLOW);
        detachInterrupt(encoderPinA);
        detachInterrupt(encoderPinB);
        ptrCurrentMode = RfidLoopOutter;
        ptrRfidMode = ResumePuzzle;
        return;
    }

    if (currentAnswer == -1 && (String)(const char*)my["game_state"] == "activate") {
        Serial.println("Puzzle " + String(answerCnt + 1) + " server-solved (-1), opening box");
        sendCommand("wQuizSolved.en=1");
        ledcWrite(VIBRATION_RANGE_PIN, 0);
        answerCnt = 0;
        detachInterrupt(encoderPinA);
        detachInterrupt(encoderPinB);
        puzzleMode = false;
        WifiTimer.deleteTimer(wifiTimerId);
        wifiTimerId = WifiTimer.setInterval(wifiTime, WifiIntervalFunc);
        PuzzleSolved();
        return;
    }

    if (digitalRead(buttonPin) == LOW)                                                                      // 엔코더 스위치 눌렸을때
    {
        volatile long currentEncoderValue = encoderValue;                                                   // EnocoderRead 함수에서 엔코더값을 저장한 전역변수 encoderValue 복사
        long differenceValue = (abs(currentAnswer - (encoderValue / 4))) / modeValue[RANGE][ANSWER_RANGE];  // 정답 범위에서 현재 엔코더 갑이 얼마나 차이나는지 확인하는 변수
        if (differenceValue == 0)               // 정답일때
        {
            Serial.println("Correct Answer");
            NeoBlink(ENCODER, GREEN, 5, 250);   // 엔코더 네오픽셀 녹색 0.25s 간격으로 5번 점멸 -> Delay사용으로 이 함수에 2.5초 머물러 있음
            rfidLastSeenTime = millis();        // 블링크(2.5s) > rfidPuzzleTimeout(2s)이므로 반드시 블링크 "후"에 갱신해야 오탐 방지됨
            answerCnt++;                        // 정답시 다음 문제로 넘어가기 위해 카운트 +1

            bool nextIsTerminator = (answerCnt < modeValue[RANGE][ANSWER_CNT]) && (modeValue[ANSWER][answerCnt] == -1);
            if (answerCnt >= modeValue[RANGE][ANSWER_CNT] || nextIsTerminator)    // 모든 정답을 맞추었거나 다음 정답이 -1이면
            {
                Serial.println("QUIZ SUCCEED");
                sendCommand("wQuizSolved.en=1");                                    // Nextion으로 "해제 완료" 나레이션 출력 명령 전송
                ledcWrite(VIBRATION_RANGE_PIN, 0);                                  // 진동모터 끄기
                answerCnt = 0;
                detachInterrupt(encoderPinA);
                detachInterrupt(encoderPinB);
                puzzleMode = false;
                WifiTimer.deleteTimer(wifiTimerId);
                wifiTimerId = WifiTimer.setInterval(wifiTime, WifiIntervalFunc);
                PuzzleSolved();                                                     // 추가 태그 없이 바로 아이템박스 열기
            }
        }
        else                                    // 틀렸을때
        {
            Serial.println("Wrong Answer");
            NeoBlink(ENCODER, RED, 5, 250);     //엔코더 네오픽셀 적색 0.25s 간격으로 5번 점멸 -> Delay사용으로 이 함수에 2.5초 머물러 있음
            rfidLastSeenTime = millis();        // 블링크(2.5s) > rfidPuzzleTimeout(2s)이므로 반드시 블링크 "후"에 갱신해야 오탐 방지됨
        }
        encoderValue = currentEncoderValue;     // 네오픽셀 점멸 시 마지막으로 저장된 엔코더 값 저장해서 현재 엔코더 값이 바뀌어도 되돌아가게 하는 변수 저장
    }
}
