#ifdef _HARMATTAN_PLUS
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "xi2.h"
#include "gl_vkb.h"

idCVar in_mouse( "in_mouse", "1", CVAR_SYSTEM | CVAR_ARCHIVE, "" );
idCVar in_dgamouse( "in_dgamouse", "1", CVAR_SYSTEM | CVAR_ARCHIVE, "" );
idCVar in_nograb( "in_nograb", "0", CVAR_SYSTEM | CVAR_NOCHEAT, "" );

#ifdef _HARMATTAN_PLUS
idCVar h_cUsingVKB( "h_cUsingVKB", "1", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_BOOL, "[harmattan] - Rendering virtual buttons layer and response events: 0 / 1" );
idCVar h_cSwipeSens( "h_cSwipeSens", "1.0", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_FLOAT, "[harmattan] - Swipe sensitivity on virtual buttons layer: float." );
#endif

// have a working xkb extension
static bool have_xkb = false;

// toggled by grab calls - decides if we ignore MotionNotify events
static bool mouse_active = false;

// non-DGA pointer-warping mouse input
static int mwx, mwy;
static int mx = 0, my = 0;

// time mouse was last reset, we ignore the first 50ms of the mouse to allow settling of events
static int mouse_reset_time = 0;
#define MOUSE_RESET_DELAY 50

// backup original values for pointer grab/ungrab
static int mouse_accel_numerator;
static int mouse_accel_denominator;
static int mouse_threshold;

#ifdef _HARMATTAN_PLUS
static byte s_scantokey[128] = {
/*  0 */ 0, 0, 0, 0, 0, 0/*harm_Fn 133*/, 0, K_TAB/*0*//*harm_Sym 135*/,
/*  8 */ 0, 27, '1', '2', '3', '4', '5', '6', // 27 - ESC
/* 10 16 */ '7', '8', '9', '0', '-', '=', K_BACKSPACE, 9, // 9 - TAB
/* 18 24 */ 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',
/* 20 32 */ 'o', 'p', '[', ']', K_ENTER, K_CTRL, 'a', 's',
/* 28 40 */ 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
/* 30 48*/ '\'', '`', K_SHIFT, '\\'/*harm_?*/, 'z', 'x', 'c', 'v',
/* 38 56 */ 'b', 'n', 'm', ',', '.', '/'/*harm_@*/, K_SHIFT, K_KP_STAR,
/* 40 64 */ K_ALT, ' ', K_CAPSLOCK, K_F1, K_F2, K_F3, K_F4, K_F5,
/* 48 72 */ K_F6, K_F7, K_F8, K_F9, K_F10, K_PAUSE, 0, K_HOME,
/* 50 80 */ K_UPARROW, K_PGUP, K_KP_MINUS, K_LEFTARROW, K_KP_5, K_RIGHTARROW, K_KP_PLUS, K_END,
/* 58 88 */ K_DOWNARROW, K_PGDN, K_INS, K_DEL, 0, 0, '\\', K_F11,
/* 60 96 */ K_F12, K_HOME, K_UPARROW, K_PGUP, K_LEFTARROW, 0, K_RIGHTARROW, K_END,
/* 68 104 */ K_DOWNARROW, K_CTRL/*K_PGDN*/, K_INS, K_DEL, K_ENTER, K_CTRL, K_PAUSE, K_UPARROW,
/* 70 112 */ '/', K_LEFTARROW, K_RIGHTARROW/*K_ALT*/, 0, K_DOWNARROW, 0, 0, 0,
/* 78 120 */ 0, 0, 0, 0, 0, 0, 0, 0
};
#else
static byte s_scantokey[128] = {
/*  0 */ 0, 0, 0, 0, 0, 0, 0, 0,
/*  8 */ 0, 27, '1', '2', '3', '4', '5', '6', // 27 - ESC
/* 10 */ '7', '8', '9', '0', '-', '=', K_BACKSPACE, 9, // 9 - TAB
/* 18 */ 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',
/* 20 */ 'o', 'p', '[', ']', K_ENTER, K_CTRL, 'a', 's',
/* 28 */ 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
/* 30 */ '\'', '`', K_SHIFT, '\\', 'z', 'x', 'c', 'v',
/* 38 */ 'b', 'n', 'm', ',', '.', '/', K_SHIFT, K_KP_STAR,
/* 40 */ K_ALT, ' ', K_CAPSLOCK, K_F1, K_F2, K_F3, K_F4, K_F5,
/* 48 */ K_F6, K_F7, K_F8, K_F9, K_F10, K_PAUSE, 0, K_HOME,
/* 50 */ K_UPARROW, K_PGUP, K_KP_MINUS, K_LEFTARROW, K_KP_5, K_RIGHTARROW, K_KP_PLUS, K_END,
/* 58 */ K_DOWNARROW, K_PGDN, K_INS, K_DEL, 0, 0, '\\', K_F11,
/* 60 */ K_F12, K_HOME, K_UPARROW, K_PGUP, K_LEFTARROW, 0, K_RIGHTARROW, K_END,
/* 68 */ K_DOWNARROW, K_PGDN, K_INS, K_DEL, K_ENTER, K_CTRL, K_PAUSE, 0,
/* 70 */ '/', K_ALT, 0, 0, 0, 0, 0, 0,
/* 78 */ 0, 0, 0, 0, 0, 0, 0, 0
};
#endif

