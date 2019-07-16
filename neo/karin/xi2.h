#ifndef KARIN_XI2_H
#define KARIN_XI2_H

#ifdef _HARMATTAN_PLUS

#include <X11/Xlib.h>

#include "harm.h"

typedef unsigned (* MultiMouseEvent_Function)(int button, unsigned pressed, int x, int y);
typedef unsigned (* MultiMotionEvent_Function)(int button, unsigned pressed, int x, int y, int dx, int dy);

extern Time myxitime;

void karinInitXI2(void);
void karinXI2Atom(void);
Bool karinXI2Event(XEvent *event);
int X11_Pending(Display *display);
void karinSetMultiMouseEventFunction(MultiMouseEvent_Function f);
void karinSetMultiMotionEventFunction(MultiMotionEvent_Function f);

#endif

#endif
