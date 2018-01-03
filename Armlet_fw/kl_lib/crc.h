/*
 * crc.h
 *
 *  Created on: 2 џэт. 2018 у.
 *      Author: Kreyl
 */

#pragma once

#include "inttypes.h"

uint16_t calc_crc16(char *buf, uint32_t len);
