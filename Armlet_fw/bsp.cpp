/*
 * bsp.cpp
 *
 *  Created on: 1 мая 2018 г.
 *      Author: Kreyl
 */

#include "bsp.h"
#include "kl_fs_utils.h"

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
    if(TryOpenFileRewrite("State.csv", &CommonFile) == retvOk) {
        CloseFile(&CommonFile);
    }
}
void SaveKatet(const KaTetLinks *links) {}
void SaveCounters(const KaTetCounters *counters) {}


