/**
 * @file Done_ItemBox_code.ino
 * @author 김병준 (you@domain.com)
 * @brief
 * @version 1.0
 * @date 2022-11-29
 *
 * @copyright Copyright (c) 2022
 *
 */

#define FIRMWARE_VER 10
#include "Train_HAS3_itembox.h"
#include "esp_system.h"

void setup()
{
    Serial.begin(115200);

    has2wifi.Setup("badland_shoot", "Code3824@");
    TelnetInit();
    Serial.println("MAC: " + WiFi.macAddress());
    ota.setLogStream(Serial);
    ota.setOnSuccess([]() {
        for (int i = 0; i < 5; i++) {
            AllNeoOn(RED);
            delay(300);
            AllNeoOn(BLACK);
            delay(300);
        }
        has2wifi.Send((String)(const char*)my["device_name"], "device_state", "setting");
    });
    ota.setOnSkip([]() {
        has2wifi.Send((String)(const char*)my["device_name"], "device_state", "setting");
    });
    NeopixelInit();
    RfidInit();
    MotorInit();
    EncoderInit();
    NextionInit();
    TimerInit();
    // has2wifi.Setup();
    // has2wifi.Setup("KT_GiGA_6C64","ed46zx1198");
    DataChanged();
    ActivateFunc();
}
void loop()
{
    if (boxMotorRunning) {
        if (boxClosing) {
            if (digitalRead(BOXSWITCH_PIN) == HIGH) {  // 닫힘 감지 → 모터 정지
                MotorStop();
                boxMotorRunning = false;
                Serial.println("BOX Closed");
            }
        } else {
            if (millis() - motorStartTime >= motorOpenDuration) {  // 타이머 만료 → 모터 정지
                MotorStop();
                boxMotorRunning = false;
                Serial.println("BOX Opened");
                if (pendingOpenScreen) {
                    BatteryPackSend();
                    sendCommand("page pgItemOpen");
                    SendLanguage();
                    ExpSend();
                    if ((String)(const char*)shift_machine["selected_language"] != "EN")
                        sendCommand("wQuizSolved.en=1");
                    else
                        sendCommand("wEQuizSolved.en=1");
                    pendingOpenScreen = false;
                }
            }
        }
    }
    TelnetRun();
    ptrCurrentMode();
    WifiTimer.run();
    GameTimer.run();
    BlinkTimer.run();
}
