void NextionInit()
{
   nexInit();
   nexHwSerial.begin(9600, SERIAL_8N1, NEXTIONHWSERIAL_RX_PIN, NEXTIONHWSERIAL_TX_PIN);
}

void SendLanguage()
{
    int lang = ((String)(const char*)shift_machine["selected_language"] == "EN") ? 0 : 1;
    const char* pages[] = {"pgSetting", "pgItemOpen", "pgItemTaken", "pgItemFail",
                           "pgEscapeOpen", "pgSurvivorWin", "pgSurvivorLose"};
    for (int i = 0; i < 7; i++) {
        String cmd = String(pages[i]) + ".vLanguage.val=" + String(lang);
        sendCommand(cmd.c_str());
    }
}

void DisplayCheck()
{
 while (nexHwSerial.available() > 0)
 {
   String nextion_string = nexHwSerial.readStringUntil(' ');
   NextionReceived(&nextion_string);
 }
}

void NextionReceived(String *nextion_string)
{
 if (*nextion_string == "test")
 {
   sendCommand("page pgItemTaken");
   sendCommand("wQuizSolved.en=1");
 }
}

void ExpSend(){
    if((String)(const char*)my["exp_pack"] == "10")
    {
        sendCommand("pgItemOpen.picExp.pic=0");
        sendCommand("pgItemOpen.vExp.val=10");
    }
    else if((String)(const char*)my["exp_pack"] == "30")
    {
        sendCommand("pgItemOpen.picExp.pic=1");
        sendCommand("pgItemOpen.vExp.val=30");
    }
    else if((String)(const char*)my["exp_pack"] == "50")
    {
        sendCommand("pgItemOpen.picExp.pic=2");
        sendCommand("pgItemOpen.vExp.val=50");
    }
}
void BatteryPackSend(){
    // 훈련소 시나리오: 배터리 2개 고정
    sendCommand("picBatteryPack.pic=4");
    sendCommand("pgItemOpen.vBatteryPack.val=2");
}
