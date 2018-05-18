#include "lcd2630.h"
#include <string.h>
#include <stdarg.h>
#include "core_cmInstr.h"

#include "lcdFont8x8.h"

// Variables
Lcd_t Lcd;

// Pin driving functions
#define LCD_DELAY()        // DelayLoop(36)
static inline void LCD_XRES_Hi() { PinSetHi(LCD_GPIO, LCD_XRES); }
static inline void LCD_XRES_Lo() { PinSetLo(LCD_GPIO, LCD_XRES); }
static inline void LCD_XCS_Hi () { PinSetHi(LCD_GPIO, LCD_XCS);  }
static inline void LCD_XCS_Lo () { PinSetLo(LCD_GPIO, LCD_XCS);  }
__always_inline static inline void LCD_DC_Hi()  { PinSetHi(LCD_GPIO, LCD_DC);   LCD_DELAY();}
__always_inline static inline void LCD_DC_Lo()  { PinSetLo(LCD_GPIO, LCD_DC);   LCD_DELAY();}
static inline void LCD_WR_Hi()   { PinSetHi(LCD_GPIO, LCD_WR);   LCD_DELAY();}
//static inline void LCD_WR_Lo()   { PinSetLo(LCD_GPIO, LCD_WR);   LCD_DELAY();}
static inline void LCD_RD_Hi()   { PinSetLo(LCD_GPIO, LCD_RD);   LCD_DELAY();}
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
    // Reset display
    LCD_XRES_Hi();
    LCD_XCS_Hi();
    chThdSleepMilliseconds(9);
    LCD_XRES_Lo();
    chThdSleepMilliseconds(9);
    LCD_XRES_Hi();
    chThdSleepMilliseconds(130); // No less than 360
    LCD_DC_Lo();    // Command mode by default
    LCD_WR_Hi();    // Default hi
    LCD_RD_Hi();    // Default hi
    LCD_XCS_Lo();   // Interface is always enabled

    WriteCmd(0x01);         // Software reset
    chThdSleepMilliseconds(130);
    WriteCmd(0x11);         // Sleep out
    chThdSleepMilliseconds(9);
    WriteCmd(0x13);         // Normal Display Mode ON
    WriteCmd(0x3A, 0x55);   // Pixel format: VIPF=0(undef), IFPF=16 bit per pixel
    WriteCmd(0x29);         // Display on
    WriteCmd(0x20);         // Inv off
    WriteCmd(0x13);         // Normal Display Mode ON
    WriteCmd(0x36, 0xA0);   // Display mode: Y inv, X none-inv, Row/Col exchanged
//    Cls(clRed);
    Brightness(LCD_TOP_BRIGHTNESS);

}

void Lcd_t::Shutdown(void) {
    LCD_XRES_Lo();
    LCD_XCS_Lo();
    Brightness(0);
}

// =============================== Local use ===================================
__attribute__ ((always_inline)) static inline void ModeWrite() {
    LCD_GPIO->MODER |= LCD_MODE_WRITE;
}
__attribute__ ((always_inline)) static inline void ModeRead() {
    LCD_GPIO->MODER &= LCD_MODE_READ;
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
    LCD_DC_Hi();
    WriteByte(AData);
    LCD_DC_Lo();
}

// ================================ Graphics ===================================
void Lcd_t::SetBounds(uint8_t xStart, uint8_t xEnd, uint8_t yStart, uint8_t yEnd) {
    // Set column bounds
    WriteByte(0x2A);
    LCD_DC_Hi();
    WriteByte(0x00);            // }
    WriteByte(LCD_X_0+xStart);  // } Col addr start
    WriteByte(0x00);            // }
    WriteByte(LCD_X_0+xEnd-1);  // } Col addr end
    LCD_DC_Lo();
    // Set row bounds
    WriteByte(0x2B);
    LCD_DC_Hi();
    WriteByte(0x00);            // }
    WriteByte(LCD_Y_0+yStart);  // } Row addr start = 0
    WriteByte(0x00);            // }
    WriteByte(LCD_Y_0+yEnd-1);  // } Row addr end
    LCD_DC_Lo();
}

