// for include in cpp source file.

#ifdef _HARMATTAN

#include "../idlib/precompiled.h"
#include "../renderer/tr_local.h"
#include "../sys/linux/local.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "../sys/posix/posix_public.h"
#include <X11/Xatom.h>

#include "xi2.h"
#include "gl_vkb.h"
#include "q3_png.h"
#include "fbo.h"

idCVar sys_videoRam( "sys_videoRam", "0", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_INTEGER, "Texture memory on the video card (in megabytes) - 0: autodetect", 0, 512 );

Display *dpy = NULL;
static int scrnum = 0;

Window win = 0;

EGLContext eglContext = NULL;
EGLDisplay eglDisplay = NULL;
EGLSurface eglSurface = NULL;

#ifdef _HARMATTAN_GL
extern Bool karinXI2MouseEvent(int button, Bool pressed, int x, int y);
extern Bool karinXI2MotionEvent(int button, Bool pressed, int x, int y, int dx, int dy);
#endif

#ifdef _HARMATTAN_GL

#define KARIN_MAX_TOUCH_COUNT 10
#define KARIN_TOUCH_BUTTON 1
#define KARIN_TRUE 1
#define KARIN_FALSE 0

static karin_FrameBufferObject *fbo = 0;
extern unsigned karinHandleVKBAction(int action, unsigned pressed, int dx, int dy);

static int idXErrorHandler(Display *l_dpy, XErrorEvent *ev)
{
	char buf[1024];
	common->Printf("Fatal X Error:\n");
	common->Printf("  Major opcode of failed request: %d\n", ev->request_code);
	common->Printf("  Minor opcode of failed request: %d\n", ev->minor_code);
	common->Printf("  Serial number of failed request: %lu\n", ev->serial);
	XGetErrorText(l_dpy, ev->error_code, buf, 1024);
	common->Printf("%s\n", buf);
	return 0;
}


static void GLimp_DisableComposition(void)
{
#define _NET_WM_STATE_REMOVE        0	/* remove/unset property */
#define _NET_WM_STATE_ADD           1	/* add/set property */
#define _NET_WM_STATE_TOGGLE        2	/* toggle property  */
	XClientMessageEvent xclient;
	Atom atom;
	int one = 1;

	atom = XInternAtom(dpy, "_HILDON_NON_COMPOSITED_WINDOW", False);
	XChangeProperty(dpy, win, atom, XA_INTEGER, 32, PropModeReplace, (unsigned char *)&one, 1);

	xclient.type = ClientMessage;
	xclient.window = win;	//GDK_WINDOW_XID (window);
	xclient.message_type = XInternAtom(dpy, "_NET_WM_STATE", False);
	xclient.format = 32;
	xclient.data.l[0] = glConfig.isFullscreen ? _NET_WM_STATE_ADD : _NET_WM_STATE_REMOVE;
	//gdk_x11_atom_to_xatom_for_display (display, state1);
	//gdk_x11_atom_to_xatom_for_display (display, state2);
	xclient.data.l[1] = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
	xclient.data.l[2] = 0;
	xclient.data.l[3] = 0;
	xclient.data.l[4] = 0;
	XSendEvent(dpy, DefaultRootWindow(dpy), False, SubstructureRedirectMask | SubstructureNotifyMask, (XEvent *) & xclient);
}

static void GLimp_HandleError(const char *func)
{
static const char *GLimp_StringErrors[] = {
	"EGL_SUCCESS",
	"EGL_NOT_INITIALIZED",
	"EGL_BAD_ACCESS",
	"EGL_BAD_ALLOC",
	"EGL_BAD_ATTRIBUTE",
	"EGL_BAD_CONFIG",
	"EGL_BAD_CONTEXT",
	"EGL_BAD_CURRENT_SURFACE",
	"EGL_BAD_DISPLAY",
	"EGL_BAD_MATCH",
	"EGL_BAD_NATIVE_PIXMAP",
	"EGL_BAD_NATIVE_WINDOW",
	"EGL_BAD_PARAMETER",
	"EGL_BAD_SURFACE",
	"EGL_CONTEXT_LOST",
};
	GLint err = eglGetError();

	fprintf(stderr, "%s: 0x%04x: %s\n", func, err, GLimp_StringErrors[err - EGL_SUCCESS]);
	assert(0);
}