/*
=================
glBegin(karin_FUNC)
=================
*/
static int karinSwipeSens(int x)
{
#define FLOAT_TO_INT
	float sens = h_cSwipeSens.GetFloat();
	return (int)FLOAT_TO_INT((float)x * sens);
#undef FLOAT_TO_INT
}

static unsigned karinHandleVKBAction(int action, unsigned pressed, int dx, int dy)
{
#define		MAXCMDLINE	256
#define		MAXCMDLENGTH 1024
	static int _keys[MAXCMDLINE];
	static char _cmd[MAXCMDLENGTH];
	unsigned int key_count = 0;

	int r = karinGetActionData(action, _keys, MAXCMDLINE, &key_count, _cmd, MAXCMDLENGTH);
	if(r == Cmd_Data)
	{
		return karinAddActionPollEvent(_keys[0], pressed != 0);
	}
	else if(r == Key_Data)
	{
		int i = 0;
		for(i = 0; i < key_count; i++)
		{
			Posix_QueEvent(SE_KEY, _keys[i], pressed != 0, 0, NULL);
		}
		return 1;
	}
	else if(r == Char_Data)
	{
		if(!pressed)
		{
			int i = 0;
			for(i = 0; i < key_count; i++)
			{
				{
					Posix_QueEvent(SE_CHAR, _keys[i], 0, 0, NULL);
				}
			}
		}
		return 1;
	}
	else if(r == Button_Data)
	{
		if(pressed)
		{
			int x = 0;
			//Posix_QueEvent(SE_MOUSE, dx, -dy, 0, NULL);
			if(dx != 0)
			{
				x = karinSwipeSens(dx);
				if(x != 0)
					karinAddTouchPollEvent(M_DELTAX, x);
			}
			if(dy != 0)
			{
				x = karinSwipeSens(-dy);
				if(dy != 0)
					karinAddTouchPollEvent(M_DELTAY, x);
			}
		}
	}

	return 0;
#undef MAXCMDLINE
#undef MAXCMDLENGTH
}

Bool karinXI2MouseEvent(int button, Bool pressed, int x, int y)
{
	if(h_cUsingVKB.GetBool() && karinVKBMouseEvent(button, pressed, x, glConfig.vidHeight - y, karinHandleVKBAction))
		return True;
	Posix_QueEvent( SE_KEY, K_MOUSE1, pressed == True, 0, NULL);
	return True;
}

Bool karinXI2MotionEvent(int button, Bool pressed, int x, int y, int dx, int dy)
{
	if(h_cUsingVKB.GetBool() && karinVKBMouseMotionEvent(button, pressed, x, glConfig.vidHeight - y, dx, -dy, karinHandleVKBAction))
		return True;
	Posix_QueEvent( SE_TOUCH, x, y, 0, NULL);
	return True;
}
/*
=================
glEnd(karin_FUNC)
=================
*/

/*
=================
IN_Clear_f
=================
*/
void IN_Clear_f( const idCmdArgs &args ) {
	idKeyInput::ClearStates();
}

