#include "lcd2630.h"
#include <string.h>
#include <stdarg.h>
#include "core_cmInstr.h"

#include "lcdFont8x8.h"

// Variables
Lcd_t Lcd;
//static char CharBuf[198];

// Pin driving functions
#define LCD_DELAY()         DelayLoop(36)
static inline void XRES_Hi() { PinSetHi(LCD_GPIO, LCD_XRES); LCD_DELAY();}
static inline void XRES_Lo() { PinSetLo(LCD_GPIO, LCD_XRES); LCD_DELAY();}
static inline void XCS_Hi () { PinSetHi(LCD_GPIO, LCD_XCS);  LCD_DELAY();}
static inline void XCS_Lo () { PinSetLo(LCD_GPIO, LCD_XCS);  LCD_DELAY();}
__always_inline static inline void DC_Hi()  { PinSetHi(LCD_GPIO, LCD_DC);   LCD_DELAY();}
__always_inline static inline void DC_Lo()  { PinSetLo(LCD_GPIO, LCD_DC);   LCD_DELAY();}
static inline void WR_Hi()   { PinSetHi(LCD_GPIO, LCD_WR);   LCD_DELAY();}
static inline void RD_Hi()   { PinSetLo(LCD_GPIO, LCD_RD);   LCD_DELAY();}
//__attribute__ ((always_inline)) static inline void RD_Lo()  { PinClear(LCD_GPIO, LCD_RD);   LCD_DELAY}

void Lcd_t::Init() {
    // Backlight
    BckLt.Init();
    PinSetupOut(LCD_GPIO, LCD_DC,   omPushPull, psHigh);
    PinSetupOut(LCD_GPIO, LCD_WR,   omPushPull, psHigh);
    PinSetupOut(LCD_GPIO, LCD_RD,   omPushPull, psHigh);
    PinSetupOut(LCD_GPIO, LCD_XRES, omPushPull, psHigh);
    PinSetupOut(LCD_GPIO, LCD_XCS,  omPushPull, psHigh);
    // Configure data bus as outputs
    for(uint8_t i=0; i<8; i++) PinSetupOut(LCD_GPIO, i, omPushPull, psHigh);

    // ======= Init LCD =======
    Brightness(LCD_TOP_BRIGHTNESS);
    XCS_Hi();
    XRES_Lo();  // }
    XRES_Hi();  // } Reset display
    DC_Lo();    // Command mode by default
    WR_Hi();    // Default hi
    RD_Hi();    // Default hi
    XCS_Lo();   // Interface is always enabled

    WriteCmd(0x11);         // Sleep out
    chThdSleepMilliseconds(207);
    WriteCmd(0x13);         // Normal Display Mode ON
    WriteCmd(0x3A, 0x05);   // Pixel format: VIPF=0(undef), IFPF=16 bit per pixel
    WriteCmd(0x29);         // Display on
    WriteCmd(0x20);         // Inv off
    WriteCmd(0x13);         // Normal Display Mode ON
    WriteCmd(0x36, 0xA0);   // Display mode: Y inv, X none-inv, Row/Col exchanged

    Cls(clBlack);
}

void Lcd_t::Shutdown(void) {
    XRES_Lo();
    XCS_Lo();
    Brightness(0);
}

// =============================== Local use ===================================
__attribute__ ((always_inline)) static inline void ModeWrite() {
    LCD_GPIO->MODER |= LCD_MODE_WRITE;
}
__attribute__ ((always_inline)) static inline void ModeRead() {
    LCD_GPIO->MODER &= LCD_MODE_READ;
}

__attribute__ ((always_inline)) static inline void WriteByte(uint8_t Byte) {
    LCD_GPIO->BSRR = (LCD_MASK_WR << 16);  // Clear bus and set WR Low
    LCD_GPIO->BSRR = Byte;         // Place data on bus
    LCD_GPIO->BSRR = (1<<LCD_WR);  // WR high
}
__attribute__ ((always_inline)) static inline uint8_t ReadByte() {
    uint16_t w;
    LCD_GPIO->BSRR = ((1<<LCD_RD) << 16);  // RD Low
    LCD_GPIO->BSRR = (1<<LCD_RD);  // RD high
    w = LCD_GPIO->IDR;             // Read data from bus
    return (uint8_t)w;
}

// ==== WriteCmd ====
void Lcd_t::WriteCmd(uint8_t ACmd) {
    // DC is lo by default => Cmd by default
    WriteByte(ACmd);    // Send Cmd byte
}
void Lcd_t::WriteCmd(uint8_t ACmd, uint8_t AData) {
    // DC is lo by default => Cmd by default
    WriteByte(ACmd);    // Send Cmd byte
    // Send data
    DC_Hi();
    WriteByte(AData);
    DC_Lo();
}

// ================================= Printf ====================================
//uint16_t Lcd_t::PutChar(uint8_t x, uint8_t y, char c, Color_t ForeClr, Color_t BckClr) {
//    char *PFont = (char*)Font8x8;  // Font to use
//    // Read font params
//    uint8_t nCols = PFont[0];
//    uint8_t nRows = PFont[1];
//    uint16_t nBytes = PFont[2];
    // Get pointer to the first byte of the desired character
//    const char *PChar = Font8x8 + (nBytes * (c - 0x1F));
    // Iterate rows of the char