static void karinPrintDev(void)
{
	char fmt_str[] = "[%s]: %s -> %s\n";
	common->Printf( fmt_str, _HARMATTAN_APPNAME, "Ver", _HARMATTAN_VER);
	common->Printf( fmt_str, _HARMATTAN_APPNAME, "Code", _HARMATTAN_DEVCODE);
	common->Printf( fmt_str, _HARMATTAN_APPNAME, "Dev", _HARMATTAN_DEV);
	common->Printf( fmt_str, _HARMATTAN_APPNAME, "Release", _HARMATTAN_RELEASE);
	common->Printf( fmt_str, _HARMATTAN_APPNAME, "Desc", _HARMATTAN_DESC);
}

unsigned karinXI2MouseEvent(int button, unsigned pressed, int x, int oy)
{
	int y;

	y = fbo->height() - oy;
	if(h_cUsingVKB.GetBool() && karinVKBMouseEvent(button, pressed, x, y, karinHandleVKBAction))
		return 1;
	Posix_QueEvent( SE_TOUCH, x, oy, 0, NULL);
	Posix_QueEvent( SE_KEY, K_MOUSE1, pressed, 0, NULL);
	return 1;
}

unsigned karinXI2MotionEvent(int button, unsigned pressed, int x, int oy, int dx, int ody)
{
	int y;
	int dy;

	y = fbo->height() - oy;
	dy = -ody;
	if(h_cUsingVKB.GetBool() && karinVKBMouseMotionEvent(button, pressed, x,  y, dx, dy, karinHandleVKBAction))
		return 1;
	Posix_QueEvent( SE_TOUCH, x, oy, 0, NULL);
	return 1;
}

static void karinResize(int32_t width, int32_t height)
{
	printf("X11 window resize: (%d, %d)\n", width, height);
#if 0 // for test
	glConfig.vidWidth = GL_W;
	glConfig.vidHeight = GL_H;
#endif

	karinResizeVKB(width, height);
	karinSetScrGeometry(width, height);

	fbo->resize(0, 0, width, height);
	fbo->bind();
}

static void karinX11(int width, int height)
{
	Window root;
	XSetWindowAttributes attr;
	// XSizeHints sizehints;
	unsigned long mask;
	int i;
	const char *glstring;

	root = RootWindow( dpy, scrnum );

	// window attributes
	int blackColour = BlackPixel(dpy, scrnum);
	win = XCreateSimpleWindow(dpy, /* root */DefaultRootWindow(dpy), 0, 0, 1, 1, 0, blackColour, blackColour);

#ifdef _HARMATTAN_PLUS
	karinInitXI2();
#endif

	XWindowAttributes WinAttr;
	int XResult = BadImplementation;

	if (!(XResult = XGetWindowAttributes(dpy, win, &WinAttr)))
		GLimp_HandleError("");

	XSelectInput(dpy, win, X_MASK);

	XStoreName(dpy, win, GAME_NAME);

	GLimp_DisableComposition();
	XMapWindow( dpy, win );
	GLimp_DisableComposition();

	XFlush(dpy);
	XSync(dpy, False);

#ifdef _HARMATTAN_PLUS
	karinXI2Atom();

	karinSetMultiMouseEventFunction(karinXI2MouseEvent);
	karinSetMultiMotionEventFunction(karinXI2MotionEvent);
#endif

}

