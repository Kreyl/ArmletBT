#include <string.h>
#include <vs1011.h>

#define HDR_SZ      44
const unsigned char header[HDR_SZ] = {
    0x52, 0x49, 0x46, 0x46, 0xff, 0xff, 0xff, 0xff,
    0x57, 0x41, 0x56, 0x45, 0x66, 0x6d, 0x74, 0x20,
    0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00,
    0x80, 0xbb, 0x00, 0x00, 0x00, 0xee, 0x02, 0x00,
    0x04, 0x00, 0x10, 0x00, 0x64, 0x61, 0x74, 0x61,
    0xff, 0xff, 0xff, 0xff
};

int16_t Saw[48*2];

Sound_t Sound;

// Pin operations
inline void Rst_Lo()   { PinSetLo(VS_GPIO, VS_RST); }
inline void Rst_Hi()   { PinSetHi(VS_GPIO, VS_RST); }
inline void XCS_Lo()   { PinSetLo(VS_GPIO, VS_XCS); }
inline void XCS_Hi()   { PinSetHi(VS_GPIO, VS_XCS); }
inline void XDCS_Lo()  { PinSetLo(VS_GPIO, VS_XDCS); }
inline void XDCS_Hi()  { PinSetHi(VS_GPIO, VS_XDCS); }

// Mode register
#define VS_MODE_REG_VALUE   0x0802  // Native SDI mode, Layer I + II enabled

// After file end, send several zeroes
//#define ZERO_SEQ_LEN        128
//static const uint8_t SZero = 0;



// ================================= IRQ =======================================
void Sound_t::IIrqHandler() {
//    PrintfI("DreqIrq\r");
    ISendNexChunk();
}

extern "C" {
// DMA irq
void SIrqDmaHandler(void *p, uint32_t flags) {
    chSysLockFromISR();
    dmaStreamDisable(VS_DMA);
    Sound.IDmaIsIdle = true;
//    PrintfI("DMAIrq\r");
    Sound.ISendNexChunk();
    chSysUnlockFromISR();
}
} // extern c

// =========================== Implementation ==================================
void Sound_t::Init() {
    // ==== GPIO init ====
    PinSetupOut(VS_GPIO, VS_RST, omPushPull);
    PinSetupOut(VS_GPIO, VS_XCS, omPushPull);
    PinSetupOut(VS_GPIO, VS_XDCS, omPushPull);
    Rst_Lo();
    XCS_Hi();
    XDCS_Hi();
    PinSetupAlterFunc(VS_GPIO, VS_XCLK, omPushPull, pudNone, VS_AF);
    PinSetupAlterFunc(VS_GPIO, VS_SO,   omPushPull, pudNone, VS_AF);
    PinSetupAlterFunc(VS_GPIO, VS_SI,   omPushPull, pudNone, VS_AF);
    // DREQ IRQ
    IDreq.Init(ttRising);

    // ==== SPI init ====
    ISpi.Setup(boMSB, cpolIdleLow, cphaFirstEdge, sclkDiv8);
    ISpi.Enable();
    ISpi.EnableTxDma();

    // ==== DMA ====
    // Here only unchanged parameters of the DMA are configured.
    dmaStreamAllocate     (VS_DMA, IRQ_PRIO_MEDIUM, SIrqDmaHandler, NULL);
    dmaStreamSetPeripheral(VS_DMA, &VS_SPI->DR);
    dmaStreamSetMode      (VS_DMA, VS_DMA_MODE);
    IDmaIsIdle = true;

    // ==== Init VS ====
    Rst_Hi();
    Clk.EnableMCO1(mco1HSE, mcoDiv1);   // Only after reset, as pins are grounded when Rst is Lo
    chThdSleepMicroseconds(45);

    // Send init commands
    CmdWrite(VS_REG_MODE, VS_MODE_REG_VALUE);
    CmdWrite(VS_REG_CLOCKF, (0x8000 + (12000000/2000)));
    CmdWrite(VS_REG_VOL, 0);

    XDCS_Lo();  // Start data transmission
    // Send header
    ptr = (uint8_t*)header;
    for(int i=0; i<HDR_SZ; i++) {
        while(!IDreq.IsHi());
        ISpi.ReadWriteByte(*ptr++);
    }

    // Fill Saw
    int n=0;
    for(int i=-23000; i<23000; i+=1000) {
        Saw[n++] = i;
        Saw[n++] = i;
    }

    while(true) {
        SendBuf((uint8_t*)Saw, 192);
        while(!BufSent);
    }
}

// ================================ Inner use ==================================
void Sound_t::SendBuf(uint8_t* ABuf, uint32_t Sz) {
    while(!IDreq.IsHi());
    uint32_t Sz2Send = MIN_(Sz, 32);
    chSysLock();
    ptr = ABuf;
    dmaStreamSetMemory0(VS_DMA, ptr);
    dmaStreamSetTransactionSize(VS_DMA, Sz2Send);
    dmaStreamSetMode(VS_DMA, VS_DMA_MODE | STM32_DMA_CR_MINC);  // Memory pointer increase
    dmaStreamEnable(VS_DMA);
    IDmaIsIdle = false;
    RemainedSz = Sz - Sz2Send;
    ptr += Sz2Send;
    if(RemainedSz > 0) {
        IDreq.EnableIrq(IRQ_PRIO_MEDIUM);
        BufSent = false;
    }
    else BufSent = true;
    chSysUnlock();
}

void Sound_t::ISendNexChunk() {
    if(IDmaIsIdle and IDreq.IsHi()) {
        dmaStreamDisable(VS_DMA);
        if(RemainedSz > 0) {
            uint32_t Sz2Send = MIN_(RemainedSz, 32);
            dmaStreamSetMemory0(VS_DMA, ptr);
            dmaStreamSetTransactionSize(VS_DMA, Sz2Send);
            dmaStreamSetMode(VS_DMA, VS_DMA_MODE | STM32_DMA_CR_MINC);  // Memory pointer increase
            dmaStreamEnable(VS_DMA);
            RemainedSz -= Sz2Send;
            ptr += Sz2Send;
        }
        else {
            IDreq.DisableIrq();
            BufSent = true;
        }
    }
}

// ==== Commands ====
uint8_t Sound_t::CmdRead(uint8_t AAddr, uint16_t* AData) {
//    uint8_t IReply;
    uint16_t IData;
    // Wait until ready
    //if ((IReply = BusyWait()) != OK) return IReply; // Get out in case of timeout
    XCS_Lo();   // Start transmission
    ISpi.ReadWriteByte(VS_READ_OPCODE);  // Send operation code
    ISpi.ReadWriteByte(AAddr);           // Send addr
    *AData = ISpi.ReadWriteByte(0);      // Read upper byte
    *AData <<= 8;
    IData = ISpi.ReadWriteByte(0);       // Read lower byte
    *AData += IData;
    XCS_Hi();   // End transmission
    return retvOk;
}
uint8_t Sound_t::CmdWrite(uint8_t AAddr, uint16_t AData) {
//    uint8_t IReply;
    // Wait until ready
//    if ((IReply = BusyWait()) != OK) return IReply; // Get out in case of timeout
    XCS_Lo();                       // Start transmission
    ISpi.ReadWriteByte(VS_WRITE_OPCODE); // Send operation code
    ISpi.ReadWriteByte(AAddr);           // Send addr
    ISpi.ReadWriteByte(AData >> 8);      // Send upper byte
    ISpi.ReadWriteByte(0x00FF & AData);  // Send lower byte
    XCS_Hi();                       // End transmission
    return retvOk;
}
