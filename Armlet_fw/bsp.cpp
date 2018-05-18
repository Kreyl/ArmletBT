/*
 * bsp.cpp
 *
 *  Created on: 1 мая 2018 г.
 *      Author: Kreyl
 */

#include "bsp.h"
#include "kl_fs_utils.h"
#include "shell.h"
#include "localcharacter.h"
#include "SlotPlayer.h"
#include "ChunkTypes.h"
#include "vibro.h"
#include "radio_lvl1.h"
#include "lcd2630.h"
#include "kl_buf.h"
#include "DrawBmp.h"

#define PRINT_FUNC()  // Printf("%S\r", __FUNCTION__)
#define STANDBY_TIMEOUT_MS  1008

#if 1 // General
extern Vibro_t Vibra;
BaseChunk_t vsqBrr[] = {
        {csSetup, 100}, // Vibro volume = 100
        {csWait, 11},   // dummy
        {csSetup, 0},
        {csWait, 11},   // dummy
        {csRepeat, 0},
        {csEnd}
};

void Vibro(uint32_t Duration_ms, int Cnt) {
    Printf("Vibro: dur %u; Cnt %u\r", Duration_ms, Cnt);
    vsqBrr[1].Time_ms = Duration_ms;
    if(Cnt > 1) { // Repeat several times
        vsqBrr[3].Time_ms = 108; // Some default value
        vsqBrr[4].RepeatCnt = Cnt - 1;
    }
    else { // Cnt == 1, no repeat
        vsqBrr[3].Time_ms = 0;
        vsqBrr[4].RepeatCnt = 0;
    }
    Vibra.StartOrRestart(vsqBrr);
}

void PowerOff() {
    chSysLock();
    __disable_irq();
    // Setup IWDG to reset after a while
    Iwdg::InitAndStart(STANDBY_TIMEOUT_MS);
    // Enter standby
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;  // Set DEEPSLEEP bit
    // Flash stopped in stop mode, Enter Standby mode, power regulator in low-power mode when stopped, clear WakeUp and Standby flags
    PWR->CR = PWR_CR_FPDS | PWR_CR_PDDS | PWR_CR_LPDS | PWR_CR_CSBF | PWR_CR_CWUF;
    // Command to clear WUF (wakeup flag) and wait two sys clock cycles to allow it be cleared
    PWR->CR |= PWR_CR_CWUF;
    __NOP(); __NOP();
    __WFI();
    chSysUnlock();
}

static bool IsSleepingNow = false;
void SleepEnable() {
    Lcd.Shutdown();
    IsSleepingNow = true;
}
void SleepDisable() {
    if(IsSleepingNow) Lcd.Init();
    IsSleepingNow = false;
}
bool IsSleeping() {
    return IsSleepingNow;
}
#endif

#if 1 // Sound
static const uint16_t VolumeLvls[] = {
        4, 5, 8, 11, 16, 22, 32, 45, 64, 90, 128, 181, 256, 362, 512, 724, 1024, 1448, 2048, 2896, 4096, 5792, 8192, 11585
};
#define VOL_LVLS_CNT    countof(VolumeLvls)
static uint8_t CurrVolLvlIndx = 20;

void PlayerVolumeUp() {
    PRINT_FUNC();
    if(CurrVolLvlIndx < (VOL_LVLS_CNT - 1)) {
        CurrVolLvlIndx++;
        SlotPlayer::SetVolumeForAll(VolumeLvls[CurrVolLvlIndx]);
    }
}
void PlayerVolumeDown() {
    PRINT_FUNC();
    if(CurrVolLvlIndx > 0) {
        CurrVolLvlIndx--;
        SlotPlayer::SetVolumeForAll(VolumeLvls[CurrVolLvlIndx]);
    }
}

void PlayerStart(uint8_t SlotN, uint16_t Volume, const char* Emo, bool Repeat) {
    PRINT_FUNC();
    SlotPlayer::Start(SlotN, Volume, Emo, Repeat);
}
void PlayerSetVolume(uint8_t SlotN, uint16_t Volume) {
    PRINT_FUNC();
    SlotPlayer::SetVolume(SlotN, Volume);
}
void PlayerStop(uint8_t SlotN) {
    PRINT_FUNC();
    SlotPlayer::Stop(SlotN);
}
#endif

#if 1 // Screen
static char PicName[MAX_NAME_LEN];

void ScreenHighlight(uint32_t Value_percent) {
    PRINT_FUNC();
    Lcd.Brightness(Value_percent);
}
void ScreenShowPicture(const char* AFilename) {
    PRINT_FUNC();
    strcpy(PicName, "Images/");
    strcat(PicName, AFilename);
    if(IsSleepingNow) SleepDisable();
    DrawBmpFile(0, 0, PicName, &CommonFile);
}

#define BMP_Q_LEN   18
LifoNumber_t<const char*, BMP_Q_LEN> IBmpBuf;

void ScreenAddBMPToQueue(const char* AFilename) {
    PRINT_FUNC();
    IBmpBuf.Put(AFilename);
    ScreenShowActualBMP();
}
void ScreenShowNextBMP() {
    PRINT_FUNC();
    const char* PFilename;
    if(IBmpBuf.Get(&PFilename) == retvOk) ScreenShowPicture(PFilename);
    else Printf("Empty BMP queue\r");
}
void ScreenShowActualBMP() {
    PRINT_FUNC();
    const char* PFilename;
    if(IBmpBuf.GetAndDoNotRemove(&PFilename) == retvOk) ScreenShowPicture(PFilename);
    else Printf("Empty BMP queue\r");
}
uint32_t GetBMPQueueLength() {
    // PRINT_FUNC();
    Printf("GetBMPQueueLength: %u\r", IBmpBuf.GetFullCount());
    return IBmpBuf.GetFullCount();
}
#endif

#if 1 // Character
void SetTodash(bool Todash) {
    Radio.PktTx.IsInTodash = Todash;
}

void SaveState(int Dogan, bool Dead, bool Corrupted) {
    PRINT_FUNC();
    // Construct param to transmit
    Radio.PktTx.Dogan = Dogan;
    Radio.PktTx.Dead = Dead;
    Radio.PktTx.Corrupted = Corrupted;
    // Save state
    if(TryOpenFileRewrite("State.csv", &CommonFile) == retvOk) {
        f_printf(&CommonFile, "Dogan = %d\r\n", Dogan);
        f_printf(&CommonFile, "Dead = %d\r\n", Dead);
        f_printf(&CommonFile, "Corrupted = %d\r\n", Corrupted);
        CloseFile(&CommonFile);
    }
}
void SaveKatet(const KaTetLinks *links) {
    PRINT_FUNC();
    if(TryOpenFileRewrite("KatetLinks.csv", &CommonFile) == retvOk) {
        for(uint32_t i=0; i<links->SIZE; i++) {
            f_printf(&CommonFile, "%u = %d\r\n", i, links->get(i));
        }
        CloseFile(&CommonFile);
    }
}
void SaveCounters(const KaTetCounters *counters) {
    PRINT_FUNC();
    if(TryOpenFileRewrite("Counters.csv", &CommonFile) == retvOk) {
        for(uint32_t i=0; i<counters->SIZE; i++) {
            f_printf(&CommonFile, "%u = %u\r\n", i, (*counters)[i]);
        }
        CloseFile(&CommonFile);
    }
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
