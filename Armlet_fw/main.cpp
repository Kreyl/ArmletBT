/*
 * File:   main.cpp
 *
 * Created on May 27, 2016, 6:37 PM
 */

#include "hal.h"
#include "MsgQ.h"
#include "kl_lib.h"
#include "led.h"
#include "Sequences.h"
#include "shell.h"
#include "radio_lvl1.h"
#include "lcd2630.h"
#include "kl_sd.h"
#include "DrawBmp.h"
#include "kl_fs_utils.h"
#include "beeper.h"
#include "vibro.h"
#include "sound.h"
#include "kl_adc.h"
#include "pill.h"
#include "pill_mgr.h"
#include "dispatcher.h"

#if 1 // =============== Low level ================
// Forever
EvtMsgQ_t<EvtMsg_t, MAIN_EVT_Q_LEN> EvtQMain;
extern CmdUart_t Uart;
void OnCmd(Shell_t *PShell);
void ITask();

uint8_t Status;
uint16_t ID = 7;
uint8_t Influence = ID + 128;

Beeper_t Beeper(BEEPER_PIN);
Vibro_t Vibro(VIBRO_PIN);

static TmrKL_t TmrOneSecond {MS2ST(999), evtIdEverySecond, tktPeriodic}; // Measure battery periodically

class Power_t {
public:
    bool ExternalPwrOn() { return  PinIsHi(USB_DETECT_PIN); }
    bool IsCharging()    { return !PinIsHi(IS_CHARGING_PIN); }
    void Init() {
        // Battery
        PinSetupAnalog(BAT_MEAS_PIN);
        Adc.Init();
        // Charging
        PinSetupInput(IS_CHARGING_PIN, pudPullUp);
    }
};
static Power_t Power;
#endif

#if 1 // ===================== Logic ======================
// Callbacks. Returns true if OK
size_t TellCallback(void *file_context);
bool SeekCallback(void *file_context, size_t offset);
size_t ReadCallback(void *file_context, uint8_t *buffer, size_t length);



Dispatcher dispatcher;
CsvTable csvTable {ReadCallback};
EmotionTable emoTable;
InfluenceTable infTable;
CharacterTable charTable;
LocalCharacter localChar;

#endif

int main() {
#if 1 // Low level init
    // ==== Setup clock ====
    Clk.SetCoreClk(cclk24MHz);
//    Clk.SetCoreClk(cclk48MHz);

    // ==== Init OS ====
    halInit();
    chSysInit();
    Clk.UpdateFreqValues(); // Do it after halInit to update system timer using correct prescaler

    // ==== Init Hard & Soft ====
    EvtQMain.Init();
    Uart.Init(115200);
    Printf("\r%S %S\r\n", APP_NAME, BUILD_TIME);
    Clk.PrintFreqs();

    Lcd.Init();
    SD.Init();
//    Printf("ID = %u\r", ID);
    DrawBmpFile(0, 0, "Splash.bmp", &CommonFile);

    SimpleSensors::Init();
    Power.Init();

    i2c2.Init();
    PillMgr.Init();

//    Beeper.Init();
//    Beeper.StartOrRestart(bsqBeepBeep);
//    Vibro.Init(VIBRO_TIM_FREQ);
//    Vibro.StartOrRestart(vsqBrrBrr);

//    Sound.Init();

    TmrOneSecond.StartOrRestart();

    Radio.Init();
#endif

#if 0 // ==== Logic init ====
    // Open Emotions
    if(TryOpenFileRead("Emotions.csv", &CommonFile) == retvOk) {
        emoTable.init(&CommonFile, &csvTable);
        CloseFile(&CommonFile);
    }
    else chSysHalt("No Emotions");

    // Open Influence
    if(TryOpenFileRead("Reasons.csv", &CommonFile) == retvOk) {
        infTable.init(&CommonFile, &csvTable, &emoTable);
        CloseFile(&CommonFile);
    }
    else chSysHalt("No Reasons");

    // Get Person name
    char CharName[] = "mr First"; // XXX

    // Character table
    if(TryOpenFileRead("Characters.csv", &CommonFile) == retvOk) {
        charTable.init(&CommonFile, &csvTable, CharName, &localChar);
        CloseFile(&CommonFile);
    }
    else chSysHalt("No Characters");

    // === Load localChar ===
    // Load State: Dogan, Dead, Corrupted
    // Load KatetLinks
    // Load counters

    dispatcher.init(&infTable, &emoTable, &charTable, &localChar);
#endif
    // ==== Main cycle ====
    ITask();
}

