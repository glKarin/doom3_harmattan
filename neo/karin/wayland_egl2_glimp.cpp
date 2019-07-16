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
#include "gl_vkb.h"
#include "q3_png.h"
#include "fbo.h"

idCVar sys_videoRam( "sys_videoRam", "0", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_INTEGER, "Texture memory on the video card (in megabytes) - 0: autodetect", 0, 512 );

struct wl_display *dpy = NULL;
struct wl_registry *registry = NULL;
struct wl_compositor *compositor = NULL;
struct wl_shell *shell = NULL;
struct wl_shm *shm = NULL;
struct wl_seat *seat = NULL;
struct wl_pointer *pointer = NULL;
struct wl_keyboard *keyboard = NULL;
struct wl_touch *touch = NULL;
struct wl_surface *surface = NULL;
struct wl_shell_surface *shell_surface = NULL;
struct wl_egl_window *win = NULL;
struct wl_buffer *buffer = NULL;
void *data = NULL;

EGLContext eglContext = NULL;
EGLDisplay eglDisplay = NULL;
EGLSurface eglSurface = NULL;

#ifdef _HARMATTAN_GL

#define KARIN_MAX_TOUCH_COUNT 10
#define KARIN_TOUCH_BUTTON 1
#define KARIN_TRUE 1
#define KARIN_FALSE 0

static karin_FrameBufferObject *fbo = 0;
extern unsigned karinHandleVKBAction(int action, unsigned pressed, int dx, int dy);

typedef enum _karin_TouchState_e
{
	Touch_None = 0,
	Touch_Down,
	Touch_Motion,
	Touch_Up,
} karin_TouchState_e;

typedef struct _karin_TouchEvent
{
	unsigned mask; // 0 - invalid, 1 - down, 2 - motion
	uint32_t id;
	int last_x;
	int last_y;
	int x;
	int y;
	uint32_t time;
} karin_TouchEvent;

static karin_TouchEvent touch_events[KARIN_MAX_TOUCH_COUNT];
static int touch_event_depth = 0;

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

	fprintf(stderr, "%s: 0x%04x: %s\n", func, err, GLimp_StringErrors[err] - EGL_SUCCESS);
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

static void karinCoord(int ox, int oy, int w, int h, int *rx, int *ry)
{
	if(!rx || !ry)
		return;

	*rx = oy;
	*ry = ox;
}

static karin_TouchEvent * karinFind(uint32_t id)
{
	int i;
	karin_TouchEvent *e;

	for(i = 0; i < KARIN_MAX_TOUCH_COUNT; i++)
	{
		e = touch_events + i;
		if(e->mask && e->id == id)
		{
			return e;
		}
	}
	return NULL;
}

static karin_TouchEvent * karinUpdate(unsigned mask, uint32_t id, wl_fixed_t x, wl_fixed_t y, uint32_t time)
{
	int i;
	karin_TouchEvent *e;

	if(mask == Touch_None)
		return NULL;

	// update
	e = karinFind(id);
	if(e)
	{
		e->mask = mask;
		e->last_x = e->x;
		e->last_y = e->y;
		e->x = wl_fixed_to_int(x);
		e->y = wl_fixed_to_int(y);
		e->time = time;
		return e;
	}

	// add
	if(mask != Touch_Down)
		return NULL;

	for(i = 0; i < KARIN_MAX_TOUCH_COUNT; i++)
	{
		e = touch_events + i;
		if(e->mask == Touch_None)
		{
			e->mask = mask;
			e->id = id;
			e->x = wl_fixed_to_int(x);
			e->y = wl_fixed_to_int(y);
			e->last_x = e->x;
			e->last_y = e->y;
			e->time = time;
			return e;
		}
	}
	return NULL;
}

static int karinFlush(void)
{
	int i;
	int c;
	karin_TouchEvent *e;

	c = 0;
	for(i = 0; i < KARIN_MAX_TOUCH_COUNT; i++)
	{
		e = touch_events + i;
		if(e->mask == Touch_Up)
		{
			memset(e, 0, sizeof(karin_TouchEvent));
		}
		if(e->mask != Touch_None)
			c++;
	}
	touch_event_depth = c;
	return c;
}

unsigned karinXI2MouseEvent(int button, unsigned pressed, int x, int y)
{
	int rx;
	int ry;

	karinCoord(x, y, scr_width, scr_height, &rx, &ry);

	if(h_cUsingVKB.GetBool() && karinVKBMouseEvent(button, pressed, rx, ry, karinHandleVKBAction))
		return 1;
	Posix_QueEvent( SE_TOUCH, rx, scr_height - ry, 0, NULL);
	Posix_QueEvent( SE_KEY, K_MOUSE1, pressed, 0, NULL);
	return 1;
}

