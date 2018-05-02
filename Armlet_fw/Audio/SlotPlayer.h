/*
 * SlotPlayer.h
 *
 *  Created on: 2 мая 2018 г.
 *      Author: Kreyl
 */

#pragma once

#include <inttypes.h>

namespace SlotPlayer {
    void Init();
    void Start(uint8_t SlotN, uint16_t Volume, const char* Emo, bool Repeat);
    void SetVolume(uint8_t SlotN, uint16_t Volume);
    void Stop(uint8_t SlotN);
};
