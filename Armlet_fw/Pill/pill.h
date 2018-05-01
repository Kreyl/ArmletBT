/*
 * pill.h
 *
 *  Created on: 22 ��� 2014 �.
 *      Author: g.kruglov
 */

#pragma once

#include "color.h"

//enum PillType_t {
//    ptCure = 102,
//    ptPanacea = 103,
//};

struct Pill_t {
//    union { // Type
        int32_t TypeInt32;      // offset 0
//        PillType_t Type;        // offset 0
//    };
    // Contains dose value after pill application
//    int32_t ChargeCnt;          // offset 4
    bool IsOk() const { return (TypeInt32 >= 1 and TypeInt32 <= 127); }
} __attribute__ ((__packed__));
#define PILL_SZ     sizeof(Pill_t)
#define PILL_SZ32   (sizeof(Pill_t) / sizeof(int32_t))