void Lcd_t::Cls(Color_t Color) {
    SetBounds(0, LCD_W, 0, LCD_H);
    uint32_t Cnt = LCD_W * LCD_H;
    uint8_t HiByte = Color.RGBTo565_HiByte();
    uint8_t LoByte = Color.RGBTo565_LoByte();
    // Write RAM
    WriteByte(0x2C);    // Memory write
    LCD_DC_Hi();
    for(uint32_t i=0; i<Cnt; i++) {
        WriteByte(HiByte);
        WriteByte(LoByte);
    }
    LCD_DC_Lo();
}

void Lcd_t::GetBitmap(uint8_t x0, uint8_t y0, uint8_t Width, uint8_t Height, uint16_t *PBuf) {
    SetBounds(x0, x0+Width, y0, y0+Height);
    // Prepare variables
    uint32_t Cnt = Width * Height;
    uint16_t R, G, B;
    // Read RAM
    WriteByte(0x2E);    // RAMRD
    LCD_DC_Hi();
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
    LCD_DC_Lo();
}

void Lcd_t::PutBitmap(uint8_t x0, uint8_t y0, uint8_t Width, uint8_t Height, uint16_t *PBuf) {
    //Uart.Printf("%u %u %u %u %u\r", x0, y0, Width, Height, *PBuf);
    SetBounds(x0, x0+Width, y0, y0+Height);
    // Prepare variables
    uint16_t Clr;
    uint32_t Cnt = (uint32_t)Width * (uint32_t)Height;    // One pixel at one time
    // Write RAM
    WriteByte(0x2C);    // Memory write
    LCD_DC_Hi();
    for(uint32_t i=0; i<Cnt; i++) {
        Clr = *PBuf++;
        WriteByte(Clr >> 8);
        WriteByte(Clr & 0xFF);
    }
    LCD_DC_Lo();
}

void Lcd_t::PutBitmapBegin(uint8_t x0, uint8_t y0, uint8_t Width, uint8_t Height) {
    SetBounds(x0, x0+Width, y0, y0+Height);
    // Write RAM
    WriteByte(0x2C);    // Memory write
    LCD_DC_Hi();
}

void Lcd_t::PutBitmapEnd() {
    LCD_DC_Lo();
}

// ==== Printf ====
uint8_t Lcd_t::IPutChar(char c) {
    uint8_t Rslt = retvOk;
    char *PFont = (char*)Font8x8;  // Font to use
    // Read font params
    uint8_t nCols = PFont[0];
    uint8_t nRows = PFont[1];
    uint16_t nBytes = PFont[2];
    SetBounds(Fnt.X, Fnt.X+nCols, Fnt.Y, Fnt.Y+nRows);
    // Get pointer to the first byte of the desired character
    const char *PChar = Font8x8 + (nBytes * (c - 0x1F));
    // Write RAM
    WriteByte(0x2C);    // Memory write
    LCD_DC_Hi();
    // Iterate rows of the char
    uint8_t row, col;
    for(row = 0; row < nRows; row++) {
        if((Fnt.Y+row) >= LCD_H) {
            Rslt = retvFail;
            break;
        }
        uint8_t PixelRow = *PChar++;
        // Loop on each pixel in the row (left to right)
        for(col=0; col < nCols; col++) {
            if((Fnt.X+col) >= LCD_W) {
                Rslt = retvFail;
                break;
            }
            Color_t *PClr = (PixelRow & 0x80)? &Fnt.FClr : &Fnt.BClr;
            PixelRow <<= 1;
            WriteByte(PClr->RGBTo565_HiByte());
            WriteByte(PClr->RGBTo565_LoByte());
        } // col
    } // row
    LCD_DC_Lo();
    Fnt.X += nCols;
    return Rslt;
}

void Lcd_t::LPrintf(uint8_t x, uint8_t y, Color_t ForeClr, Color_t BckClr, const char *format, ...) {
    Fnt.X = x;
    Fnt.Y = y;
    Fnt.FClr = ForeClr;
    Fnt.BClr = BckClr;
    va_list args;
    va_start(args, format);
    IVsPrintf(format, args);
    va_end(args);
}