/*
=================
Sys_InitInput
=================
*/
void Sys_InitInput(void) {
	int major_in_out, minor_in_out, opcode_rtrn, event_rtrn, error_rtrn;
	bool ret;

	common->Printf( "\n------- Input Initialization -------\n" );
	assert( dpy );
	cmdSystem->AddCommand( "in_clear", IN_Clear_f, CMD_FL_SYSTEM, "reset the input keys" );
	major_in_out = XkbMajorVersion;
	minor_in_out = XkbMinorVersion;
	ret = XkbLibraryVersion( &major_in_out, &minor_in_out );
	common->Printf( "XKB extension: compile time 0x%x:0x%x, runtime 0x%x:0x%x: %s\n", XkbMajorVersion, XkbMinorVersion, major_in_out, minor_in_out, ret ? "OK" : "Not compatible" );
	if ( ret ) {
		ret = XkbQueryExtension( dpy, &opcode_rtrn, &event_rtrn, &error_rtrn, &major_in_out, &minor_in_out );
		if ( ret ) {
			common->Printf( "XKB extension present on server ( 0x%x:0x%x )\n", major_in_out, minor_in_out );
			have_xkb = true;
		} else {
			common->Printf( "XKB extension not present on server\n" );
			have_xkb = false;
		}
	} else {
		have_xkb = false;
	}
	common->Printf( "------------------------------------\n" );
}

//#define XEVT_DBG
//#define XEVT_DBG2

static Cursor Sys_XCreateNullCursor( Display *display, Window root ) {
	Pixmap cursormask; 
	XGCValues xgc;
	GC gc;
	XColor dummycolour;
	Cursor cursor;

	cursormask = XCreatePixmap(display, root, 1, 1, 1/*depth*/);
	xgc.function = GXclear;
	gc =  XCreateGC(display, cursormask, GCFunction, &xgc);
	XFillRectangle(display, cursormask, gc, 0, 0, 1, 1);
	dummycolour.pixel = 0;
	dummycolour.red = 0;
	dummycolour.flags = 04;
	cursor = XCreatePixmapCursor(display, cursormask, cursormask,
								 &dummycolour,&dummycolour, 0,0);
	XFreePixmap(display,cursormask);
	XFreeGC(display,gc);
	return cursor;
}

static void Sys_XInstallGrabs( void ) {
	assert( dpy );

	XWarpPointer( dpy, None, win,
				 0, 0, 0, 0,
				 glConfig.vidWidth / 2, glConfig.vidHeight / 2 );

	XSync( dpy, False );

	XDefineCursor( dpy, win, Sys_XCreateNullCursor( dpy, win ) );
	
#ifndef _HARMATTAN_PLUS
	XGrabPointer( dpy, win,
				 False,
				 MOUSE_MASK,
				 GrabModeAsync, GrabModeAsync,
				 win,
				 None,
				 CurrentTime );
#endif

	XGetPointerControl( dpy, &mouse_accel_numerator, &mouse_accel_denominator,
					   &mouse_threshold );
	
	XChangePointerControl( dpy, True, True, 1, 1, 0 );
	
	XSync( dpy, False );
	
	mouse_reset_time = Sys_Milliseconds ();
	
	in_dgamouse.SetBool( false ); // DGA not support
	
	if ( in_dgamouse.GetBool() ) {
#if defined( ID_ENABLE_DGA )
		XF86DGADirectVideo( dpy, DefaultScreen( dpy ), XF86DGADirectMouse );
		XWarpPointer( dpy, None, win, 0, 0, 0, 0, 0, 0 );
#endif
	} else {
		mwx = glConfig.vidWidth / 2;
		mwy = glConfig.vidHeight / 2;
		mx = my = 0;
	}
	
	XGrabKeyboard( dpy, win,
				  False,
				  GrabModeAsync, GrabModeAsync,
				  CurrentTime );
	
	XSync( dpy, False );

	mouse_active = true;
}

void Sys_XUninstallGrabs(void) {
	assert( dpy );

#if defined( ID_ENABLE_DGA )
	if ( in_dgamouse.GetBool() ) {
		common->DPrintf( "DGA Mouse - Disabling DGA DirectVideo\n" );
		XF86DGADirectVideo( dpy, DefaultScreen( dpy ), 0 );
	}
#endif
	
	XChangePointerControl( dpy, true, true, mouse_accel_numerator, 
						  mouse_accel_denominator, mouse_threshold );
	
	XUngrabPointer( dpy, CurrentTime );
	XUngrabKeyboard( dpy, CurrentTime );
	
	XWarpPointer( dpy, None, win,
				 0, 0, 0, 0,
				 glConfig.vidWidth / 2, glConfig.vidHeight / 2);
	
	XUndefineCursor( dpy, win );

	mouse_active = false;
}

