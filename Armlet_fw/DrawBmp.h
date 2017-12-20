/*
 * DrawBmp.h
 *
 *  Created on: 20 дек. 2017 г.
 *      Author: Kreyl
 */

#pragma once

#include "kl_lib.h"
#include "kl_fs_utils.h"

uint8_t DrawBmpFile(uint8_t x0, uint8_t y0, const char *Filename, FIL *PFile);
