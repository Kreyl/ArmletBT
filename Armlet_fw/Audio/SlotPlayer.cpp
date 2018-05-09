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
#include "vs1011.h"

thread_reference_t ISndThd;
DirList_t DirList;

#define ALL_SLOTS   0xFF

enum SndCmd_t {sndcmdNone, sndcmdStart, sndCmdVolume, sndcmdStop, sndcmdPrepareNextBuf};

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
    SndMsg_t(SndCmd_t ACmd) : Cmd(ACmd) {}
    SndMsg_t(SndCmd_t ACmd, uint8_t ASlot) : Cmd(ACmd), Slot(ASlot) {}
    SndMsg_t(SndCmd_t ACmd, uint8_t ASlot, uint16_t AVolume) : Cmd(ACmd), Slot(ASlot), Volume(AVolume) {}
    SndMsg_t(SndCmd_t ACmd, uint8_t ASlot, uint16_t AVolume, const char* AEmo, bool ARepeat) :
        Cmd(ACmd), Slot(ASlot), Volume(AVolume), Emo(AEmo), Repeat(ARepeat) {}
} __packed;

static EvtMsgQ_t<SndMsg_t, MAIN_EVT_Q_LEN> MsgQSnd;

static char IFName[MAX_NAME_LEN]; // Static buffer for file names

// Callbacks. Returns true if OK
size_t TellCallback(void *file_context);
bool SeekCallback(void *file_context, size_t offset);
size_t ReadCallback(void *file_context, uint8_t *buffer, size_t length);
void TrackEndCallback(int SlotN);

static AudioMixer mixer {TellCallback, SeekCallback, ReadCallback, TrackEndCallback, 44100, 2};

// ==== Buffers ====
#define BUF_SZ_FRAME        1024UL
struct SndBuf_t {
    uint32_t Buf[BUF_SZ_FRAME], Sz;
};
static SndBuf_t Buf1, Buf2;
static volatile SndBuf_t *PNextBuf;

static void ReadToBuf(volatile SndBuf_t *PBuf) {
    PBuf->Sz = mixer.play((int16_t*)PBuf->Buf, BUF_SZ_FRAME); // returns how many frames was read
}

static void OnVsBufEnd() {
//    PrintfI("VsBufEnd %X %X\r", PNextBuf->Buf[5], PNextBuf->Buf[6]);
    VS.SetNextBuf((uint8_t*)PNextBuf->Buf, PNextBuf->Sz * 4);
    PNextBuf = (PNextBuf == &Buf1)? &Buf2 : &Buf1;  // Switch buffers
    MsgQSnd.SendNowOrExitI(SndMsg_t(sndcmdPrepareNextBuf));
}

#if 1 // ============================ Slot =====================================
class Slot_t {
private:
    uint8_t Indx;
    FIL IFile;
public:
    void Init(uint8_t AIndx) { Indx = AIndx; }
    void Start(uint16_t Volume, bool ARepeat) {
        Printf("%S: %S %u %u\r", __FUNCTION__, IFName, Volume, ARepeat);
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
    if(DirList.GetRandomFnameFromDir(Emo, IFName) == retvOk) {
//        Printf("%S: %S\r", __FUNCTION__, IFName);
        return retvOk;
    }
    else return retvFail;
}

#if 1 // ============================== Thread =================================
static THD_WORKING_AREA(waSndThread, 16384);
__noreturn
static void SoundThread(void *arg) {
    chRegSetThreadName("Sound");
    // Fill two buffers
    ReadToBuf(&Buf1);
    ReadToBuf(&Buf2);
    PNextBuf = &Buf2;
    // Start playing
    VS.SendFirstBuf((uint8_t*)Buf1.Buf, Buf1.Sz * 4);

    while(true) {
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
                if(Msg.Slot == ALL_SLOTS) mixer.fade(Msg.Volume);
                else Slot[Msg.Slot].SetVolume(Msg.Volume);
                break;

            case sndcmdStop:
                Printf("sndcmdStop %u\r", Msg.Slot);
                Slot[Msg.Slot].Stop();
                break;

            case sndcmdPrepareNextBuf:
                ReadToBuf(PNextBuf);
                break;

            case sndcmdNone: break;
        } // switch
    } // while true
}

void TrackEndCallback(int SlotN) {
//    Printf("TrackEnd: %u\r", SlotN);
    Slot[SlotN].Stop(); // Close file
    EvtQMain.SendNowOrExit(EvtMsg_t(evtIdSoundFileEnd, SlotN));
}

#endif

#if 1 // ========================= Interface ===================================
namespace SlotPlayer {
void Init() {
    MsgQSnd.Init();
    for(uint32_t i=0; i<AudioMixer::TRACKS; i++) Slot[i].Init(i);
    VS.Init();
    VS.OnBufEndI = OnVsBufEnd;
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

void SetVolume(uint16_t Volume) {
    MsgQSnd.SendNowOrExit(SndMsg_t(sndCmdVolume, ALL_SLOTS, Volume));
}

void Stop(uint8_t SlotN) {
    if(SlotN >= AudioMixer::TRACKS) return;
    MsgQSnd.SendNowOrExit(SndMsg_t(sndcmdStop, SlotN));
}
}
#endif