unsigned karinXI2MotionEvent(int button, unsigned pressed, int x, int y, int dx, int dy)
{
	int rx;
	int ry;

	karinCoord(x, y, scr_width, scr_height, &rx, &ry);

	if(h_cUsingVKB.GetBool() && karinVKBMouseMotionEvent(button, pressed, rx,  ry, dy, dx, karinHandleVKBAction))
		return 1;
	Posix_QueEvent( SE_TOUCH, rx, scr_height - ry, 0, NULL);
	return 1;
}

// touch listener
static void karinTouchDown(void *data, struct wl_touch *wl_touch, uint32_t serial, uint32_t time, struct wl_surface *surface, int32_t id, wl_fixed_t x, wl_fixed_t y);
static void karinTouchUp(void *data, struct wl_touch *wl_touch, uint32_t serial, uint32_t time, int32_t id);
static void karinTouchMotion(void *data, struct wl_touch *wl_touch, uint32_t time, int32_t id, wl_fixed_t x, wl_fixed_t y);
static void karinTouchFrame(void *data, struct wl_touch *wl_touch);
static void karinTouchCancel(void *data, struct wl_touch *wl_touch);

// global listener
static void karinRegistryListenerGlobal(void *data, struct wl_registry *registry, uint32_t id, const char *interface, uint32_t version);
static void karinRegistryListenerGlobalRemove(void *data, struct wl_registry *registry, uint32_t name);

// shell surface listener
static void karinShellSurfaceListenerPing(void *data, struct wl_shell_surface *shell_surface, uint32_t serial);
static void karinShellSurfaceListenerConfigure(void *data, struct wl_shell_surface *shell_surface, uint32_t edges, int32_t width, int32_t height);
static void karinShellSurfaceListenerPopupDone(void *data, struct wl_shell_surface *shell_surface);

static const struct wl_registry_listener listener = {
	karinRegistryListenerGlobal, karinRegistryListenerGlobalRemove
};

static const struct wl_shell_surface_listener shell_surface_listener = {
	karinShellSurfaceListenerPing, karinShellSurfaceListenerConfigure, karinShellSurfaceListenerPopupDone
};

static const struct wl_touch_listener touch_listener = {
	karinTouchDown, karinTouchUp, karinTouchMotion, karinTouchFrame, karinTouchCancel
};

static void karinResize(int32_t width, int32_t height)
{
	printf("Wayland window resize: (%d, %d)\n", width, height);
	wl_egl_window_resize(win, width, height, 0, 0);
#if 0 // for test
	glConfig.vidWidth = GL_W;
	glConfig.vidHeight = GL_H;
#endif

	karinResizeVKB(height, width);
	karinSetScrGeometry(height, width);

	fbo->resize(0, 0, width, height);
	fbo->bind();
}

static void karinWayland(int width, int height)
{
	registry = wl_display_get_registry(dpy);
	wl_registry_add_listener(registry, &listener, &dpy);
	wl_display_dispatch(dpy);

	surface = wl_compositor_create_surface(compositor);

	shell_surface = wl_shell_get_shell_surface(shell, surface);
	wl_display_roundtrip(dpy);
	wl_display_get_fd(dpy);
	wl_shell_surface_add_listener(shell_surface, &shell_surface_listener, NULL);
	wl_shell_surface_set_title(shell_surface, "DOOM III on Nemo Wayland");
	wl_shell_surface_set_toplevel(shell_surface);
	wl_shell_surface_set_maximized(shell_surface, NULL);
	//wl_shell_surface_set_fullscreen(shell_surface, WL_SHELL_SURFACE_FULLSCREEN_METHOD_FILL, 60, NULL);
	//wl_surface_set_buffer_transform(surface, WL_OUTPUT_TRANSFORM_90);
	//wl_surface_damage(surface, 0, 0, 480, 854);
	wl_surface_commit(surface);
	
	wl_display_flush(dpy);

	win = wl_egl_window_create(surface, width, height);

}