__noreturn
void ITask() {
    while(true) {
        EvtMsg_t Msg = EvtQMain.Fetch(TIME_INFINITE);
        switch(Msg.ID) {
            case evtIdShellCmd:
                OnCmd((Shell_t*)Msg.Ptr);
                ((Shell_t*)Msg.Ptr)->SignalCmdProcessed();
                break;

            case evtIdButtons:
                Printf("Btn: %u %u\r", Msg.BtnEvtInfo.BtnID, Msg.BtnEvtInfo.Type);
                dispatcher.handle_button(Msg.BtnEvtInfo.BtnID, (Msg.BtnEvtInfo.Type == beLongPress));
                break;

            case evtIdEverySecond:
//                Printf("Second\r");
                break;

            case evtIdAdcRslt:
//                Printf("Adc: %u; ExtPwr: %u; Charging: %u\r", Msg.Value, Power.ExternalPwrOn(), Power.IsCharging());
                // TODO: send to statemachine
                break;

            case evtIdNewRPkt:
                Printf("RPkt: Inf=%u; Par=%u; RSSI=%d\r", Msg.b[0], Msg.b[1], (int8_t)Msg.b[2]);
                dispatcher.handle_radio_packet(Msg.b[0], Msg.b[1], (int8_t)Msg.b[2]);
                break;

#if 1 // ======= Pill ======
            case evtIdPillConnected:
                Printf("Pill: %d\r", ((Pill_t*)Msg.Ptr)->TypeInt32);
                dispatcher.handle_nfc_packet((uint8_t)((Pill_t*)Msg.Ptr)->TypeInt32);
                break;
            case evtIdPillDisconnected:
                Printf("Pill Discon\r");
                break;
#endif

#if 1 // ======= USB =======
            case evtIdUsbConnect:
                Printf("USB connect\r");
                break;
            case evtIdUsbDisconnect:
                Printf("USB disconnect\r");
//                StateMachine(eventDisconnect);
                break;
#endif

            default: break;
        } // switch
    } // while true
}

void ProcessUsbDetect(PinSnsState_t *PState, uint32_t Len) {
    EvtMsg_t Msg;
    if(*PState == pssRising) Msg.ID = evtIdUsbConnect;
    else if(*PState == pssFalling) Msg.ID = evtIdUsbDisconnect;
    EvtQMain.SendNowOrExit(Msg);
}

#if 1 // ======================= Command processing ============================
void OnCmd(Shell_t *PShell) {
    Cmd_t *PCmd = &PShell->Cmd;
//    __unused int32_t dw32 = 0;  // May be unused in some configurations
//    Printf("%S  ", PCmd->Name);
    // Handle command
    if(PCmd->NameIs("Ping")) PShell->Ack(retvOk);
    else if(PCmd->NameIs("Version")) PShell->Printf("%S %S\r", APP_NAME, BUILD_TIME);

//    else if(PCmd->NameIs("GetBat")) { PShell->Printf("Battery: %u\r", Audio.GetBatteryVmv()); }

    else if(PCmd->NameIs("R")) Lcd.Cls(clRed);
    else if(PCmd->NameIs("G")) Lcd.Cls(clGreen);
    else if(PCmd->NameIs("B")) Lcd.Cls(clBlue);
    else if(PCmd->NameIs("p")) DrawBmpFile(0, 0, "Splash.bmp", &CommonFile);
    else if(PCmd->NameIs("S")) Lcd.SetBounds(0,160, 0, 128);


    else PShell->Ack(retvCmdUnknown);
}
#endif

#if 1 // ========================== Callbacks ==================================
size_t TellCallback(void *file_context) {
    FIL *pFile = (FIL*)file_context;
    return pFile->fptr;
}

bool SeekCallback(void *file_context, size_t offset) {
    FIL *pFile = (FIL*)file_context;
    FRESULT rslt = f_lseek(pFile, offset);
    if(rslt == FR_OK) return true;
    else {
        Printf("SeekErr %u\r", rslt);
        return false;
    }
}

size_t ReadCallback(void *file_context, uint8_t *buffer, size_t length) {
    FIL *pFile = (FIL*)file_context;
    uint32_t ReadSz=0;
    FRESULT rslt = f_read(pFile, buffer, length, &ReadSz);
    if(rslt == FR_OK) {
        return ReadSz;
    }
    else {
//        Printf("ReadErr %u\r", rslt);
        return 0;
    }
}
#endif
