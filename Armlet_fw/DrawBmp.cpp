/*
 * DrawBmp.cpp
 *
 *  Created on: 20 дек. 2017 г.
 *      Author: Kreyl
 */

#include "DrawBmp.h"
#include "lcd2630.h"

#define BUF_SZ              1024
static uint8_t IBuf[BUF_SZ];

// Length of next structure BmpInfo added to optimize reading
struct BmpHeader_t {
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t Reserved[2];
    uint32_t bfOffBits;
} __packed;
#define BMP_HDR_SZ      sizeof(BmpHeader_t)     // 14

struct BmpInfo_t {
    uint32_t BmpInfoSz;
    int32_t Width;
    int32_t Height;
    uint16_t Planes;
    uint16_t BitCnt;
    uint32_t Compression;
    uint32_t SzImage;
    int32_t XPelsPerMeter, YPelsPerMeter;
    uint32_t ClrUsed, ClrImportant;
    // End of Bmp info min
    uint32_t RedMsk, GreenMsk, BlueMsk, AlphaMsk;
    uint32_t CsType;
    uint32_t Endpoints[9];
    uint32_t GammaRed;
    uint32_t GammaGreen;
    uint32_t GammaBlue;
} __packed;
#define BMP_INFO_SZ         sizeof(BmpInfo_t)
#define BMP_MIN_INFO_SZ     40

// Color table
struct BGR_t {
    uint8_t B, G, R, A;
} __packed;
static BGR_t ColorTable[256];

//__ramfunc
static inline void PutTablePixel(uint8_t id) {
    uint8_t R = ColorTable[id].R;
    uint8_t G = ColorTable[id].G;
    uint8_t B = ColorTable[id].B;
    // High byte
    uint8_t byte1 = R & 0b11111000;
    byte1 |= G >> 5;
    // Low byte
    uint8_t byte2 = (G << 3) & 0b11100000;
    byte2 |= B >> 3;
    Lcd.PutBitmapNext(byte1, byte2);
}

//__ramfunc
void WriteLine1(uint8_t *PBuf, int32_t Width) {
    int32_t Cnt = 0, Top = MIN_(Width, LCD_W);
    while(true) {
        uint8_t Indx = *PBuf++;
        for(uint32_t k=0; k<8; k++) {
            PutTablePixel(Indx & 0x80 ? 1 : 0);
            Indx <<= 1;
            Cnt++;
            if(Cnt >= Top) return;
        }
    } // while(true)
}

//__ramfunc
void WriteLine4(uint8_t *PBuf, int32_t Width) {
    int32_t Cnt = 0, Top = MIN_(Width, LCD_W);
    while(true) {
        uint8_t Indx = *PBuf++;
        PutTablePixel((Indx >> 4) & 0x0F);
        Cnt++;
        if(Cnt >= Top) break;
        PutTablePixel(Indx & 0x0F);
        Cnt++;
        if(Cnt >= Top) break;
    } // while(true)
}

//__ramfunc
void WriteLine8(uint8_t *PBuf, int32_t Width) {
    int32_t Cnt = 0, Top = MIN_(Width, LCD_W);
    while(Cnt < Top) {
        uint8_t Indx = *PBuf++;
        PutTablePixel(Indx);
        Cnt++;
    } // while(true)
}

__ramfunc
void WriteLine16(uint8_t *PBuf, int32_t Width) {
    int32_t Cnt = 0, Top = MIN_(Width, LCD_W);
    while(Cnt < Top) {
        Lcd.PutBitmapNext(PBuf[1], PBuf[0]);
        PBuf += 2;
        Cnt++;
    }
}

__ramfunc
void WriteLine24(uint8_t *PBuf, int32_t Width) {
    int32_t Cnt = 0, Top = MIN_(Width, LCD_W);
    while(Cnt < Top) {
        uint8_t B = *PBuf++;
        uint8_t G = *PBuf++;
        uint8_t R = *PBuf++;
        // High byte
        uint8_t byte1 = R & 0b11111000;
        byte1 |= G >> 5;
        // Low byte
        uint8_t byte2 = (G << 3) & 0b11100000;
        byte2 |= B >> 3;
        Lcd.PutBitmapNext(byte1, byte2);
        Cnt++;
    }
}

void WriteLine32(uint8_t *PBuf, int32_t Width) {
    int32_t Cnt = 0, Top = MIN_(Width, LCD_W);
    while(Cnt < Top) {
        uint8_t B = *PBuf++;
        uint8_t G = *PBuf++;
        uint8_t R = *PBuf++;
        PBuf++; // Discard alpha channel
        // High byte
        uint8_t byte1 = R & 0b11111000;
        byte1 |= G >> 5;
        // Low byte
        uint8_t byte2 = (G << 3) & 0b11100000;
        byte2 |= B >> 3;
        Lcd.PutBitmapNext(byte1, byte2);
        Cnt++;
    }
}