void Sys_GrabMouseCursor( bool grabIt ) {

#if defined( ID_DEDICATED )
	return;
#endif

	if ( !dpy ) {
		#ifdef XEVT_DBG
			common->DPrintf("Sys_GrabMouseCursor: !dpy\n");
		#endif
		return;
	}
	
	if ( glConfig.isFullscreen ) {
		if ( !grabIt ) {
			return; // never ungrab while fullscreen
		}
		if ( in_nograb.GetBool() ) {
			common->DPrintf("forcing in_nograb 0 while running fullscreen\n");
			in_nograb.SetBool( false );
		}
	}
	
	if ( in_nograb.GetBool() ) {
		if ( in_dgamouse.GetBool() ) {
			common->DPrintf("in_nograb 1, forcing forcing DGA mouse off\n");
			in_dgamouse.SetBool( false );
		}
		if (grabIt) {
			mouse_active = true;
		} else {
			mouse_active = false;
		}
		return;
	}

	if ( grabIt && !mouse_active ) {
		Sys_XInstallGrabs();
	} else if ( !grabIt && mouse_active ) {
		Sys_XUninstallGrabs();
	}
}

/**
 * XPending() actually performs a blocking read 
 *  if no events available. From Fakk2, by way of
 *  Heretic2, by way of SDL, original idea GGI project.
 * The benefit of this approach over the quite
 *  badly behaved XAutoRepeatOn/Off is that you get
 *  focus handling for free, which is a major win
 *  with debug and windowed mode. It rests on the
 *  assumption that the X server will use the
 *  same timestamp on press/release event pairs 
 *  for key repeats. 
 */
static bool Sys_XPendingInput( void ) {
	// Flush the display connection
	//  and look to see if events are queued
	XFlush( dpy );
	if ( XEventsQueued( dpy, QueuedAlready) ) {
		return true;
	}

	// More drastic measures are required -- see if X is ready to talk
	static struct timeval zero_time;
	int x11_fd;
	fd_set fdset;

    x11_fd = ConnectionNumber( dpy );
    FD_ZERO( &fdset );
    FD_SET( x11_fd, &fdset );
    if ( select( x11_fd+1, &fdset, NULL, NULL, &zero_time ) == 1 ) {
		return XPending( dpy );
    }

	// Oh well, nothing is ready ..
	return false;
}

/**
 * Intercept a KeyRelease-KeyPress sequence and ignore
 */
static bool Sys_XRepeatPress( XEvent *event ) {
	XEvent	peekevent;
	bool	repeated = false;
	int		lookupRet;
	char	buf[5];
	KeySym	keysym;

	if ( Sys_XPendingInput() ) {
		XPeekEvent( dpy, &peekevent );

		if ((peekevent.type == KeyPress) &&
			(peekevent.xkey.keycode == event->xkey.keycode) &&
			(peekevent.xkey.time == event->xkey.time)) {
			repeated = true;
			XNextEvent( dpy, &peekevent );
			// emit an SE_CHAR for the repeat
			lookupRet = XLookupString( (XKeyEvent*)&peekevent, buf, sizeof(buf), &keysym, NULL );
			if (lookupRet > 0) {
				Posix_QueEvent( SE_CHAR, buf[ 0 ], 0, 0, NULL);
			} else {
				// shouldn't we be doing a release/press in this order rather?
				// ( doesn't work .. but that's what I would have expected to do though )
				Posix_QueEvent( SE_KEY, s_scantokey[peekevent.xkey.keycode], true, 0, NULL);
				Posix_QueEvent( SE_KEY, s_scantokey[peekevent.xkey.keycode], false, 0, NULL);
			}
		}
  	}

	return repeated;
}

