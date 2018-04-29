/*
 * glue.h
 *
 *  Created on: 28 апр. 2018 г.
 *      Author: Kreyl
 */

#pragma once

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
void ScreenAddBMPToQueue(char* AFilename);
void ScreenShowNextBMP();
void ScreenShowActualBMP();
uint32_t GetBMPQueueLength();
void ScreenShowPicture(char* AFilename);

// Character
void LoadCharactericstics();
void SaveState(uint32_t StateCode);
void SaveKatet();
void SaveCounters();
