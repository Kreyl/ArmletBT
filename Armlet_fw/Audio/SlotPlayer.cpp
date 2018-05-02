/*
 * SlotPlayer.cpp
 *
 *  Created on: 2 мая 2018 г.
 *      Author: Kreyl
 */

#include "SlotPlayer.h"
#include "kl_lib.h"
#include "MsgQ.h"

thread_reference_t ISndThd;

enum SndCmd_t {sndcmdStart, sndCmdVolume, sndcmdStop};

union SndMsg_t {
    uint32_t DWord[2];
    struct {
        SndCmd_t Cmd;
        uint8_t Slot;
        uint8_t Volume;
        char* Emo;
    } __packed;
    EvtMsg_t& operator = (const EvtMsg_t &Right) {
        DWord[0] = Right.DWord[0];
        DWord[1] = Right.DWord[1];
        return *this;
    }
    SndMsg_t(SndCmd_t ACmd) : Cmd(ACmd) {}
    SndMsg_t(SndCmd_t ACmd, uint8_t ASlot, uint8_t AVolume) : Cmd(ACmd), Slot(ASlot), Volume(AVolume) {}
    SndMsg_t(SndCmd_t ACmd, uint8_t ASlot, uint8_t AVolume, char* AEmo) : Cmd(ACmd), Slot(ASlot), Volume(AVolume), Emo(AEmo) {}
} __packed;

static EvtMsgQ_t<SndMsg_t, MAIN_EVT_Q_LEN> MsgQSnd;

static THD_WORKING_AREA(waSndThread, 1024);
__noreturn
static void SoundThread(void *arg) {
    chRegSetThreadName("Sound");
    while(true) {
        chThdSleepMilliseconds(450);
        SndMsg_t Msg = MsgQSnd.Fetch(TIME_INFINITE);
        switch(Msg.Cmd) {
            case sndcmdStart:
                break;

            case sndCmdVolume:
                break;

            case sndcmdStop:
                break;
        } // switch
    } // while true
}

#if 1 // ========================= Interface ===================================
namespace SlotPlayer {
void Init() {
    MsgQSnd.Init();
    ISndThd = chThdCreateStatic(waSndThread, sizeof(waSndThread), NORMALPRIO, SoundThread, NULL);
}

void Start(uint8_t SlotN, uint8_t Volume, char* Emo) {
    MsgQSnd.SendNowOrExit(SndMsg_t(sndcmdStart, SlotN, Volume, Emo));
}

void SetVolume(uint8_t SlotN, uint8_t Volume) {
    MsgQSnd.SendNowOrExit(SndMsg_t(sndcmdStart, SlotN, Volume));
}

void Stop(uint8_t SlotN) {
    MsgQSnd.SendNowOrExit(SndMsg_t(sndcmdStart, SlotN));
}
}
#endif