static unsigned karinEGL(void)
{
#define MAX_NUM_CONFIGS 1
	int i;
	EGLint major, minor;
	EGLint config_count;
	EGLConfig configs[MAX_NUM_CONFIGS];
	int attrib[] = {
		EGL_NATIVE_VISUAL_TYPE, 0,

		/* RGB565 */
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 8,

		EGL_DEPTH_SIZE, 8,
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

		common->Printf( "[Harmattan]: EGL context: %d/%d/%d Color bits, %d Alpha bits, %d depth, %d stencil display, %d buffer.\n",
			red_bits, green_bits,
			 blue_bits, alpha_bits,
			 depth_bits,
			 stencil_bits, buffer_bits);

		/*
		glConfig.colorBits = tcolorbits;
		glConfig.depthBits = tdepthbits;
		glConfig.stencilBits = tstencilbits;
		*/

		return 1;
}

static void karinTouchDown(void *data, struct wl_touch *wl_touch, uint32_t serial, uint32_t time, struct wl_surface *surface, int32_t id, wl_fixed_t x, wl_fixed_t y)
{
	karin_TouchEvent *e;

	e = karinUpdate(Touch_Down, id, x, y, time);

	if(e)
	{
		karinXI2MouseEvent(KARIN_TOUCH_BUTTON, KARIN_TRUE, e->x, e->y);
		//printf("D %d %d\n", e->x, e->y);
	}
}

static void karinTouchUp(void *data, struct wl_touch *wl_touch, uint32_t serial, uint32_t time, int32_t id)
{
	karin_TouchEvent *e;

	e = karinUpdate(Touch_Up, id, 0, 0, time);

	if(e)
	{
		karinXI2MouseEvent(KARIN_TOUCH_BUTTON, KARIN_FALSE, e->last_x, e->last_y);
		//printf("U %d %d\n", e->x, e->y);
	}

	karinFlush();
}

static void karinTouchMotion(void *data, struct wl_touch *wl_touch, uint32_t time, int32_t id, wl_fixed_t x, wl_fixed_t y)
{
	karin_TouchEvent *e;

	e = karinUpdate(Touch_Motion, id, x, y, time);

	if(e)
	{
		karinXI2MotionEvent(KARIN_TOUCH_BUTTON, KARIN_TRUE, e->x, e->y, e->x - e->last_x, e->y - e->last_y);
		//printf("M %d %d\n", e->x, e->y);
	}
}

static void karinTouchFrame(void *data, struct wl_touch *wl_touch)
{
}

static void karinTouchCancel(void *data, struct wl_touch *wl_touch)
{
}


void karinShellSurfaceListenerPing(void *data, struct wl_shell_surface *shell_surface, uint32_t serial)
{
	wl_shell_surface_pong(shell_surface, serial);
}

void	karinShellSurfaceListenerConfigure(void *data, struct wl_shell_surface *shell_surface, uint32_t edges, int32_t width, int32_t height)
{
	//printf("*************                 %d %d\n", width, height);
	// intex 1280 720
	karinResize(width, height);
}

void karinShellSurfaceListenerPopupDone(void *data, struct wl_shell_surface *shell_surface)
{
}


void karinRegistryListenerGlobalRemove(void *data, struct wl_registry *reg, uint32_t name)
{
} 

void karinRegistryListenerGlobal(void *data, struct wl_registry *reg, uint32_t id, const char *interface, uint32_t version)
{
	if(strcmp(interface, "wl_compositor") == 0)
	{
		compositor = (struct wl_compositor *)wl_registry_bind(reg, id, &wl_compositor_interface, 1);	
	}
	else if(strcmp(interface, "wl_shell") == 0)
	{
		shell = (struct wl_shell *)wl_registry_bind(reg, id, &wl_shell_interface, 1);
	}
	else if(strcmp(interface, "wl_shm") == 0)
	{
		//shm = wl_registry_bind(reg, id, &wl_shm_interface, 1);
//wl_shm_add_listener(shm, &shm_listenter, NULL);
	}
	else if(strcmp(interface, "wl_seat") == 0)
	{
		seat = (struct wl_seat *)wl_registry_bind(reg, id, &wl_seat_interface, 1);
		//pointer = wl_seat_get_pointer(seat);
		//keyboard = wl_seat_get_keyboard(seat);
		touch = wl_seat_get_touch(seat);
		wl_touch_add_listener(touch, &touch_listener, NULL);
	}
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
		
		if (win)
			wl_egl_window_destroy(win);

		wl_shell_surface_destroy(shell_surface);
		wl_surface_destroy(surface);
		//wl_seat_release(seat);
		// wl_keyboard_release(keyboard);
		// wl_pointer_release(pointer);
		wl_touch_release(touch);

		wl_display_disconnect(dpy);

		// FIXME: that's going to crash
		//XCloseDisplay( dpy );

	}
	seat = NULL;
	surface = NULL;
	keyboard = NULL;
	pointer = NULL;
	touch = NULL;

	shm = NULL;
	registry = NULL;
	compositor = NULL;
	shell = NULL;
	shell_surface = NULL;

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

	common->Printf( "Setup wayland display connection\n" );

	// that should be the first call into X
#if 0
	if ( !XInitThreads() ) {
		common->Printf("XInitThreads failed\n");
		return false;
	}
#endif
	
	// set up our custom error handler for X failures
	if ( !( dpy = wl_display_connect(NULL) ) ) {
		common->Printf( "Couldn't open the wayland display\n" );
		return false;
	}
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

	karinWayland(glConfig.vidWidth, glConfig.vidHeight);
	// color, depth and stencil

	if(!karinEGL())
	{
		common->Printf( "Initializing EGL fail.\n" );
		return false;
	}

	fbo = new karin_FrameBufferObject(glConfig.vidWidth, glConfig.vidHeight, glConfig.vidWidth, glConfig.vidHeight);
	fbo->bind();
	glConfig.isFullscreen = a.fullScreen;

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
	karinSetScrGeometry(glConfig.vidHeight, glConfig.vidWidth);
	karinNewVKB(0.0, 0.0, 0.0, glConfig.vidHeight, glConfig.vidWidth);
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
