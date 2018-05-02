/*
 * SlotPlayer.cpp
 *
 *  Created on: 2 мая 2018 г.
 *      Author: Kreyl
 */

#include "SlotPlayer.h"
#include "kl_lib.h"
#include "MsgQ.h"
#include "shell.h"
#include "kl_fs_utils.h"
#include "audiomixer.h"

thread_reference_t ISndThd;

enum SndCmd_t {sndcmdNone, sndcmdStart, sndCmdVolume, sndcmdStop};

union SndMsg_t {
    uint32_t DWord[3];
    struct {
        SndCmd_t Cmd;
        uint8_t Slot;
        uint16_t Volume;
        const char* Emo;
        bool Repeat;
    } __packed;
    SndMsg_t& operator = (const SndMsg_t &Right) {
        DWord[0] = Right.DWord[0];
        DWord[1] = Right.DWord[1];
        DWord[2] = Right.DWord[2];
        return *this;
    }
    SndMsg_t() : Cmd(sndcmdNone) {}
    SndMsg_t(SndCmd_t ACmd, uint8_t ASlot) : Cmd(ACmd), Slot(ASlot) {}
    SndMsg_t(SndCmd_t ACmd, uint8_t ASlot, uint16_t AVolume) : Cmd(ACmd), Slot(ASlot), Volume(AVolume) {}
    SndMsg_t(SndCmd_t ACmd, uint8_t ASlot, uint16_t AVolume, const char* AEmo, bool ARepeat) :
        Cmd(ACmd), Slot(ASlot), Volume(AVolume), Emo(AEmo), Repeat(ARepeat) {}
} __packed;

static EvtMsgQ_t<SndMsg_t, MAIN_EVT_Q_LEN> MsgQSnd;

static char IFName[MAX_NAME_LEN];

// Callbacks. Returns true if OK
size_t TellCallback(void *file_context);
bool SeekCallback(void *file_context, size_t offset);
size_t ReadCallback(void *file_context, uint8_t *buffer, size_t length);

static AudioMixer mixer {TellCallback, SeekCallback, ReadCallback, 44100, 2};

#if 1 // ============================ Slot =====================================
class Slot_t {
private:
    uint8_t Indx;
    FIL IFile;
public:
    void Init(uint8_t AIndx) { Indx = AIndx; }
    void Start(uint16_t Volume, bool ARepeat) {
        if(TryOpenFileRead(IFName, &IFile) == retvOk) {
            mixer.start(
                    Indx, &IFile,
                    (ARepeat? AudioMixer::Mode::Continuous : AudioMixer::Mode::Single),
                    true, Volume);
        }
    }
    void SetVolume(uint16_t Volume) {
        mixer.fade(Indx, Volume);
    }
    void Stop() {
        mixer.stop(Indx);
        CloseFile(&IFile);
    }
};
static Slot_t Slot[AudioMixer::TRACKS];
#endif


uint8_t EmoToFName(const char* Emo) {
    strcpy(IFName, Emo);
    Printf("%S: %S\r", __FUNCTION__, IFName);
    return retvOk;
}


#if 1 // ============================== Thread =================================
static THD_WORKING_AREA(waSndThread, 1024);
__noreturn
static void SoundThread(void *arg) {
    chRegSetThreadName("Sound");
    while(true) {
        chThdSleepMilliseconds(450);
        SndMsg_t Msg = MsgQSnd.Fetch(TIME_INFINITE);
        switch(Msg.Cmd) {
            case sndcmdStart:
                Printf("sndcmdStart %u\r", Msg.Slot);
                if(EmoToFName(Msg.Emo) == retvOk) {
                    Slot[Msg.Slot].Start(Msg.Volume, Msg.Repeat);
                }
                break;

            case sndCmdVolume:
                Printf("sndCmdVolume %u\r", Msg.Slot);
                Slot[Msg.Slot].SetVolume(Msg.Volume);
                break;

            case sndcmdStop:
                Printf("sndcmdStop %u\r", Msg.Slot);
                Slot[Msg.Slot].Stop();
                break;

            case sndcmdNone: break;
        } // switch
    } // while true
}
#endif

#if 1 // ========================= Interface ===================================
namespace SlotPlayer {
void Init() {
    MsgQSnd.Init();
    for(uint32_t i=0; i<AudioMixer::TRACKS; i++) Slot[i].Init(i);
    ISndThd = chThdCreateStatic(waSndThread, sizeof(waSndThread), NORMALPRIO, SoundThread, NULL);
}

void Start(uint8_t SlotN, uint16_t Volume, const char* Emo, bool Repeat) {
    if(SlotN >= AudioMixer::TRACKS) return;
    MsgQSnd.SendNowOrExit(SndMsg_t(sndcmdStart, SlotN, Volume, Emo, Repeat));
}

void SetVolume(uint8_t SlotN, uint16_t Volume) {
    if(SlotN >= AudioMixer::TRACKS) return;
    MsgQSnd.SendNowOrExit(SndMsg_t(sndCmdVolume, SlotN, Volume));
}

void Stop(uint8_t SlotN) {
    if(SlotN >= AudioMixer::TRACKS) return;
    MsgQSnd.SendNowOrExit(SndMsg_t(sndcmdStop, SlotN));
}
}
#endif
