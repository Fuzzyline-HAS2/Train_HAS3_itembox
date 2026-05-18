/**
 * @brief 현재 풀고있는 문제에서 엔코더 값의 변화에 따라 네오픽셀 진동모터 정답카운팅을 관장하는 함수
 */
void Puzzle(void)
{
    // 외부 RFID 태그 유지 확인 + 감지 시 리셋 타이머 갱신
    {
        byte pn532_buf[64] = {0};
        if (nfc[OUTPN532].sendCommandCheckAck(pn532_buf, 1)) {
            if (nfc[OUTPN532].startPassiveTargetIDDetection(PN532_MIFARE_ISO14443A)) {
                rfidLastSeenTime = millis();
                GameTimer.deleteTimer(gameTimerId);
                gameTimerId = GameTimer.setInterval(puzzleResetTime, GameTimerFunc);
            }
        }
    }
    if (millis() - rfidLastSeenTime > rfidPuzzleTimeout) {
        Serial.println("Puzzle Paused: RFID 태그 없음");
        puzzleMode = false;
        ledcWrite(VIBRATION_RANGE_PIN, 0);
        AllNeoOn(YELLOW);
        detachInterrupt(encoderPinA);
        detachInterrupt(encoderPinB);
        ptrCurrentMode = RfidLoopOutter;
        ptrRfidMode = ResumePuzzle;
        return;
    }

    int currentAnswer = modeValue[ANSWER][answerCnt];   // Puzzle 함수를 진행하는 동안 현재의 정답 저장용 변수, 몇번째 문제인지 저장하는건 answerCnt 전연 변수

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

    EncoderNeopixelOn();                                // 현재 엔코더 위치 적색으로 표현하기 위해 네오픽셀 켜주는 함수
    EncoderVibrationStrength(currentAnswer);            // 현재 엔코더 위치에 따라 진동모터 세기 결정해주는 함수

    if (digitalRead(buttonPin) == LOW)                                                                      // 엔코더 스위치 눌렸을때
    {
        volatile long currentEncoderValue = encoderValue;                                                   // EnocoderRead 함수에서 엔코더값을 저장한 전역변수 encoderValue 복사
        long differenceValue = (abs(currentAnswer - (encoderValue / 4))) / modeValue[RANGE][ANSWER_RANGE];  // 정답 범위에서 현재 엔코더 갑이 얼마나 차이나는지 확인하는 변수
        if (differenceValue == 0)               // 정답일때
        {
            Serial.println("Correct Answer");
            rfidLastSeenTime = millis();        // NeoBlink(2.5초 블로킹) 전 타임스탬프 갱신 - 오탐 방지
            NeoBlink(ENCODER, GREEN, 5, 250);   // 엔코더 네오픽셀 적색 0.25s 간격으로 5번 점멸 -> Delay사용으로 이 함수에 2초 머물러 있음
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
            rfidLastSeenTime = millis();        // NeoBlink(2.5초 블로킹) 전 타임스탬프 갱신 - 오탐 방지
            NeoBlink(ENCODER, RED, 5, 250);     //엔코더 네오픽셀 적색 0.25s 간격으로 5번 점멸 -> Delay사용으로 이 함수에 2초 머물러 있음
        }
        encoderValue = currentEncoderValue;     // 네오픽셀 점멸 시 마지막으로 저장된 엔코더 값 저장해서 현재 엔코더 값이 바뀌어도 되돌아가게 하는 변수 저장
    }
}
