/*
 * File:   lcd6101.h
 * Author: Kreyl Laurelindo
 *
 * Created on 10.11.2012
 */

#pragma once

#include "kl_lib.h"
#include "board.h"
#include "color.h"

// This LCD embeds SPFD54124B controller

// ================================ Defines ====================================
#define LCD_GPIO        GPIOE
#define LCD_DC          8
#define LCD_WR          9
#define LCD_RD          10
#define LCD_XCS         11
#define LCD_XRES        12
#define LCD_MASK_WR     (0x00FF | (1<<LCD_WR))  // clear bus and set WR low
#define LCD_MODE_READ   0xFFFF0000
#define LCD_MODE_WRITE  0x00005555
#define LCD_BCKLT_PIN   { GPIOB, 9, TIM11, 1, invNotInverted, omPushPull, 100 }

#define LCD_X_0             1   // }
#define LCD_Y_0             2   // } Zero pixels are shifted
#define LCD_H               128 // }
#define LCD_W               160 // } Pixels count
#define LCD_TOP_BRIGHTNESS  100 // i.e. 100%

class Lcd_t {
private:
    const PinOutputPWM_t BckLt{LCD_BCKLT_PIN};
    void WriteCmd(uint8_t ACmd);
    void WriteCmd(uint8_t ACmd, uint8_t AData);
public:
    // General use
    void Init();
    void Shutdown();
    void Brightness(uint16_t Brightness)  { BckLt.Set(Brightness); }
    // Direction & Origin
    void SetDirHOrigTopLeft()    { WriteCmd(0x36, 0xA0); } //
    void SetDirHOrigBottomLeft() { WriteCmd(0x36, 0xE0); } //

    // High-level
    void SetBounds(uint8_t xStart, uint8_t xEnd, uint8_t yStart, uint8_t yEnd);
//    void Printf(uint8_t x, uint8_t y, Color_t ForeClr, Color_t BckClr, const char *S, ...);
    void Cls(Color_t Color);
    void GetBitmap(uint8_t x0, uint8_t y0, uint8_t Width, uint8_t Height, uint16_t *PBuf);
    void PutBitmap(uint8_t x0, uint8_t y0, uint8_t Width, uint8_t Height, uint16_t *PBuf);
    void PutBitmapBegin(uint8_t x0, uint8_t y0, uint8_t Width, uint8_t Height);
    void PutBitmapEnd();
    void PutPixel (uint8_t x0, uint8_t y0, uint16_t Clr);
//    void DrawImage(const uint8_t x, const uint8_t y, const uint8_t *Img);
//    void DrawSymbol(const uint8_t x, const uint8_t y, const uint8_t ACode);
};

extern Lcd_t Lcd;

__attribute__ ((always_inline)) static inline void WriteByte(uint8_t Byte) {
    __NOP(); __NOP();
    LCD_GPIO->BSRR = Byte;                // Place data on bus
    __NOP(); __NOP();
    LCD_GPIO->BSRR = (1<<LCD_WR);         // WR high
    __NOP(); __NOP();
    LCD_GPIO->BSRR = (LCD_MASK_WR << 16); // Clear bus and set WR Low
}

__attribute__ ((always_inline)) inline
void PutBitmapNext(uint8_t Byte1, uint8_t Byte2) {
    WriteByte(Byte1);
    WriteByte(Byte2);
}
