#pragma once

#include <cstdint>

class KaTetLinks;
class KaTetCounters;

// General
void Vibro(uint32_t Duration_ms);
void PowerOff();
void SleepEnable();
void SleepDisable();

// Sound
void PlayerVolumeUp();
void PlayerVolumeDown();

// Screen
void ScreenHighlight(uint32_t Value_percent);
void ScreenAddBMPToQueue(const char* AFilename);
void ScreenShowNextBMP();
void ScreenShowActualBMP();
uint32_t GetBMPQueueLength();
void ScreenShowPicture(const char* AFilename);

// Character
void SaveState(int Dogan, bool Dead, bool Corrupted);
void SaveKatet(const KaTetLinks *links);
void SaveCounters(const KaTetCounters *counters);
