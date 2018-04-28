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

void TmrTimeslotCallback(void *p) {
    chSysLockFromISR();
    chVTSetI(&TmrTimeslot, Radio.TimeslotDuration, TmrTimeslotCallback, nullptr);
    Radio.RMsgQ.SendNowOrExitI(RMsg_t(rmsgNewTimeslot, 0));
    chSysUnlockFromISR();
}

void RxCallback() {
    Radio.RMsgQ.SendNowOrExitI(RMsg_t(rmsgPktRx, 0));
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
    bool IsPwrDown=false, IsRcvng = false;
    systime_t TimeStart = chVTGetSystemTimeX();
    while(true) {
        RMsg_t msg = RMsgQ.Fetch(TIME_IMMEDIATE);
//        if(msg.Cmd == rmsgSetPwr) CC.SetTxPower(msg.Value);
//        else if(msg.Cmd == rmsgSetChnl) CC.SetChannel(msg.Value);
        if(msg.Cmd == rmsgPktRx) {
            IsRcvng = false;
            int8_t Rssi;
            CC.ReadFIFO(&PktRx, &Rssi, RPKT_LEN);
            Printf("Rx: %u @ %d\r", PktRx.ID, Rssi);
        }
        else if(msg.Cmd == rmsgNewTimeslot) {
            // Increase Timeslot and cycle if needed
            TimeSlot++;
            if(TimeSlot >= SLOT_CNT) {
                TimeSlot = 0;
                CycleN++;
                if(CycleN >= CYCLE_CNT) {
                    CycleN = 0;
                    CC.Recalibrate();
                }
                Printf("Ccl %u, dur %u\r", CycleN, chVTTimeElapsedSinceX(TimeStart));
                TimeStart = chVTGetSystemTimeX();
            }
            // Act depending on Cycle and timeslot
            // Tx if timeslot == ID
            if(TimeSlot == ID) {
                IsPwrDown = false;
                IsRcvng = false;
                PktTx.ID = ID;
                PktTx.Cycle = CycleN;
                DBG1_SET();
                CC.Transmit(&PktTx, RPKT_LEN);
                DBG1_CLR();
            }
            else {
                if(CycleN == 0) {
                    if(!IsRcvng) {
                        IsRcvng = true;
                        CC.ReceiveAsync(RxCallback);
                    }
                }
                else { //Cycle != 0, sleep
                    if(!IsPwrDown) {
                        IsPwrDown = true;
                        CC.EnterPwrDown();
                    }
                }
            }
        } // if new timeslot
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
//        TimeslotDuration
        Printf("Timeslot duration, systime: %u\r", TimeslotDuration);
        Printf("Timeslot duration, ms: %u\r", ST2MS(TimeslotDuration));
        TimeslotDuration = 13;
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
