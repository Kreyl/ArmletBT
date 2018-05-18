/*****************************************************************************
* Model: screen.qm
* File:  ./screen.h
*
* This code has been generated by QM tool (see state-machine.com/qm).
* DO NOT EDIT THIS FILE MANUALLY. All your changes will be lost.
*
* This program is open source software: you can redistribute it and/or
* modify it under the terms of the GNU General Public License as published
* by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
* or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
* for more details.
*****************************************************************************/
/*${.::screen.h} ...........................................................*/
#ifndef screen_h
#define screen_h
#ifdef __cplusplus
extern "C" {
#endif

#include "qhsm.h"    /* include own framework */
#include "localcharacter.h"

#define LOCK_LEVEL 30



class Dispatcher;


/*${SMs::Screen} ...........................................................*/
typedef struct {
/* protected: */
    QHsm super;

/* public: */
    Dispatcher* dispatcher;
    uint8_t timer;
    uint8_t ChargePercent;
    bool DoganPressed;
    bool Background;
} Screen;

/* protected: */
QState Screen_initial(Screen * const me, QEvt const * const e);
QState Screen_global(Screen * const me, QEvt const * const e);
QState Screen_ScreenButtons(Screen * const me, QEvt const * const e);
QState Screen_active(Screen * const me, QEvt const * const e);
QState Screen_disabled(Screen * const me, QEvt const * const e);
QState Screen_locked(Screen * const me, QEvt const * const e);

#ifdef DESKTOP
QState Screen_final(Screen * const me, QEvt const * const e);
#endif /* def DESKTOP */





typedef struct ScreenQEvt {
    QEvt super;
    uint8_t ChargePercent;
    bool Charging;
    bool Connected;
} ScreenQEvt;


/*${SMs::Screen_ctor} ......................................................*/
void Screen_ctor(Screen* me, Dispatcher* dispatcher);

#ifdef __cplusplus
}
#endif
#endif /* screen_h */
