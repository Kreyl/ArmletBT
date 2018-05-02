#pragma once

#include "kl_sd.h"
#include <stdint.h>
#include "kl_lib.h"
#include "uart.h"
#include "MsgQ.h"

// ==== Defines ====
#define VS_GPIO         GPIOB
// Pins
#define VS_XCS          3
#define VS_XDCS         8
#define VS_RST          4
#define VS_DREQ         0
#define VS_XCLK         13
#define VS_SO           14
#define VS_SI           15
// SPI
#define VS_SPI          SPI2
#define VS_AF           AF5
#define VS_SPI_RCC_EN() rccEnableSPI2(FALSE)
#define VS_MAX_SPI_BAUDRATE_HZ  3400000
// DMA
#define VS_DMA          STM32_DMA1_STREAM4
#define VS_DMA_CHNL     0
#define VS_DMA_MODE     STM32_DMA_CR_CHSEL(VS_DMA_CHNL) | \
                        DMA_PRIORITY_LOW | \
                        STM32_DMA_CR_MSIZE_BYTE | \
                        STM32_DMA_CR_PSIZE_BYTE | \
                        STM32_DMA_CR_DIR_M2P |    /* Direction is memory to peripheral */ \
                        STM32_DMA_CR_TCIE         /* Enable Transmission Complete IRQ */


// Command codes
#define VS_READ_OPCODE  0b00000011
#define VS_WRITE_OPCODE 0b00000010

// Registers
#define VS_REG_MODE         0x00
#define VS_REG_STATUS       0x01
#define VS_REG_BASS         0x02
#define VS_REG_CLOCKF       0x03
#define VS_REG_DECODE_TIME  0x04
#define VS_REG_AUDATA       0x05
#define VS_REG_WRAM         0x06
#define VS_REG_WRAMADDR     0x07
#define VS_REG_HDAT0        0x08
#define VS_REG_HDAT1        0x09
#define VS_REG_AIADDR       0x0A
#define VS_REG_VOL          0x0B

class VS1011_t : public IrqHandler_t {
private:
    Spi_t ISpi{VS_SPI};
    uint32_t ZeroesCount;
    PinIrq_t IDreq{VS_GPIO, VS_DREQ, pudPullDown, this};
    // Cmds
    uint8_t CmdRead(uint8_t AAddr, uint16_t *AData);
    uint8_t CmdWrite(uint8_t AAddr, uint16_t AData);

    volatile uint8_t *ptr;
    volatile uint32_t RemainedSz;
public:
    void Init();
    void Shutdown();
    void SendFirstBuf(uint8_t* ABuf, uint32_t Sz);
    ftVoidVoid OnBufEndI;
    void SetNextBuf(uint8_t* ABuf, uint32_t Sz) {
        ptr = ABuf;
        RemainedSz = Sz;
    }
    // Inner use
    thread_reference_t ThdRef;
    bool IDmaIsIdle;
    void ITask();
    void ISendNexChunk();
    void IIrqHandler();
};

extern VS1011_t VS;
