/*
 * File:   main.cpp
 * Author: Elessar
 * Project: MasonOrder
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

// Forever
EvtMsgQ_t<EvtMsg_t, MAIN_EVT_Q_LEN> EvtQMain;
extern CmdUart_t Uart;
//void OnCmd(Shell_t *PShell);
void ITask();

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
    Uart.Init(115200);
    Printf("\r%S %S\r\n", APP_NAME, BUILD_TIME);
    Clk.PrintFreqs();

    Radio.Init();
    // ==== Main cycle ====
    ITask();
}

__noreturn
void ITask() {
    while(true) {
        chThdSleepMilliseconds(205);
    } // while true
}