/*
==========================
Posix_PollInput
==========================
*/
void Posix_PollInput() {
	static char buf[16];
	static XEvent event;
	static XKeyEvent *key_event = (XKeyEvent*)&event;
  	int lookupRet;
	int b, dx, dy;
	KeySym keysym;	
	
	if ( !dpy ) {
		return;
	}
	
	// NOTE: Sys_GetEvent only calls when there are no events left
	// but here we pump all X events that have accumulated
	// pump one by one? or use threaded input?
#ifdef _HARMATTAN_PLUS
	while ( X11_Pending( dpy ) ) 
#else
	while ( XPending( dpy ) ) 
#endif
	{
		XNextEvent( dpy, &event );
		switch (event.type) {
			case KeyPress:
				#ifdef XEVT_DBG
				if (key_event->keycode > 0x7F)
					common->DPrintf("WARNING: KeyPress keycode > 0x7F");
				#endif
					//printf("SE_KEY press %c  0x%X ox%X | %d %d\n", key_event->keycode, key_event->keycode, key_event->keycode & 0x7F, key_event->keycode, key_event->keycode & 0x7F);
				key_event->keycode &= 0x7F;
				#ifdef XEVT_DBG2
					printf("SE_KEY press %d\n", key_event->keycode);
				#endif
				Posix_QueEvent( SE_KEY, s_scantokey[key_event->keycode], true, 0, NULL);
				lookupRet = XLookupString(key_event, buf, sizeof(buf), &keysym, NULL);
				if (lookupRet > 0) {
					char s = buf[0];
					#ifdef XEVT_DBG
						if (buf[1]!=0)
							common->DPrintf("WARNING: got XLookupString buffer '%s' (%d)\n", buf, strlen(buf));
					#endif
					#ifdef XEVT_DBG2
						printf("SE_CHAR %s\n", buf);
					#endif
					Posix_QueEvent( SE_CHAR, s, 0, 0, NULL);
				}
				if (!Posix_AddKeyboardPollEvent( s_scantokey[key_event->keycode], true ))
					return;
			break;			
				
			case KeyRelease:
				if (Sys_XRepeatPress(&event)) {
					#ifdef XEVT_DBG2
						printf("RepeatPress\n");
					#endif
					continue;
				}
				#ifdef XEVT_DBG
				if (key_event->keycode > 0x7F)
					common->DPrintf("WARNING: KeyRelease keycode > 0x7F");
				#endif
				key_event->keycode &= 0x7F;
				#ifdef XEVT_DBG2
					printf("SE_KEY release %d\n", key_event->keycode);
				#endif
				Posix_QueEvent( SE_KEY, s_scantokey[key_event->keycode], false, 0, NULL);
				if (!Posix_AddKeyboardPollEvent( s_scantokey[key_event->keycode], false ))
					return;
			break;
				
#ifndef _HARMATTAN_PLUS
			case ButtonPress:
				if (event.xbutton.button == 4) {
					Posix_QueEvent( SE_KEY, K_MWHEELUP, true, 0, NULL);
					if (!Posix_AddMousePollEvent( M_DELTAZ, 1 ))
						return;
				} else if (event.xbutton.button == 5) {
					Posix_QueEvent( SE_KEY, K_MWHEELDOWN, true, 0, NULL);
					if (!Posix_AddMousePollEvent( M_DELTAZ, -1 ))
						return;
				} else {
#ifndef _HARMATTAN_PLUS
					b = -1;
					if (event.xbutton.button == 1) {
						b = 0;		// K_MOUSE1
					} else if (event.xbutton.button == 2) {
						b = 2;		// K_MOUSE3
					} else if (event.xbutton.button == 3) {
						b = 1;		// K_MOUSE2
					} else if (event.xbutton.button == 6) {
						b = 3;		// K_MOUSE4
					} else if (event.xbutton.button == 7) {
						b = 4;		// K_MOUSE5
					}
					if (b == -1 || b > 4) {
						common->DPrintf("X ButtonPress %d not supported\n", event.xbutton.button);
					} else {
						Posix_QueEvent( SE_KEY, K_MOUSE1 + b, true, 0, NULL);
						if (!Posix_AddMousePollEvent( M_ACTION1 + b, true ))
							return;
					}
#endif
				}
			break;

			case ButtonRelease:
				if (event.xbutton.button == 4) {
					Posix_QueEvent( SE_KEY, K_MWHEELUP, false, 0, NULL);
				} else if (event.xbutton.button == 5) {
					Posix_QueEvent( SE_KEY, K_MWHEELDOWN, false, 0, NULL);
				} else {
#ifndef _HARMATTAN_PLUS
					b = -1;
					if (event.xbutton.button == 1) {
						b = 0;
					} else if (event.xbutton.button == 2) {
						b = 2;
					} else if (event.xbutton.button == 3) {
						b = 1;
					} else if (event.xbutton.button == 6) {
						b = 3;		// K_MOUSE4
					} else if (event.xbutton.button == 7) {
						b = 4;		// K_MOUSE5
					}
					if (b == -1 || b > 4) {
						common->DPrintf("X ButtonRelease %d not supported\n", event.xbutton.button);
					} else {
						Posix_QueEvent( SE_KEY, K_MOUSE1 + b, false, 0, NULL);
						if (!Posix_AddMousePollEvent( M_ACTION1 + b, false ))
							return;
					}
#endif
				}
			break;
			
			case MotionNotify:
				if (!mouse_active)
					break;
				if (in_dgamouse.GetBool()) {
					dx = event.xmotion.x_root;
					dy = event.xmotion.y_root;

					Posix_QueEvent( SE_MOUSE, dx, dy, 0, NULL);

					// if we overflow here, we'll get a warning, but the delta will be completely processed anyway
					Posix_AddMousePollEvent( M_DELTAX, dx );
					if (!Posix_AddMousePollEvent( M_DELTAY, dy ))
						return;
				} else {
					// if it's a center motion, we've just returned from our warp
					// FIXME: we generate mouse delta on wrap return, but that lags us quite a bit from the initial event..
					if (event.xmotion.x == glConfig.vidWidth / 2 &&
						event.xmotion.y == glConfig.vidHeight / 2) {
						mwx = glConfig.vidWidth / 2;
						mwy = glConfig.vidHeight / 2;

						Posix_QueEvent( SE_MOUSE, mx, my, 0, NULL);

						Posix_AddMousePollEvent( M_DELTAX, mx );
						if (!Posix_AddMousePollEvent( M_DELTAY, my ))
							return;
						mx = my = 0;
						break;
					}

					dx = ((int) event.xmotion.x - mwx);
					dy = ((int) event.xmotion.y - mwy);
					mx += dx;
					my += dy;

					mwx = event.xmotion.x;
					mwy = event.xmotion.y;
				    XWarpPointer(dpy,None,win,0,0,0,0, (glConfig.vidWidth/2),(glConfig.vidHeight/2));
				}
			break;
#endif

#ifdef _HARMATTAN_PLUS
				case GenericEvent:
					karinXI2Event(&event);
					/*
					else
						printf("Unhandle XGenericEvent\n");
						*/
					break;
#endif

		}
	}
}

