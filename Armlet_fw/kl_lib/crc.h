/*
 * crc.h
 *
 *  Created on: 2 ���. 2018 �.
 *      Author: Kreyl
 */

#pragma once

#include "inttypes.h"

uint16_t calc_crc16(char *buf, uint32_t len);
