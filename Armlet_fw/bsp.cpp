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

#define PRINT_FUNC()  Printf("%S\r", __FUNCTION__)

// General
void Vibro(uint32_t Duration_ms) {
    PRINT_FUNC();
}
void PowerOff() {
    PRINT_FUNC();
}

void SleepEnable() {
    PRINT_FUNC();
}
void SleepDisable() {
    PRINT_FUNC();
}

#if 1 // Sound
void PlayerVolumeUp() {
    PRINT_FUNC();
}
void PlayerVolumeDown() {
    PRINT_FUNC();
}
void PlayerStart(uint8_t SlotN, uint16_t Volume, const char* Emo, bool Repeat) {
    PRINT_FUNC();

}
void PlayerSetVolume(uint8_t SlotN, uint16_t Volume) {
    PRINT_FUNC();

}
void PlayerStop(uint8_t SlotN) {
    PRINT_FUNC();

}
#endif

// Screen
void ScreenHighlight(uint32_t Value_percent) {
    PRINT_FUNC();
}
void ScreenAddBMPToQueue(const char* AFilename) {
    PRINT_FUNC();
}
void ScreenShowNextBMP() {
    PRINT_FUNC();
}
void ScreenShowActualBMP() {
    PRINT_FUNC();
}
uint32_t GetBMPQueueLength() {
    PRINT_FUNC();
    return 0;
}
void ScreenShowPicture(const char* AFilename) {
    PRINT_FUNC();
}

// Character
void SaveState(int Dogan, bool Dead, bool Corrupted) {
    PRINT_FUNC();
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
