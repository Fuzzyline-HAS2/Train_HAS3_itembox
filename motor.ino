void MotorInit()
{
    //Linear Motor Init
    pinMode(BOXSWITCH_PIN, INPUT);  // GPIO36은 input-only 핀 - 외부 풀업 저항 필요
    pinMode(MOTOR_INA1_PIN, OUTPUT);
    pinMode(MOTOR_INA2_PIN, OUTPUT);
    ledcAttach(MOTOR_PWMA_PIN, MotorFreq, MotorResolution);
    ledcWrite(MOTOR_PWMA_PIN, 0);
    BoxClose();

    //Vibration Motor Init
    pinMode(VIBRATION_ANSWER_PIN, OUTPUT);
    digitalWrite(VIBRATION_ANSWER_PIN, HIGH);
    ledcAttach(VIBRATION_RANGE_PIN, MotorFreq, MotorResolution);
    ledcWrite(VIBRATION_RANGE_PIN, 0);
}
void BoxClose()
{
    if (digitalRead(BOXSWITCH_PIN) == HIGH) { Serial.println("BOX already closed"); return; } // HIGH = 닫힘 확인, 이미 닫혀있으면 스킵
    Serial.println("BOX Close");
    pinMode(MOTOR_INA1_PIN, OUTPUT);
    pinMode(MOTOR_INA2_PIN, OUTPUT);
    ledcAttach(MOTOR_PWMA_PIN, MotorFreq, MotorResolution);
    ledcWrite(MOTOR_PWMA_PIN, MotorMAX_DUTY_CYCLE - 1);
    digitalWrite(MOTOR_INA1_PIN, LOW);
    digitalWrite(MOTOR_INA2_PIN, HIGH);
    motorStartTime = millis();
    boxMotorRunning = true;
    boxClosing = true;
}

void BoxOpen()
{
    // 스위치 상태 확인 없이 무조건 모터 구동 (타이머로 정지)
    Serial.println("BOX Open");
    pinMode(MOTOR_INA1_PIN, OUTPUT);
    pinMode(MOTOR_INA2_PIN, OUTPUT);
    ledcAttach(MOTOR_PWMA_PIN, MotorFreq, MotorResolution);
    ledcWrite(MOTOR_PWMA_PIN, MotorMAX_DUTY_CYCLE - 1);
    digitalWrite(MOTOR_INA1_PIN, HIGH);
    digitalWrite(MOTOR_INA2_PIN, LOW);
    motorStartTime = millis();
    boxMotorRunning = true;
    boxClosing = false;
}

void MotorStop()
{
    Serial.println("모터 스탑");
    digitalWrite(MOTOR_INA1_PIN, LOW);
    digitalWrite(MOTOR_INA2_PIN, LOW);
    // ledcWrite(0) 제거 — LEDC 채널 연결 유지 (0으로 쓰면 채널이 분리됨)
}

void EncoderVibrationStrength(int answer)
{
    int differenceValue = abs(answer - (encoderValue/4));
    int answerRange = modeValue[RANGE][1];
    int vibeRange = modeValue[RANGE][2];
    int vibeStrength = 0;
    if(differenceValue < answerRange + vibeRange * 0)       vibeStrength = 0;
    else if(differenceValue < answerRange + vibeRange * 1)  vibeStrength = 1;
    else if(differenceValue < answerRange + vibeRange * 2)  vibeStrength = 2;
    else if(differenceValue < answerRange + vibeRange * 3)  vibeStrength = 3;
    else vibeStrength = 4;
    ledcWrite(VIBRATION_RANGE_PIN, modeValue[VIBESTREGNTH][vibeStrength]);
}
