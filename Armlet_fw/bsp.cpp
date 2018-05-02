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

// General
void Vibro(uint32_t Duration_ms) {
    Printf("%S\r", __FUNCTION__);
}
void PowerOff() {
    Printf("%S\r", __FUNCTION__);
}

void SleepEnable() {
    Printf("%S\r", __FUNCTION__);
}
void SleepDisable() {
    Printf("%S\r", __FUNCTION__);
}

// Sound
void PlayerVolumeUp() {
    Printf("%S\r", __FUNCTION__);
}
void PlayerVolumeDown() {
    Printf("%S\r", __FUNCTION__);
}

// Screen
void ScreenHighlight(uint32_t Value_percent) {
    Printf("%S %u\r", __FUNCTION__, Value_percent);
}
void ScreenAddBMPToQueue(const char* AFilename) {
    Printf("%S %S\r", __FUNCTION__, AFilename);
}
void ScreenShowNextBMP() {
    Printf("%S\r", __FUNCTION__);
}
void ScreenShowActualBMP() {
    Printf("%S\r", __FUNCTION__);
}
uint32_t GetBMPQueueLength() {
    Printf("%S\r", __FUNCTION__);
    return 0;
}
void ScreenShowPicture(const char* AFilename) {
    Printf("%S %S\r", __FUNCTION__, AFilename);
}

// Character
void SaveState(int Dogan, bool Dead, bool Corrupted) {
    Printf("%S\r", __FUNCTION__);
    if(TryOpenFileRewrite("State.csv", &CommonFile) == retvOk) {
        f_printf(&CommonFile, "Dogan = %d\r\n", Dogan);
        f_printf(&CommonFile, "Dead = %d\r\n", Dead);
        f_printf(&CommonFile, "Corrupted = %d\r\n", Corrupted);
        CloseFile(&CommonFile);
    }
}
void SaveKatet(const KaTetLinks *links) {
    Printf("%S\r", __FUNCTION__);
    if(TryOpenFileRewrite("KatetLinks.csv", &CommonFile) == retvOk) {
        for(uint32_t i=0; i<links->SIZE; i++) {
            f_printf(&CommonFile, "%u = %d\r\n", i, links->get(i));
        }
        CloseFile(&CommonFile);
    }
}
void SaveCounters(const KaTetCounters *counters) {
    Printf("%S\r", __FUNCTION__);
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
