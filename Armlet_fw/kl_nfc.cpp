/*
 * kl_nfc.cpp
 *
 *  Created on: 2 џэт. 2018 у.
 *      Author: Kreyl
 */

#include "kl_nfc.h"
#include "board.h"
#include "crc.h"

#define UART_DMA_TX_MODE(Chnl) \
                            STM32_DMA_CR_CHSEL(Chnl) | \
                            DMA_PRIORITY_LOW | \
                            STM32_DMA_CR_MSIZE_BYTE | \
                            STM32_DMA_CR_PSIZE_BYTE | \
                            STM32_DMA_CR_MINC |       /* Memory pointer increase */ \
                            STM32_DMA_CR_DIR_M2P |    /* Direction is memory to peripheral */ \
                            STM32_DMA_CR_TCIE         /* Enable Transmission Complete IRQ */

#define UART_DMA_RX_MODE(Chnl) \
                            STM32_DMA_CR_CHSEL((Chnl)) | \
                            DMA_PRIORITY_MEDIUM | \
                            STM32_DMA_CR_MSIZE_BYTE | \
                            STM32_DMA_CR_PSIZE_BYTE | \
                            STM32_DMA_CR_MINC |       /* Memory pointer increase */ \
                            STM32_DMA_CR_DIR_P2M |    /* Direction is peripheral to memory */ \
                            STM32_DMA_CR_CIRC         /* Circular buffer enable */


// Settings
static const UartParams_t KlNfcParams = {
        UART4,
        GPIOA, 0,
        GPIOA, 1,
        // DMA
        KLNFC_DMA_TX, KLNFC_DMA_RX,
        UART_DMA_TX_MODE(KLNFC_DMA_CHNL), UART_DMA_RX_MODE(KLNFC_DMA_CHNL),
#if defined STM32F072xB || defined STM32L4XX
        UART_USE_INDEPENDENT_CLK
#endif
};


KlNfc_t Nfc(KLNFC_TX_PIN, &KlNfcParams);
NfcPkt_t Pkt;
static thread_reference_t ThdRef = nullptr;

static THD_WORKING_AREA(waKlNfc, 256);
__noreturn
static void KlNfcThread(void *arg) {
    chRegSetThreadName("KlNfc");
    Nfc.ITask();
}

__noreturn
void KlNfc_t::ITask() {
    while(true) {
        chThdSleepMilliseconds(27);
        Transmit(&Pkt, NFCPKT_SZ);
//        Receive(99, &Pkt, NFCPKT_SZ);
    }
}

void KlNfc_t::Transmit(void *Ptr, uint8_t Len) {
    ITxPin.EnablePin();
    uint8_t *p = (uint8_t*)Ptr;
    for(uint8_t i=0; i<Len; i++) IPutByte(*p++);
    // Calculate and send crc
    uint16_t crc = calc_crc16((char*)Ptr, Len);
    IPutByte((crc >> 8) & 0xFF);
    IPutByte(crc & 0xFF);
    // Enter TX and wait IRQ
    chSysLock();
    IStartTransmissionIfNotYet();
    chThdSuspendS(&ThdRef); // Wait IRQ
    chSysUnlock();          // Will be here when IRQ fires
    ITxPin.DisablePin();
}

uint8_t KlNfc_t::Receive(uint32_t Timeout_ms, void *Ptr, uint8_t Len) {

    return retvOk;
}

static void TCCallback() {
//    PrintfI("TC\r");
    chThdResumeI(&ThdRef, MSG_OK);
}

void KlNfc_t::IOnTxEnd() {
//    PrintfI("DMATxE\r");
    EnableTCIrq(IRQ_PRIO_MEDIUM, TCCallback);
}

void KlNfc_t::Init() {
    // Power pin
    PinSetupOut(GPIOC, 1, omPushPull);
    PinSetHi(GPIOC, 1);
    // TX pin
    ITxPin.Init();
    ITxPin.SetFrequencyHz(1000000);
    ITxPin.Timer.SetTriggerInput(tiETRF);
    ITxPin.Timer.SetEtrPolarity(invInverted);
    ITxPin.Timer.SelectSlaveMode(smGated);
    ITxPin.Set(2);
    ITxPin.DisablePin();
    // Modulation input pin: T2 C1
    PinSetupAlterFunc(GPIOA, 15, omPushPull, pudNone, AF1);
    // UART
    BaseUart_t::Init(10000);

    Pkt.ID = 7;
    chThdCreateStatic(waKlNfc, sizeof(waKlNfc), NORMALPRIO, (tfunc_t)KlNfcThread, NULL);
}
