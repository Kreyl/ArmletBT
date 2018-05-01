/*
 * EvtMsgIDs.h
 *
 *  Created on: 21 апр. 2017 г.
 *      Author: Kreyl
 */

#pragma once

enum EvtMsgId_t {
    evtIdNone = 0, // Always

    // Pretending to eternity
    evtIdShellCmd,
    evtIdEverySecond,

    // Audio
    evtIdSoundPlayStop,
    evtIdSoundFileEnd,

    // Usb
    evtIdUsbConnect,
    evtIdUsbDisconnect,
    evtIdUsbReady,
    evtIdUsbNewCmd,
    evtIdUsbInDone,
    evtIdUsbOutDone,

    // Buttons
    evtIdBtnA,
    evtIdBtnB,
    evtIdBtnC,
    evtIdBtnL,
    evtIdBtnE,
    evtIdBtnR,
    evtIdBtnX,
    evtIdBtnY,
    evtIdBtnZ,

    // App specific
    evtIdAdcRslt,
    // Pill
    evtIdPillConnected,
    evtIdPillDisconnected,
    // Radio
    evtIdNewRPkt,

};