/*
=================
Sys_ShutdownInput
=================
*/
void Sys_ShutdownInput( void ) { }

/*
===============
Sys_MapCharForKey
===============
*/
unsigned char Sys_MapCharForKey( int _key ) {
	int			key;	// scan key ( != doom key )
	XkbStateRec kbd_state;
	XEvent		event;
	KeySym		keysym;
	int			lookupRet;
	char		buf[5];

	if ( !have_xkb || !dpy ) {
		return (unsigned char)_key;
	}

	// query the current keyboard group, must be passed as bit 13-14 in the constructed XEvent
	// see X Keyboard Extension library specifications 
	XkbGetState( dpy, XkbUseCoreKbd, &kbd_state );

	// lookup scancode from doom key code. unique hits
	for ( key = 0; key < 128; key++ ) {
		if ( _key == s_scantokey[ key ] ) {
			break;
		}
	}
	if ( key == 128 ) {
		// it happens. these, we can't convert
		common->DPrintf( "Sys_MapCharForKey: doom key %d -> keycode failed\n", _key );
		return (unsigned char)_key;
	}

	memset( &event, 0, sizeof( XEvent ) );
	event.xkey.type = KeyPress;
	event.xkey.display = dpy;
	event.xkey.time = CurrentTime;
	event.xkey.keycode = key;
	event.xkey.state = kbd_state.group << 13;

	lookupRet = XLookupString( (XKeyEvent *)&event, buf, sizeof( buf ), &keysym, NULL );
	if ( lookupRet <= 0 ) {
		Sys_Printf( "Sys_MapCharForKey: XLookupString key 0x%x failed\n", key );
		return (unsigned char)_key;
	}
	if ( lookupRet > 1 ) {
		// only ever expecting 1 char..
		Sys_Printf( "Sys_MapCharForKey: XLookupString returned '%s'\n", buf );
	}
	return buf[ 0 ];
}

//#include "xi2.cpp" // not add to scons main script.
//#include "vkb.cpp" // not add to scons main script.

#endif
