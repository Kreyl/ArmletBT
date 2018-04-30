/*
 * radio_lvl1.cpp
 *
 *  Created on: Nov 17, 2013
 *      Author: kreyl
 */

#include "radio_lvl1.h"
#include "cc1101.h"
#include "MsgQ.h"
#include "led.h"
#include "Sequences.h"
#include "EvtMsgIDs.h"

cc1101_t CC(CC_Setup0);
extern uint16_t ID;

#define DBG_PINS

#ifdef DBG_PINS
#define DBG_GPIO1   GPIOD
#define DBG_PIN1    14
#define DBG1_SET()  PinSetHi(DBG_GPIO1, DBG_PIN1)
#define DBG1_CLR()  PinSetLo(DBG_GPIO1, DBG_PIN1)
//#define DBG_GPIO2   GPIOB
//#define DBG_PIN2    9
//#define DBG2_SET()  PinSet(DBG_GPIO2, DBG_PIN2)
//#define DBG2_CLR()  PinClear(DBG_GPIO2, DBG_PIN2)
#else
#define DBG1_SET()
#define DBG1_CLR()
#endif

rLevel1_t Radio;
virtual_timer_t TmrTimeslot;
static systime_t TimeslotDuration;
static volatile uint32_t CycleN = 0, TimeSlot = 0;
static volatile enum CCState_t {ccstIdle, ccstRx, ccstTx} CCState = ccstIdle;
//uint32_t

void TmrTimeslotCallback(void *p) {
//    DBG1_SET();
    chSysLockFromISR();
    chVTSetI(&TmrTimeslot, TimeslotDuration, TmrTimeslotCallback, nullptr);
    TimeSlot++;
    if(TimeSlot >= SLOT_CNT) {
        TimeSlot = 0;
        CycleN++;
        if(CycleN >= CYCLE_CNT) CycleN = 0;
    }
    if(TimeSlot == ID) Radio.RMsgQ.SendNowOrExitI(RMsg_t(rmsgTimeToTx));
    else { // Not our timeslot
        if(CycleN == 0) { // Enter RX if not yet
            if(CCState != ccstRx) Radio.RMsgQ.SendNowOrExitI(RMsg_t(rmsgTimeToRx));
        }
        else { // CycleN != 0
            if(CCState != ccstIdle) Radio.RMsgQ.SendNowOrExitI(RMsg_t(rmsgTimeToSleep));
        }
    }
    chSysUnlockFromISR();
//    DBG1_CLR();
}

void RxCallback() {
    Radio.RMsgQ.SendNowOrExitI(RMsg_t(rmsgPktRx));
}

#if 1 // ================================ Task =================================
static THD_WORKING_AREA(warLvl1Thread, 256);
__noreturn
static void rLvl1Thread(void *arg) {
    chRegSetThreadName("rLvl1");
    Radio.ITask();
}

__noreturn
void rLevel1_t::ITask() {
//    systime_t TimeStart = chVTGetSystemTimeX();
    while(true) {
        RMsg_t msg = RMsgQ.Fetch(TIME_INFINITE);
        switch(msg.Cmd) {
            case rmsgTimeToTx:
                DBG1_SET();
                CCState = ccstTx;
                PktTx.ID = ID;
                PktTx.Cycle = CycleN;
                PktTx.TimeSourceID = ID;
                DBG1_SET();
                CC.Recalibrate(); // Recalibrate before every TX, do not calibrate before RX
                CC.Transmit(&PktTx, RPKT_LEN);
                DBG1_CLR();
                break;

            case rmsgTimeToRx:
                CCState = ccstRx;
                CC.ReceiveAsync(RxCallback);
                break;

            case rmsgTimeToSleep:
                CCState = ccstIdle;
                CC.EnterIdle();
                break;

            case rmsgPktRx: {
                CCState = ccstIdle;
                int8_t Rssi;
                CC.ReadFIFO(&PktRx, &Rssi, RPKT_LEN);
                Printf("Rx: %u @ %d\r", PktRx.ID, Rssi);
            } break;

            case rmsgSetPwr: CC.SetTxPower(msg.Value); break;
            case rmsgSetChnl: CC.SetChannel(msg.Value); break;
        } // switch
    } // while
}

#endif // task

#if 1 // ============================
uint8_t rLevel1_t::Init() {
#ifdef DBG_PINS
    PinSetupOut(DBG_GPIO1, DBG_PIN1, omPushPull);
//    PinSetupOut(DBG_GPIO2, DBG_PIN2, omPushPull);
#endif    // Init radioIC

    RMsgQ.Init();
    if(CC.Init() == retvOk) {
        CC.SetPktSize(RPKT_LEN);
        CC.SetChannel(RCHNL);
        CC.SetTxPower(CC_TX_PWR);
        // Measure timeslot duration
        CC.SetTxPower(CC_PwrMinus30dBm);
        systime_t TimeStart = chVTGetSystemTimeX();
        CC.Recalibrate();
        CC.Transmit(&PktTx, RPKT_LEN);
        TimeslotDuration = chVTTimeElapsedSinceX(TimeStart);
        Printf("Timeslot duration, systime: %u\r", TimeslotDuration);
        TimeslotDuration = 20;
        chVTSet(&TmrTimeslot, TimeslotDuration, TmrTimeslotCallback, nullptr);

        // Thread
        chThdCreateStatic(warLvl1Thread, sizeof(warLvl1Thread), HIGHPRIO, (tfunc_t)rLvl1Thread, NULL);
        return retvOk;
    }
    else return retvFail;
}

void rLevel1_t::SetChannel(uint8_t NewChannel) {
    CC.SetChannel(NewChannel);
}
#endif