static unsigned karinEGL(void)
{
#define MAX_NUM_CONFIGS 4
	int i;
	EGLint major, minor;
	EGLint config_count;
	EGLConfig configs[MAX_NUM_CONFIGS];
	int attrib[] = {
		EGL_NATIVE_VISUAL_TYPE, 0,

		/* RGB565 */
		EGL_RED_SIZE, 5,
		EGL_GREEN_SIZE, 6,
		EGL_BLUE_SIZE, 5,
		//EGL_ALPHA_SIZE, 1,

		EGL_DEPTH_SIZE, 24,
		EGL_STENCIL_SIZE, 8,
		EGL_BUFFER_SIZE, 16,

		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,

		EGL_NONE
	};
    // these match in the array
#define ATTR_RED_IDX            3
#define ATTR_GREEN_IDX          5
#define ATTR_BLUE_IDX           7
#define ATTR_ALPHA_IDX          9
#define ATTR_DEPTH_IDX          11
#define ATTR_STENCIL_IDX        13
#define ATTR_BUFFER_SIZE_IDX    15

	int colorbits, depthbits, stencilbits;
	int tcolorbits, tdepthbits, tstencilbits;
	// color, depth and stencil
	colorbits = 24;
	depthbits = 24;
	stencilbits = 8;

	eglSurface = EGL_NO_SURFACE;
	eglDisplay = eglGetDisplay((NativeDisplayType) dpy);
	if (!eglInitialize(eglDisplay, &major, &minor))
	{
		GLimp_HandleError("eglInitialize");
		return 0;
	}

	if (!eglGetConfigs(eglDisplay, configs, MAX_NUM_CONFIGS, &config_count))
	{
		GLimp_HandleError("eglGetConfig");
		return 0;
	}

	/*
	if (!eglChooseConfig (eglDisplay, attrib, configs, MAX_NUM_CONFIGS, &config_count))
		GLimp_HandleError("eglChooseConfig");
		*/

#if 0
	for (i = 0; i < 16; i++) {
		// 0 - default
		// 1 - minus colorbits
		// 2 - minus depthbits
		// 3 - minus stencil
		if ((i % 4) == 0 && i) {
			// one pass, reduce
			switch (i / 4) {
				case 2:
					if (colorbits == 24)
						colorbits = 16;
					break;
				case 1:
					if (depthbits == 24)
						depthbits = 16;
					else if (depthbits == 16)
						depthbits = 8;
				case 3:
					if (stencilbits == 24)
						stencilbits = 16;
					else if (stencilbits == 16)
						stencilbits = 8;
			}
		}

		tcolorbits = colorbits;
		tdepthbits = depthbits;
		tstencilbits = stencilbits;

		if ((i % 4) == 3) {        // reduce colorbits
			if (tcolorbits == 24)
				tcolorbits = 16;
		}

		if ((i % 4) == 2) {        // reduce depthbits
			if (tdepthbits == 24)
				tdepthbits = 16;
			else if (tdepthbits == 16)
				tdepthbits = 8;
		}

		if ((i % 4) == 1) {        // reduce stencilbits
			if (tstencilbits == 24)
				tstencilbits = 16;
			else if (tstencilbits == 16)
				tstencilbits = 8;
			else
				tstencilbits = 0;
		}

		if (tcolorbits == 24) {
			attrib[ATTR_RED_IDX] = 8;
			attrib[ATTR_GREEN_IDX] = 8;
			attrib[ATTR_BLUE_IDX] = 8;
			attrib[ATTR_BUFFER_SIZE_IDX] = 24;
		} else {
			// must be 16 bit
			attrib[ATTR_RED_IDX] = 4;
			attrib[ATTR_GREEN_IDX] = 4;
			attrib[ATTR_BLUE_IDX] = 4;
			attrib[ATTR_BUFFER_SIZE_IDX] = 16;
		}

	attrib[ATTR_RED_IDX] = 5;
	attrib[ATTR_GREEN_IDX] = 6;
	attrib[ATTR_BLUE_IDX] = 5;

		attrib[ATTR_DEPTH_IDX] = tdepthbits;    // default to 24 depth
		attrib[ATTR_STENCIL_IDX] = tstencilbits;

		/* Here, the application chooses the configuration it desires. In this
		 * sample, we have a very simplified selection process, where we pick
		 * the first EGLConfig that matches our criteria */
		if (!eglChooseConfig (eglDisplay, attrib, configs, MAX_NUM_CONFIGS, &config_count))
		{
			GLimp_HandleError("eglChooseConfig");
			continue;
		}

		Sys_Printf("Using %d/%d/%d Color bits, %d Alpha bits, %d depth, %d stencil display.\n",
				attrib[ATTR_RED_IDX], attrib[ATTR_GREEN_IDX],
				attrib[ATTR_BLUE_IDX], attrib[ATTR_ALPHA_IDX],
				attrib[ATTR_DEPTH_IDX],
				attrib[ATTR_STENCIL_IDX]);

		glConfig.colorBits = tcolorbits;
		glConfig.depthBits = tdepthbits;
		glConfig.stencilBits = tstencilbits;
		break;
	}
#endif

	if (!eglChooseConfig (eglDisplay, attrib, configs, MAX_NUM_CONFIGS, &config_count))
	{
		GLimp_HandleError("eglChooseConfig");
		return 0;
	}
	for (i = 0; i < config_count; i++) {
		if ((eglSurface = eglCreateWindowSurface(eglDisplay, configs[i], (NativeWindowType) win, NULL)) != EGL_NO_SURFACE)
			break;
	}
	if (eglSurface == EGL_NO_SURFACE)
	{
		GLimp_HandleError("eglCreateWindowSurface");
		return 0;
	}

	EGLint ctxAttrib[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};
	if ((eglContext = eglCreateContext(eglDisplay, configs[i], EGL_NO_CONTEXT, ctxAttrib)) == EGL_NO_CONTEXT)
	{
		GLimp_HandleError("eglCreateContext");
		return 0;
	}

	if (!eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext))
	{
		GLimp_HandleError("eglMakeCurrent");
		return 0;
	}

	//EGLDLSYM(eglCreateImageKHR);
	//EGLDLSYM(eglDestroyImageKHR);
		int stencil_bits = 0;
		int depth_bits = 0;
		int red_bits = 0;
		int green_bits = 0;
		int blue_bits = 0;
		int alpha_bits = 0;
		int buffer_bits = 0;
		eglGetConfigAttrib(eglDisplay, configs[i], EGL_STENCIL_SIZE, &stencil_bits);
		eglGetConfigAttrib(eglDisplay, configs[i], EGL_DEPTH_SIZE, &depth_bits);
		eglGetConfigAttrib(eglDisplay, configs[i], EGL_RED_SIZE, &red_bits);
		eglGetConfigAttrib(eglDisplay, configs[i], EGL_GREEN_SIZE, &green_bits);
		eglGetConfigAttrib(eglDisplay, configs[i], EGL_BLUE_SIZE, &blue_bits);
		eglGetConfigAttrib(eglDisplay, configs[i], EGL_ALPHA_SIZE, &alpha_bits); // ???
		eglGetConfigAttrib(eglDisplay, configs[i], EGL_BUFFER_SIZE, &buffer_bits); // ???
		tcolorbits = red_bits + green_bits + blue_bits;
		tdepthbits = depthbits;
		tstencilbits = 0;

		common->Printf( "[Harmattan]: EGL context: %d/%d/%d Color bits, %d Alpha bits, %d depth, %d stencil display, %d buffer.\n",
			red_bits, green_bits,
			 blue_bits, alpha_bits,
			 depth_bits,
			 stencil_bits, buffer_bits);

		glConfig.colorBits = tcolorbits;
		glConfig.depthBits = tdepthbits;
		glConfig.stencilBits = tstencilbits;

		return 1;
}

