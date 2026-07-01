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

#define FIRMWARE_VER 22
#define PARTITION_VER 1
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
    ota.setPartitionUpdate(
        "https://raw.githubusercontent.com/Fuzzyline-HAS2/Train_HAS3_itembox/third_store/partitions.bin",
        "https://raw.githubusercontent.com/Fuzzyline-HAS2/Train_HAS3_itembox/third_store/partitions.sig",
        "https://raw.githubusercontent.com/Fuzzyline-HAS2/Train_HAS3_itembox/third_store/partition_version.txt",
        PARTITION_VER
    );
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
                    // 화면 전환을 끝낸 뒤 서버 보고. 모터 정지·안정화 이후라 WiFi 전류 피크가 모터와 겹치지 않음.
                    if (!itemBoxUsed)
                        has2wifi.Send((String)(const char*)my["device_name"], "device_state", "open");
                    if (!itemBoxUsed) {
                        ptrCurrentMode = RfidLoopInner;
                        ptrRfidMode = ItemTook;
                    }
                }
                // 서버 open 경로: 모터 정지 후 전원 안정화 대기 → 내부 태그 활성화.
                if (pendingInnerEnable) {
                    delay(motorSettleDelay);
                    pendingInnerEnable = false;
                    if (!itemBoxUsed) {
                        ptrCurrentMode = RfidLoopInner;
                        ptrRfidMode = ItemTook;
                    }
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
