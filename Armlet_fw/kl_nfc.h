/*
 * kl_nfc.h
 *
 *  Created on: 2 џэт. 2018 у.
 *      Author: Kreyl
 */

#pragma once

#include "kl_lib.h"
#include "uart.h"

struct NfcPkt_t {
    uint16_t ID;
    uint8_t Cmd;
    uint8_t Value;
} __packed;

#define NFCPKT_SZ       sizeof(NfcPkt_t)

class KlNfc_t : private BaseUart_t {
private:
    PinOutputPWM_t ITxPin;
    void IOnTxEnd();
public:
    void Init();
    void Transmit(void *Ptr, uint8_t Len);
    uint8_t Receive(uint32_t Timeout_ms, void *Ptr, uint8_t Len);
    KlNfc_t(PwmSetup_t ATxPin, const UartParams_t *APParams) : BaseUart_t(APParams), ITxPin(ATxPin) {}
    // Inner use
    void ITask();
};

extern KlNfc_t Nfc;
