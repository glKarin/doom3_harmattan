/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/
#include "../../idlib/precompiled.h"
#include "posix_public.h"

typedef struct poll_keyboard_event_s {
	int key;
	bool state;
} poll_keyboard_event_t;

typedef struct poll_mouse_event_s {
	int action;
	int value;
} poll_mouse_event_t;

#define MAX_POLL_EVENTS 50
#define POLL_EVENTS_HEADROOM 2 // some situations require to add several events
static poll_keyboard_event_t poll_events_keyboard[MAX_POLL_EVENTS + POLL_EVENTS_HEADROOM];
static int poll_keyboard_event_count;
static poll_mouse_event_t poll_events_mouse[MAX_POLL_EVENTS + POLL_EVENTS_HEADROOM];
static int poll_mouse_event_count;

/*
==========
Posix_AddKeyboardPollEvent
==========
*/
bool Posix_AddKeyboardPollEvent(int key, bool state)
{
	if (poll_keyboard_event_count >= MAX_POLL_EVENTS + POLL_EVENTS_HEADROOM)
		common->FatalError("poll_keyboard_event_count exceeded MAX_POLL_EVENT + POLL_EVENTS_HEADROOM\n");

	poll_events_keyboard[poll_keyboard_event_count].key = key;
	poll_events_keyboard[poll_keyboard_event_count++].state = state;

	if (poll_keyboard_event_count >= MAX_POLL_EVENTS) {
		common->DPrintf("WARNING: reached MAX_POLL_EVENT poll_keyboard_event_count\n");
		return false;
	}

	return true;
}

/*
==========
Posix_AddMousePollEvent
==========
*/
bool Posix_AddMousePollEvent(int action, int value)
{
	if (poll_mouse_event_count >= MAX_POLL_EVENTS + POLL_EVENTS_HEADROOM)
		common->FatalError("poll_mouse_event_count exceeded MAX_POLL_EVENT + POLL_EVENTS_HEADROOM\n");

	poll_events_mouse[poll_mouse_event_count].action = action;
	poll_events_mouse[poll_mouse_event_count++].value = value;

	if (poll_mouse_event_count >= MAX_POLL_EVENTS) {
		common->DPrintf("WARNING: reached MAX_POLL_EVENT poll_mouse_event_count\n");
		return false;
	}

	return true;
}

/*
===========================================================================
polled from GetDirectUsercmd
async input polling is obsolete
we have a single entry point for both mouse and keyboard
the mouse/keyboard seperation is API legacy
===========================================================================
*/

int Sys_PollKeyboardInputEvents(void)
{
	return poll_keyboard_event_count;
}

int Sys_ReturnKeyboardInputEvent(const int n, int &key, bool &state)
{
	if (n >= poll_keyboard_event_count) {
		return 0;
	}

	key = poll_events_keyboard[n].key;
	state = poll_events_keyboard[n].state;
	return 1;
}

void Sys_EndKeyboardInputEvents(void)
{
	//isn't this were it's supposed to be, was missing some key strokes with it set below
	poll_keyboard_event_count = 0;
}

int Sys_PollMouseInputEvents(void)
{
#if 0 //moved to the Sys_End functions
	poll_keyboard_event_count = 0;
	poll_mouse_event_count = 0;
#endif

#ifndef __ANDROID__2
	// that's OS specific, implemented in osx/ and linux/
	Posix_PollInput();
#endif

	return poll_mouse_event_count;
}

int	Sys_ReturnMouseInputEvent(const int n, int &action, int &value)
{
	if (n>=poll_mouse_event_count) {
		return 0;
	}

	action = poll_events_mouse[ n ].action;
	value = poll_events_mouse[ n ].value;
	return 1;
}

void Sys_EndMouseInputEvents(void)
{
	// moved out of the Sys_PollMouseInputEvents
	poll_mouse_event_count = 0;
}

#ifdef _HARMATTAN_PLUS
#define MAX_ACTION_EVENT_LENGTH 64
#define MAX_TOUCH_EVENT_LENGTH 196
typedef struct poll_mouse_event_s karin_TouchEvent;
typedef struct _karin_ActionEvent
{
	unsigned action;
	bool triggered;
} karin_ActionEvent;
typedef struct _karin_ActionEventQueue
{
	karin_ActionEvent events[MAX_ACTION_EVENT_LENGTH];
	unsigned int length;
} karin_ActionEventQueue;
typedef struct _karin_TouchEventQueue
{
	karin_TouchEvent events[MAX_TOUCH_EVENT_LENGTH];
	unsigned int length;
} karin_TouchEventQueue;
static karin_ActionEventQueue action_event_queue;
static karin_TouchEventQueue touch_event_queue;
#define QUEUE_END(q) ((q).events + (q).length)
// Add
bool karinAddActionPollEvent(unsigned action, bool state)
{
	if (action_event_queue.length >= MAX_ACTION_EVENT_LENGTH)
	{
		common->DPrintf( "[Harmattan]: Action event queue is overflow\n"); // do not fatal error
		//action_event_queue.length = MAX_ACTION_EVENT_LENGTH - 1;
		return false;
	}
	karin_ActionEvent *ev = QUEUE_END(action_event_queue);
	ev->action = action;
	ev->triggered = state;
	action_event_queue.length++;
	if (action_event_queue.length >= MAX_ACTION_EVENT_LENGTH)
	{
		common->DPrintf("[Harmattan]: Action event queue is full.\n");
		//return false;
	}
	return true;
}

bool karinAddTouchPollEvent(unsigned action, int d)
{
	if (touch_event_queue.length >= MAX_TOUCH_EVENT_LENGTH)
	{
		common->DPrintf( "[Harmattan]: Touch event queue is overflow\n"); // do not fatal error
		//touch_event_queue.length = MAX_TOUCH_EVENT_LENGTH - 1;
		return false;
	}
	karin_TouchEvent *ev = QUEUE_END(touch_event_queue);
	ev->action = action;
	ev->value = d;
	touch_event_queue.length++;
	if (touch_event_queue.length >= MAX_TOUCH_EVENT_LENGTH)
	{
		common->DPrintf("[Harmattan]: Touch event queue is full.\n");
		//return false;
	}
	return true;
}

unsigned karinPollActionInputEvents(void)
{
	return action_event_queue.length;
}

unsigned karinPollTouchInputEvents(void)
{
	return touch_event_queue.length;
}

bool karinReturnActionInputEvent(const int n, unsigned &action, bool &state)
{
	if (n >= action_event_queue.length || n < 0)
	{
		return false;
	}
	const karin_ActionEvent *ev = action_event_queue.events + n;
	action = ev->action;
	state = ev->triggered;
	return true;
}

bool karinReturnTouchInputEvent(const int n, unsigned &action, int &value)
{
	if (n >= touch_event_queue.length || n < 0)
	{
		return false;
	}
	const karin_TouchEvent *ev = touch_event_queue.events + n;
	action = ev->action;
	value = ev->value;
	return true;
}

void karinEndActionInputEvents(void)
{
	memset(&action_event_queue, 0, sizeof(karin_ActionEventQueue));
	//action_event_queue.length = 0;
}

void karinEndTouchInputEvents(void)
{
	memset(&touch_event_queue, 0, sizeof(karin_TouchEventQueue));
	//touch_event_queue.length = 0;
}

#endif
