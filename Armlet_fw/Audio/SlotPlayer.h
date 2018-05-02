/*
 * SlotPlayer.h
 *
 *  Created on: 2 ��� 2018 �.
 *      Author: Kreyl
 */

#pragma once

namespace SlotPlayer {
    void Init();
    void Start(uint8_t SlotN, uint8_t Volume, char* Emo);
    void SetVolume(uint8_t SlotN, uint8_t Volume);
    void Stop(uint8_t SlotN);
};
