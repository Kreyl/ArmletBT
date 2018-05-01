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
void Vibro(uint32_t Duration_ms) {}
void PowerOff() {}

void SleepEnable() {}
void SleepDisable() {}

// Sound
void PlayerVolumeUp() {}
void PlayerVolumeDown() {}

// Screen
void ScreenHighlight(uint32_t Value_percent) {}
void ScreenAddBMPToQueue(const char* AFilename) {}
void ScreenShowNextBMP() {}
void ScreenShowActualBMP() {}
uint32_t GetBMPQueueLength() { return 0; }
void ScreenShowPicture(const char* AFilename) {}

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
            f_printf(&CommonFile, "%u = %u\r\n", i, counters[i]);
        }
        CloseFile(&CommonFile);
    }
}
