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

// Forever
EvtMsgQ_t<EvtMsg_t, MAIN_EVT_Q_LEN> EvtQMain;
extern CmdUart_t Uart;
void OnCmd(Shell_t *PShell);
void ITask();

uint8_t Status;
uint16_t ID = 2;

Beeper_t Beeper(BEEPER_PIN);
Vibro_t Vibro(VIBRO_PIN);

#if 1 // =========================== Locals ====================================

#endif

int main() {
    // ==== Setup clock ====
    Clk.SetHiPerfMode();

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
    Lcd.Cls(clGreen);

//    SD.Init();
//    Printf("ID = %u\r", ID);

//    DrawBmpFile(0, 0, "Splash.bmp", &CommonFile);

//    SimpleSensors::Init();
//    Beeper.Init();
//    Beeper.StartOrRestart(bsqBeepBeep);
//    Vibro.Init(VIBRO_TIM_FREQ);
//    Vibro.StartOrRestart(vsqBrrBrr);

//    Sound.Init();
//    Sound.SetVolume(254);


//    Radio.Init();
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

            case evtIdBtnA:
            case evtIdBtnB:
            case evtIdBtnC:
            case evtIdBtnL:
            case evtIdBtnE:
            case evtIdBtnR:
            case evtIdBtnX:
            case evtIdBtnY:
            case evtIdBtnZ:
                Printf("Btn: %u\r", (Msg.ID - evtIdBtnA));
                break;

            default: break;
        } // switch
    } // while true
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

    else PShell->Ack(retvCmdUnknown);
}
#endif

