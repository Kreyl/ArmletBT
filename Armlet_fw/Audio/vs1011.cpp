#include <string.h>
#include <vs1011.h>

// Header 44100
#define HDR_SZ      44
const unsigned char header[HDR_SZ] = {
    0x52, 0x49, 0x46, 0x46, 0xff, 0xff, 0xff, 0xff,
    0x57, 0x41, 0x56, 0x45, 0x66, 0x6d, 0x74, 0x20,
    0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00,
    0x44, 0xac, 0x00, 0x00, 0x10, 0xb1, 0x02, 0x00,
    0x04, 0x00, 0x10, 0x00, 0x64, 0x61, 0x74, 0x61,
    0xff, 0xff, 0xff, 0xff
};

VS1011_t VS;

// Pin operations
inline void Rst_Lo()   { PinSetLo(VS_GPIO, VS_RST); }
inline void Rst_Hi()   { PinSetHi(VS_GPIO, VS_RST); }
inline void XCS_Lo()   { PinSetLo(VS_GPIO, VS_XCS); }
inline void XCS_Hi()   { PinSetHi(VS_GPIO, VS_XCS); }
inline void XDCS_Lo()  { PinSetLo(VS_GPIO, VS_XDCS); }
inline void XDCS_Hi()  { PinSetHi(VS_GPIO, VS_XDCS); }

// Mode register
#define VS_MODE_REG_VALUE   0x0802  // Native SDI mode, Layer I + II enabled

#if 1 // ================================= IRQ =================================
void VS1011_t::IIrqHandler() {
//    PrintfI("DreqIrq\r");
    ISendNexChunk();
}

// DMA irq
extern "C"
void SIrqDmaHandler(void *p, uint32_t flags) {
    chSysLockFromISR();
    dmaStreamDisable(VS_DMA);
    VS.IDmaIsIdle = true;
//    PrintfI("DMAIrq\r");
    VS.ISendNexChunk();
    chSysUnlockFromISR();
}
#endif

void VS1011_t::Init() {
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
    uint32_t div;
    if(ISpi.PSpi == SPI1) div = Clk.APB2FreqHz / VS_MAX_SPI_BAUDRATE_HZ;
    else div = Clk.APB1FreqHz / VS_MAX_SPI_BAUDRATE_HZ;
    SpiClkDivider_t ClkDiv = sclkDiv2;
    if     (div > 128) ClkDiv = sclkDiv256;
    else if(div > 64) ClkDiv = sclkDiv128;
    else if(div > 32) ClkDiv = sclkDiv64;
    else if(div > 16) ClkDiv = sclkDiv32;
    else if(div > 8)  ClkDiv = sclkDiv16;
    else if(div > 4)  ClkDiv = sclkDiv8;
    else if(div > 2)  ClkDiv = sclkDiv4;

    ISpi.Setup(boMSB, cpolIdleLow, cphaFirstEdge, ClkDiv);
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
}

void VS1011_t::SendFirstBuf(uint8_t* ABuf, uint32_t Sz) {
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
    IDreq.EnableIrq(IRQ_PRIO_MEDIUM);
    if(RemainedSz == 0) OnBufEndI();
    chSysUnlock();
}

void VS1011_t::ISendNexChunk() {
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
        if(RemainedSz == 0) OnBufEndI();
    }
}

// ==== Commands ====
uint8_t VS1011_t::CmdRead(uint8_t AAddr, uint16_t* AData) {
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
uint8_t VS1011_t::CmdWrite(uint8_t AAddr, uint16_t AData) {
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
