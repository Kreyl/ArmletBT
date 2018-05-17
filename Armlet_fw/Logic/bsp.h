#pragma once

#include <cstdint>

class Flags;
typedef Flags KaTetLinks;

class Counters;
typedef Counters KaTetCounters;

// General
void Vibro(uint32_t Duration_ms, uint32_t Cnt=0);
void PowerOff();
void SleepEnable();
void SleepDisable();

// Sound
void PlayerVolumeUp();
void PlayerVolumeDown();
void PlayerStart(uint8_t SlotN, uint16_t Volume, const char* Emo, bool Repeat);
void PlayerSetVolume(uint8_t SlotN, uint16_t Volume);
void PlayerStop(uint8_t SlotN);

// Screen
void ScreenHighlight(uint32_t Value_percent);
void ScreenAddBMPToQueue(const char* AFilename);
void ScreenShowNextBMP();
void ScreenShowActualBMP();
uint32_t GetBMPQueueLength();
void ScreenShowPicture(const char* AFilename);

// Character
void SetTodash(bool Todash);
void SaveState(int Dogan, bool Dead, bool Corrupted);
void SaveKatet(const KaTetLinks *links);
void SaveCounters(const KaTetCounters *counters);