//    uint8_t row, col;
//    for(uint8_t row = 0; row < nRows; row++) {
//        if((y+row) >= LCD_H) break;
//        uint8_t PixelRow = *PChar++;
//        // loop on each pixel in the row (left to right)
//        uint8_t Mask = 0x80;
//        for (col = 0; col < nCols; col++) {
//            if((x+col) >= LCD_W) break;
//            PackedBuf[y+row][x+col] = (PixelRow & Mask)? ForeClr : BckClr;
//            Mask >>= 1;
//        } // col
//    } // row
//    // Mark area as changed
//    uint8_t xaStart = x / AREA_W;
//    uint8_t yaStart = y / AREA_H;
//    uint8_t xaEnd = (x+nCols) / AREA_W;
//    uint8_t yaEnd = (y+nRows) / AREA_H;
//    for(row = yaStart; row<=yaEnd; row++)
//        for(col = xaStart; col<=xaEnd; col++)
//            Changed[row][col] = true;
//    // Return next pixel to right
//    return x+nCols;
//}

//void Lcd_t::Printf(uint8_t x, uint8_t y, const Color_t ForeClr, const Color_t BckClr, const char *S, ...) {
//    // Printf to buffer
//    va_list args;
//    va_start(args, S);
//    uint32_t Cnt = tiny_vsprintf(CharBuf, S, args);
//    va_end(args);
//    // Draw what printed
//    for(uint32_t i=0; i<Cnt; i++) {
//        x = PutChar(x, y, CharBuf[i], ForeClr, BckClr);
//        if(x>160) break;
//    }
//}

// ================================ Graphics ===================================
void Lcd_t::SetBounds(uint8_t xStart, uint8_t xEnd, uint8_t yStart, uint8_t yEnd) {
    // Set column bounds
    WriteByte(0x2A);
    DC_Hi();
    WriteByte(0x00);            // }
    WriteByte(LCD_X_0+xStart);  // } Col addr start
    WriteByte(0x00);            // }
    WriteByte(LCD_X_0+xEnd-1);  // } Col addr end
    DC_Lo();
    // Set row bounds
    WriteByte(0x2B);
    DC_Hi();
    WriteByte(0x00);            // }
    WriteByte(LCD_Y_0+yStart);  // } Row addr start = 0
    WriteByte(0x00);            // }
    WriteByte(LCD_Y_0+yEnd-1);  // } Row addr end
    DC_Lo();
}

void Lcd_t::Cls(Color_t Color) {
    SetBounds(0, LCD_W, 0, LCD_H);
    uint32_t Cnt = LCD_W * LCD_H;
    uint8_t HiByte = Color.RGBTo565_HiByte();
    uint8_t LoByte = Color.RGBTo565_LoByte();
    // Write RAM
    WriteByte(0x2C);    // Memory write
    DC_Hi();
    for(uint32_t i=0; i<Cnt; i++) {
        WriteByte(HiByte);
        WriteByte(LoByte);
    }
    DC_Lo();
}

void Lcd_t::GetBitmap(uint8_t x0, uint8_t y0, uint8_t Width, uint8_t Height, uint16_t *PBuf) {
    SetBounds(x0, x0+Width, y0, y0+Height);
    // Prepare variables
    uint32_t Cnt = Width * Height;
    uint16_t R, G, B;
    // Read RAM
    WriteByte(0x2E);    // RAMRD
    DC_Hi();
    ModeRead();
    ReadByte();         // Dummy read
    for(uint32_t i=0; i<Cnt; i++) {
        R = ReadByte(); // }
        G = ReadByte(); // }
        B = ReadByte(); // } Inside LCD, data is always in 18bit format.
        // Produce 4R-4G-4B from 6R-6G-6B
        *PBuf++ = ((R & 0xF0) << 4) | (G & 0xF0) | ((B & 0xF0) >> 4);
    }
    ModeWrite();
    DC_Lo();
}

void Lcd_t::PutBitmap(uint8_t x0, uint8_t y0, uint8_t Width, uint8_t Height, uint16_t *PBuf) {
    //Uart.Printf("%u %u %u %u %u\r", x0, y0, Width, Height, *PBuf);
    SetBounds(x0, x0+Width, y0, y0+Height);
    // Prepare variables
    uint16_t Clr;
    uint32_t Cnt = (uint32_t)Width * (uint32_t)Height;    // One pixel at one time
    // Write RAM
    WriteByte(0x2C);    // Memory write
    DC_Hi();
    for(uint32_t i=0; i<Cnt; i++) {
        Clr = *PBuf++;
        WriteByte(Clr >> 8);
        WriteByte(Clr & 0xFF);
    }
    DC_Lo();
}

void Lcd_t::PutBitmapBegin(uint8_t x0, uint8_t y0, uint8_t Width, uint8_t Height) {
    SetBounds(x0, x0+Width, y0, y0+Height);
    // Write RAM
    WriteByte(0x2C);    // Memory write
    DC_Hi();
}
void Lcd_t::PutBitmapNext(uint8_t Byte1, uint8_t Byte2) {
    WriteByte(Byte1);
    WriteByte(Byte2);
}
void Lcd_t::PutBitmapEnd() {
    DC_Lo();
}