#endif

void GLimp_WakeBackEnd(void *a) {
	common->DPrintf("GLimp_WakeBackEnd stub\n");
}

void GLimp_EnableLogging(bool log) {
	//common->DPrintf("GLimp_EnableLogging stub\n");
}

void GLimp_FrontEndSleep() {
	common->DPrintf("GLimp_FrontEndSleep stub\n");
}

void *GLimp_BackEndSleep() {
	common->DPrintf("GLimp_BackEndSleep stub\n");
	return 0;
}

bool GLimp_SpawnRenderThread(void (*a) ()) {
	common->DPrintf("GLimp_SpawnRenderThread stub\n");
	return false;
}

void GLimp_ActivateContext() {
	assert( dpy );
	assert( eglDisplay );
	assert( eglSurface );
	assert( eglContext );
	eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext);
}

void GLimp_DeactivateContext() {
	assert( dpy );
	assert( eglDisplay ); // ???
	eglMakeCurrent(eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

/*
=================
GLimp_SaveGamma

save and restore the original gamma of the system
=================
*/
void GLimp_SaveGamma() {
}

/*
=================
GLimp_RestoreGamma

save and restore the original gamma of the system
=================
*/
void GLimp_RestoreGamma() {
}

/*
=================
GLimp_SetGamma

gamma ramp is generated by the renderer from r_gamma and r_brightness for 256 elements
the size of the gamma ramp can not be changed on X (I need to confirm this)
=================
*/
void GLimp_SetGamma(unsigned short red[256], unsigned short green[256], unsigned short blue[256]) {
}

void GLimp_Shutdown() {
	if ( dpy ) {
		
#ifdef _HARMATTAN_PLUS
		delete fbo;
		fbo = 0;
		common->Printf("[karin]: Destroy virtual button layer ... ");
		karinDeleteVKB();
		common->Printf("Done\n");
#endif

		Sys_XUninstallGrabs();
	
		GLimp_RestoreGamma();

		eglDestroyContext(eglDisplay, eglContext);
		eglDestroySurface(eglDisplay, eglSurface);
		eglMakeCurrent(eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		eglTerminate(eglDisplay);
		
		XDestroyWindow( dpy, win );

		XFlush( dpy );
		// FIXME: that's going to crash
		//XCloseDisplay( dpy );

	}
	eglContext = NULL;
	eglDisplay = NULL;
	eglSurface = NULL;
	win = NULL;
	dpy = NULL;
}

void GLimp_SwapBuffers() {
	assert( dpy );
	assert( eglDisplay );
	assert( eglSurface );

	fbo->blit();
	eglSwapBuffers(eglDisplay, eglSurface);
	fbo->bind();
}

/*
GLX_TestDGA
Check for DGA	- update in_dgamouse if needed
*/
void GLX_TestDGA() {
}

/*
** XErrorHandler
**   the default X error handler exits the application
**   I found out that on some hosts some operations would raise X errors (GLXUnsupportedPrivateRequest)
**   but those don't seem to be fatal .. so the default would be to just ignore them
**   our implementation mimics the default handler behaviour (not completely cause I'm lazy)
*/

bool GLimp_OpenDisplay( void ) {
	if ( dpy ) {
		return true;
	}

	if ( cvarSystem->GetCVarInteger( "net_serverDedicated" ) == 1 ) {
		common->DPrintf( "not opening the display: dedicated server\n" );
		return false;
	}

	common->Printf( "Setup X display connection\n" );

	// that should be the first call into X
	if ( !XInitThreads() ) {
		common->Printf("XInitThreads failed\n");
		return false;
	}
	
	// set up our custom error handler for X failures
	XSetErrorHandler( &idXErrorHandler );

	if ( !( dpy = XOpenDisplay(NULL) ) ) {
		common->Printf( "Couldn't open the X display\n" );
		return false;
	}
	scrnum = DefaultScreen( dpy );
	return true;
}

/*
===============
EGL_Init
===============
*/
int EGL_Init(glimpParms_t a) {
	/*
	int attrib[] = {
		GLX_RGBA,				// 0
		GLX_RED_SIZE, 8,		// 1, 2
		GLX_GREEN_SIZE, 8,		// 3, 4
		GLX_BLUE_SIZE, 8,		// 5, 6
		GLX_DOUBLEBUFFER,		// 7
		GLX_DEPTH_SIZE, 24,		// 8, 9
		GLX_STENCIL_SIZE, 8,	// 10, 11
		GLX_ALPHA_SIZE, 8, // 12, 13
		None
	};
	*/

	if ( !GLimp_OpenDisplay() ) {
		return false;
	}

	common->Printf( "Initializing OpenGL display\n" );

	int actualWidth, actualHeight;
	const char *glstring;

	actualWidth = glConfig.vidWidth;
	actualHeight = glConfig.vidHeight;
	glConfig.isFullscreen = a.fullScreen;

	karinX11(glConfig.vidWidth, glConfig.vidHeight);
	// color, depth and stencil

	if(!karinEGL())
	{
		common->Printf( "Initializing EGL fail.\n" );
		return false;
	}

	// ??? qglXMakeCurrent(dpy, win, ctx);

	glstring = (const char *) glGetString(GL_SHADING_LANGUAGE_VERSION);
	common->Printf("GL_SHADING_LANGUAGE_VERSION: %s\n", glstring);
	
	glstring = (const char *) glGetString(GL_VERSION);
	common->Printf("GL_VERSION: %s\n", glstring);
	
	glstring = (const char *) glGetString(GL_RENDERER);
	common->Printf("GL_RENDERER: %s\n", glstring);
	
	glstring = (const char *) glGetString(GL_EXTENSIONS);
	common->Printf("GL_EXTENSIONS: %s\n", glstring);

	// FIXME: here, software GL test

	// glConfig.isFullscreen = a.fullScreen;
	
	if ( glConfig.isFullscreen ) {
		Sys_GrabMouseCursor( true );
	}
	
	fbo = new karin_FrameBufferObject(glConfig.vidWidth, glConfig.vidHeight, glConfig.vidWidth, glConfig.vidHeight);
	fbo->bind();

	return true;
}

/*
===================
GLimp_Init

This is the platform specific OpenGL initialization function.  It
is responsible for loading OpenGL, initializing it,
creating a window of the appropriate size, doing
fullscreen manipulations, etc.  Its overall responsibility is
to make sure that a functional OpenGL subsystem is operating
when it returns to the ref.

If there is any failure, the renderer will revert back to safe
parameters and try again.
===================
*/
bool GLimp_Init( glimpParms_t a ) {

	if ( !GLimp_OpenDisplay() ) {
		return false;
	}
	
	if (!EGL_Init(a)) {
		return false;
	}
	
	karinPrintDev();

#ifdef _HARMATTAN_PLUS
	common->Printf("[karin]: Load virtual button layer ... ");
	karinSetScrGeometry(glConfig.vidWidth, glConfig.vidHeight);
	karinNewVKB(0.0, 0.0, 0.0, glConfig.vidWidth, glConfig.vidHeight);
	common->Printf("Done\n");
#endif

	return true;
}

/*
===================
GLimp_SetScreenParms
===================
*/
bool GLimp_SetScreenParms( glimpParms_t parms ) {
	return true;
}

/*
================
Sys_GetVideoRam
returns in megabytes
open your own display connection for the query and close it
using the one shared with GLimp_Init is not stable
================
*/
int Sys_GetVideoRam( void ) {
	static int run_once = 0;

	if ( run_once ) {
		return run_once;
	}

	if ( sys_videoRam.GetInteger() ) {
		run_once = sys_videoRam.GetInteger();
		return sys_videoRam.GetInteger();
	}

	// FIXME Harmattan device

	// try a few strategies to guess the amount of video ram
	common->Printf( "guessing video ram ( use +set sys_videoRam to force ) ..\n" );
	if ( !GLimp_OpenDisplay( ) ) {
		run_once = 64;
		return run_once;
	}
	common->Printf( "guess failed, return default low-end VRAM setting ( %dMB VRAM )\n", _HARMATTAN_GPU_MEM );
	run_once = _HARMATTAN_GPU_MEM;
	return run_once;
}

//#include "q3_png.cpp" // not add to scons main script.
//#include "gl_vkb1.cpp" // not add to scons main script.

#endif