uint8_t DrawBmpFile(uint8_t x0, uint8_t y0, const char *Filename, FIL *PFile) {
    Printf("Draw %S\r", Filename);
    uint32_t RCnt=0, FOffset, ColorTableSize = 0, BitCnt;
    int32_t Width, Height, LineSz;
    BmpHeader_t *PHdr;
    BmpInfo_t *PInfo;
    if(TryOpenFileRead(Filename, PFile) != retvOk) return retvFail;
    uint8_t Rslt = retvFail;

//    Clk.SwitchToHsi48();    // Increase MCU freq

    // ==== BITMAPFILEHEADER ====
    if(f_read(PFile, IBuf, BMP_HDR_SZ, &RCnt) != FR_OK) goto end;
    PHdr = (BmpHeader_t*)IBuf;
//    Printf("T=%X; Sz=%u; Off=%u\r", PHdr->bfType, PHdr->bfSize, PHdr->bfOffBits);
    if(PHdr->bfType != 0x4D42) goto end;    // Wrong file type
    FOffset = PHdr->bfOffBits;

    // ==== BITMAPINFO ====
    if(f_read(PFile, IBuf, BMP_MIN_INFO_SZ, &RCnt) != FR_OK) goto end;
    PInfo = (BmpInfo_t*)IBuf;
//    Printf("BmpInfoSz=%u; W=%d; H=%d; BitCnt=%u; Cmp=%u; Sz=%u;  ColorsInTbl=%u\r", PInfo->BmpInfoSz, PInfo->Width, PInfo->Height, PInfo->BitCnt, PInfo->Compression, PInfo->SzImage, PInfo->ClrUsed);
    Width = PInfo->Width;
    Height = PInfo->Height;
    BitCnt = PInfo->BitCnt;

    // Check row order
    if(Height < 0) Height = -Height; // Top to bottom, normal order. Just remove sign.
    else Lcd.SetDirHOrigBottomLeft();    // Bottom to top, set origin to bottom
    TRIM_VALUE(Height, LCD_H);

    // ==== Color table ====
    if(PInfo->ClrUsed == 0) {
        if     (BitCnt == 1) ColorTableSize = 2;
        else if(BitCnt == 4) ColorTableSize = 16;
        else if(BitCnt == 8) ColorTableSize = 256;
    }
    else ColorTableSize = PInfo->ClrUsed;
    if(ColorTableSize > 256) goto end;
    if(ColorTableSize != 0) {
        // Move file cursor to color table data if needed
        if(PInfo->BmpInfoSz != BMP_MIN_INFO_SZ) {
            uint32_t ClrTblOffset = BMP_HDR_SZ + PInfo->BmpInfoSz;
            if(f_lseek(PFile, ClrTblOffset) != FR_OK) goto end;
        }
        // Read color table
        if(f_read(PFile, ColorTable, (ColorTableSize * 4), &RCnt) != FR_OK) goto end;
    }

    // Move file cursor to pixel data
    if(f_lseek(PFile, FOffset) != FR_OK) goto end;
    // Setup window
    if(Width < LCD_W) x0 = (LCD_W - Width) / 2;     // }
    if(Height < LCD_H) y0 = (LCD_H - Height) / 2;   // } Put image to center
    Lcd.PutBitmapBegin(x0, y0, MIN_((uint8_t)Width, LCD_W), Height);

    // ==== Draw pic line by line ====
    LineSz = (((Width * BitCnt) / 8) + 3) & ~3;
    if(LineSz > BUF_SZ) goto end;
    for(int32_t i=0; i<Height; i++) {
        if(f_read(PFile, IBuf, LineSz, &RCnt) != FR_OK) goto end;
        // Select method of drawing depending on bits per pixel
        switch(BitCnt) {
            case 1:  WriteLine1 (IBuf, Width); break;
            case 4:  WriteLine4 (IBuf, Width); break;
            case 8:  WriteLine8 (IBuf, Width); break;
            case 16: WriteLine16(IBuf, Width); break;
            case 24: WriteLine24(IBuf, Width); break;
            case 32: WriteLine32(IBuf, Width); break;
            default: break;
        }
    } // for i
    Lcd.PutBitmapEnd();
    Rslt = retvOk;

    end:
    f_close(PFile);
    Lcd.SetDirHOrigTopLeft();   // Restore normal origin and direction

    // Switch back low freq
//    Clk.SwitchToHsi();
    // Signal Draw Completed
//    App.SignalEvt(EVT_LCD_DRAW_DONE);
    return Rslt;
}


