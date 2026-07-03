void TimerInit()
{
    wifiTimerId = WifiTimer.setInterval(wifiTime,WifiIntervalFunc);
    gameTimerId = GameTimer.setInterval(puzzleResetTime,GameTimerFunc);
    blinkTimerId = BlinkTimer.setInterval(blinkTime,BlinkTimerFunc);
    GameTimer.deleteTimer(gameTimerId);
    BlinkTimer.deleteTimer(blinkTimerId); 
}

void WifiIntervalFunc()
{
    if (puzzleMode) {
        // 퍼즐 중: 인터럽트 잠깐 해제 → WiFi 수신 → 엔코더 값 복원 → 인터럽트 재연결
        long savedEncoder = encoderValue;
        detachInterrupt(encoderPinA);
        detachInterrupt(encoderPinB);
        has2wifi.Loop(DataChanged);
        encoderValue = savedEncoder;
        attachInterrupt(encoderPinA, updateEncoder, CHANGE);
        attachInterrupt(encoderPinB, updateEncoder, CHANGE);
    } else {
        has2wifi.Loop(DataChanged);
    }
}
void GameTimerFunc(){
    puzzleMode = false;
    ActivateFunc();
    ledcWrite(VIBRATION_RANGE_PIN, 0);
    GameTimer.deleteTimer(gameTimerId);
    WifiTimer.deleteTimer(wifiTimerId);
    wifiTimerId = WifiTimer.setInterval(wifiTime, WifiIntervalFunc);
    detachInterrupt(encoderPinA);
    detachInterrupt(encoderPinB);
    encoderValue = 165;
}

void BlinkTimerFunc(){
    Serial.println("Blink!");
    if(blinkOn == true){
        lightColor(pixels[blinkNeo], color[blinkColor]);
        blinkOn = false;
    }
    else{
        lightColor(pixels[blinkNeo], color[BLACK]);
        blinkOn = true;
    }

}
void BlinkTimerStart(int Neo, int NeoColor){
    blinkNeo = Neo;
    blinkColor = NeoColor;
    blinkTimerId = BlinkTimer.setInterval(blinkTime,BlinkTimerFunc);
}