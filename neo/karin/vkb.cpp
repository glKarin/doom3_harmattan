#include "vkb.h"

#include <string.h>
#include "../idlib/precompiled.h"
#include "../framework/KeyInput.h"

#define VB_S(n) (n * VB_SPACING)
#define VB_W(n) (n * VB_WIDTH)
#define VB_H(n) (n * VB_HEIGHT)
#define VB_S2(n) (VB_S(n) / 3 * 2)
#define VB_W2(n) (VB_W(n) / 3 * 2)
#define VB_H2(n) (VB_H(n) / 3 * 2)
#define TEX_W(n) (n * TEX_WIDTH)
#define TEX_H(n) (n * TEX_HEIGHT)

#define VKB_BUTTON_Z_BASE 2000
#define VKB_SWIPE_Z_BASE 1000
#define VKB_JOYSTICK_Z_BASE 3000 
#define VKB_CURSOR_Z_BASE 4000

#define CIRCLECENTERX 182
#define CIRCLECENTERY 308
#define JOYSTICKOFFSET 70
//small circle 139x139 center point (70,70)
//x offset=69 y offset=69
#define BIGCIRCLEX 0
#define BIGCIRCLEY 0
#define SMALLCIRCLEX 294
#define SMALLCIRCLEY 89
#define BIGCIRCLEWIDTH 283
#define SMALLCIRCLEWIDTH 139
#define MYARG 22.5

#define CIRCLEHALFLENGTH 185.0
#define CIRCLEHALFLENGTHWITHOUTLISTENER 25.0

#define TRANSLUCENTLEVEL 157

unsigned client_state = 0;
boolean render_lock = bfalse;

/*
// sort by setting in D3
static const char *Action_Cmds[Total_Action] = {
	"_moveUp",
	"_moveDown",
	"_left",
	"_right",
	"_forward",
	"_back",
	"_lookUp",
	"_lookDown",
	"_strafe",
	"_moveLeft",
	"_moveRight",

	"_attack",
	"_speed", // run
	"_zoom",
	"_showScores",
	"_mlook",

	// button 0 - 7
	"_impulse0", // weapon 0 fist
	"_impulse1", // weapon 1
	"_impulse2", // weapon 2
	"_impulse3", // weapon 3
	"_impulse4", // weapon 4
	"_impulse5", // weapon 5
	"_impulse6", // weapon 6
	"_impulse7", // weapon 7
	"_impulse8", // weapon 8
	"_impulse9", // weapon 9
	"_impulse10", // weapon 10
	"_impulse11", // weapon 11 flashlight
	"_impulse12", // weapon 12 nothing
	"_impulse13", // weapon reload
	"_impulse14", // weapon next
	"_impulse15", // weapon prev
	// 16
	//"_impulse17", // ready to play
	"_impulse18", // center view
	"_impulse19", // show PDA
	//"_impulse20", // toggle team
	// 21
	// 22 spectate
	// NULL
};
*/

#define K_CONSOLE _HARMATTAN_CONSOLE_KEY

static int Key_Defs[Total_Key - Up_Key] = {
	K_UPARROW,
	K_DOWNARROW,
	K_LEFTARROW,
	K_RIGHTARROW,
	K_ESCAPE,
	K_CONSOLE,
	K_PGUP,
	K_PGDN,
	K_TAB,
	K_SHIFT,
	K_ENTER,
	K_BACKSPACE,
	K_SPACE,
	'a',
	'b',
	'c',
	'd',
	'e',
	'f',
	'g',
	'h',
	'i',
	'j',
	'k',
	'l',
	'm',
	'n',
	'o',
	'p',
	'q',
	'r',
	's',
	't',
	'u',
	'v',
	'w',
	'x',
	'y',
	'z',
	'0',
	'1',
	'2',
	'3',
	'4',
	'5',
	'6',
	'7',
	'8',
	'9',
	'_',
	'-',
	'.',
	'/',
	'+',
	'=',
	'?',
	'\\',
	K_MOUSE1,
};

const char *Tex_Files[VKB_TEX_COUNT] = {
	_HARMATTAN_RESC"anna_buttons.png",
	_HARMATTAN_RESC"circle_joystick.png",
	_HARMATTAN_RESC"A-Z_u.png",
	_HARMATTAN_RESC"0-9.png",
	_HARMATTAN_RESC"a-z_l.png"
};

struct vkb_cursor VKB_Cursor[CURSOR_COUNT] = {
	{VB_S(0) + VB_W(6), VB_S(2), VB_W(2) + VB_S(1), 
		0.5, 3.0, btrue, 1, 0.0,
		BIGCIRCLEX, BIGCIRCLEY + BIGCIRCLEWIDTH, BIGCIRCLEWIDTH,
		0.5, SMALLCIRCLEX, SMALLCIRCLEY + SMALLCIRCLEWIDTH, SMALLCIRCLEWIDTH,
		SMALLCIRCLEX, SMALLCIRCLEY + SMALLCIRCLEWIDTH, SMALLCIRCLEWIDTH,
		opengl_e_base, opengl_s_base, btrue, VKB_CURSOR_Z_BASE + 3, VKB_In_Game,
		Total_Action, TurnUp_Action, TurnDown_Action, TurnLeft_Action, TurnRight_Action}, // ???
	{VB_S(7) + VB_W(2), VB_S(5) + VB_W(1), VB_W(1) + VB_S(2),
		1.0, 5.0, bfalse, 0, 0.0,
		0, 0, 0,
		1.0, TEX_W(2), TEX_W(3), TEX_W(1),
		TEX_W(2), TEX_W(4), TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_CURSOR_Z_BASE + 2, VKB_In_Game,
		Attack_Action, 
		// TurnUp_Action, TurnDown_Action, TurnLeft_Action, TurnRight_Action
		Mouse1_Button, Mouse1_Button, Mouse1_Button, Mouse1_Button
	},
	{VB_W(3) + VB_S(5), VB_H(2) + VB_S(2), VB_W(1), 
		1.0, 6.0, bfalse, 0, 0.0,
		0, 0, 0,
		1.0, TEX_W(0), TEX_W(3), TEX_W(1),
		TEX_W(0), TEX_W(4), TEX_W(1),
		opengl_e_base, opengl_e_base, btrue, VKB_CURSOR_Z_BASE + 1, VKB_In_Game,
		Zoom_Action,
		Mouse1_Button, Mouse1_Button, Mouse1_Button, Mouse1_Button
	},
};

struct vkb_swipe VKB_Swipe[SWIPE_COUNT] = {
	{HARMATTAN_WIDTH / 4 * 3, 0, HARMATTAN_WIDTH, HARMATTAN_HEIGHT,
		HARMATTAN_WIDTH / 4 * 3, 0, HARMATTAN_WIDTH, HARMATTAN_HEIGHT, bfalse,
		BIGCIRCLEX, BIGCIRCLEY + BIGCIRCLEWIDTH, BIGCIRCLEWIDTH, BIGCIRCLEWIDTH,
		opengl_e_base, opengl_s_base, btrue, VKB_SWIPE_Z_BASE + 1, VKB_In_Game, 
		//TurnUp_Action, TurnDown_Action, TurnLeft_Action, TurnRight_Action
		MouseLook_Action,
		Mouse1_Button, Mouse1_Button, Mouse1_Button, Mouse1_Button
		}
};

struct vkb_joystick VKB_Joystick[JOYSTICK_COUNT] = {
	{VB_S(1), VB_S(1), VB_W(3) + VB_S(2), 
		0.0f, 1.8f,
		BIGCIRCLEX, BIGCIRCLEY + BIGCIRCLEWIDTH, BIGCIRCLEWIDTH,
		0.5, SMALLCIRCLEX, SMALLCIRCLEY + SMALLCIRCLEWIDTH, SMALLCIRCLEWIDTH,
		opengl_s_base, opengl_s_base, btrue, VKB_JOYSTICK_Z_BASE + 1, VKB_In_Game, 
		Forward_Action, Backward_Action, MoveLeft_Action, MoveRight_Action},
	{VB_S(1), VB_S(1), VB_W(3) + VB_S(2), 
		0.0f, 1.2f,
		BIGCIRCLEX, BIGCIRCLEY + BIGCIRCLEWIDTH, BIGCIRCLEWIDTH,
		0.5, SMALLCIRCLEX, SMALLCIRCLEY + SMALLCIRCLEWIDTH, SMALLCIRCLEWIDTH,
		opengl_s_base, opengl_s_base, btrue, VKB_CURSOR_Z_BASE + 200, VKB_In_Console | VKB_In_Message, 
		Up_Key, Down_Key, Left_Key, Right_Key},
	{VB_S(0), VB_W(1) + VB_S(1) / 2, VB_W(2) + VB_S(1), //107 
		0.0f, 1.0f,
		BIGCIRCLEX, BIGCIRCLEY + BIGCIRCLEWIDTH, BIGCIRCLEWIDTH,
		0.5, SMALLCIRCLEX, SMALLCIRCLEY + SMALLCIRCLEWIDTH, SMALLCIRCLEWIDTH,
		opengl_s_base, opengl_mb_base, btrue, VKB_CURSOR_Z_BASE + 100, VKB_In_Menu, 
		Up_Key, Down_Key, Left_Key, Right_Key}
};

struct vkb_button VKB_Button[VKB_COUNT] = {
	// game
	// right bottom
	{VB_S(1) + VB_W(1), VB_S(3) + VB_H(2), VB_W(1), VB_H(1), 
		VB_S(1) + VB_W(1), VB_S(3) + VB_H(2), VB_W(1), VB_H(1),  
		TEX_W(4), TEX_W(3), TEX_W(1), TEX_W(1),
		TEX_W(4), TEX_W(4), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 1, VKB_In_Game, 0,
		Jump_Action},
	{VB_S(1) + VB_W(1), VB_S(2) + VB_H(1), VB_W(1), VB_H(1), 
		VB_S(1) + VB_W(1), VB_S(2) + VB_H(1), VB_W(1), VB_H(1),  
		TEX_W(4), TEX_W(2), TEX_W(1), -TEX_W(1),
		TEX_W(4), TEX_W(3), TEX_W(1), -TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 1, VKB_In_Game, 0,
		Crouch_Action},
	{VB_S(1) + VB_W(1), VB_S(1), VB_W(1), VB_H(1), 
		VB_S(1) + VB_W(1), VB_S(1), VB_W(1), VB_H(1),  
		TEX_W(2), TEX_W(5), TEX_W(1), TEX_W(1),
		TEX_W(2), TEX_W(6), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 1, VKB_In_Game, 0,
		Run_Action},
	{VB_S(3) + VB_W(3), VB_S(1), VB_W(1), VB_H(1), 
		VB_S(3) + VB_W(3), VB_S(1), VB_W(1), VB_H(1),  
		TEX_W(2), TEX_W(5), TEX_W(1), TEX_W(1),
		TEX_W(2), TEX_W(6), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 1, VKB_In_Game, 0,
		Reload_Action},
	{VB_S(2) + VB_W(2), VB_S(1), VB_W(1), VB_H(1), 
		VB_S(2) + VB_W(2), VB_S(1), VB_W(1), VB_H(1),  
		TEX_W(0), TEX_W(3), TEX_W(1), TEX_W(1),
		TEX_W(0), TEX_W(4), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 1, VKB_In_Game, 0,
		ViewCenter_Action},

	// left bottom
	{VB_S(-1) + VB_W(2), VB_S(0), VB_W(1), VB_H(1), 
		VB_S(-1) + VB_W(2), VB_S(0), VB_W(1), VB_H(1),  
		TEX_W(2), TEX_W(1), TEX_W(1), TEX_W(1),
		TEX_W(2), TEX_W(2), TEX_W(1), TEX_W(1),
		opengl_mb_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 1, VKB_In_Game, 0,
		PrevWeapon_Action},
	{VB_S(-2) + VB_W(1), VB_S(0), VB_W(1), VB_H(1), 
		VB_S(-2) + VB_W(1), VB_S(0), VB_W(1), VB_H(1),  
		TEX_W(4), TEX_W(1), TEX_W(1), TEX_W(1),
		TEX_W(4), TEX_W(2), TEX_W(1), TEX_W(1),
		opengl_mb_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 1, VKB_In_Game, 0,
		NextWeapon_Action},

	// left top
	{VB_W(1) + VB_S(3), VB_S(2) + VB_W(2), VB_W(1), VB_H(1), 
		VB_W(1) + VB_S(3), VB_S(2) + VB_W(2), VB_W(1), VB_H(1),  
		TEX_W(4), TEX_W(5), TEX_W(1), TEX_W(1),
		TEX_W(4), TEX_W(6), TEX_W(1), TEX_W(1),
		opengl_s_base, opengl_e_base, btrue, VKB_BUTTON_Z_BASE + 1, VKB_In_Game, 0,
		SideStep_Action},
	{VB_W(2) + VB_S(4), VB_S(2) + VB_W(2), VB_W(1), VB_H(1), 
		VB_W(2) + VB_S(4), VB_S(2) + VB_W(2), VB_W(1), VB_H(1),  
		TEX_W(3), TEX_W(5), TEX_W(1), TEX_W(1),
		TEX_W(3), TEX_W(6), TEX_W(1), TEX_W(1),
		opengl_s_base, opengl_e_base, btrue, VKB_BUTTON_Z_BASE + 1, VKB_In_Game, 0,
		MouseLook_Action},
	{VB_W(3) + VB_S(5), VB_H(2) + VB_S(2), VB_W(1), VB_H(1), 
		VB_W(3) + VB_S(5), VB_H(2) + VB_S(2), VB_W(1), VB_H(1),  
		TEX_W(3), TEX_W(3), TEX_W(1), TEX_W(1),
		TEX_W(3), TEX_W(4), TEX_W(1), TEX_W(1),
		opengl_s_base, opengl_e_base, btrue, VKB_BUTTON_Z_BASE + 1, VKB_In_Game, 0,
		ShowPDA_Action},
	/*
	{VB_W(4) + VB_S(6), VB_H(2) + VB_S(2), VB_W(1), VB_H(1), 
		VB_W(4) + VB_S(6), VB_H(2) + VB_S(2), VB_W(1), VB_H(1),  
		TEX_W(3), TEX_W(3), TEX_W(1), TEX_W(1),
		TEX_W(3), TEX_W(4), TEX_W(1), TEX_W(1),
		opengl_s_base, opengl_e_base, btrue, VKB_BUTTON_Z_BASE + 1, VKB_In_Game, 0,
		Score_Action},
		*/

	// right top
	{VB_W(1) + VB_S(1), VB_H(2) + VB_S(2), VB_W(1), VB_H(1), 
		VB_W(1) + VB_S(1), VB_H(2) + VB_S(2), VB_W(1), VB_H(1),  
		TEX_W(1), TEX_W(5), TEX_W(1), TEX_W(1),
		TEX_W(1), TEX_W(6), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_e_base, btrue, VKB_BUTTON_Z_BASE + 1, VKB_In_Game, 0,
		FlashLight_Action},
	{VB_W(2) + VB_S(2), VB_H(2) + VB_S(2), VB_W(1), VB_H(1), 
		VB_W(2) + VB_S(2), VB_H(2) + VB_S(2), VB_W(1), VB_H(1),  
		TEX_W(7), TEX_W(1), TEX_W(1), TEX_W(1),
		TEX_W(7), TEX_W(2), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_e_base, btrue, VKB_BUTTON_Z_BASE + 1, VKB_In_Game, 0,
		Fist_Action},

	// top
	{VB_S(1) / 4 * 9 + VB_W(5), VB_H(1), VB_W(1), VB_H(1), 
		VB_S(1) / 4 * 9 + VB_W(5), VB_H(1), VB_W(1), VB_H(1),  
		TEX_W(6), TEX_W(1), TEX_W(1), TEX_W(1),
		TEX_W(6), TEX_W(2), TEX_W(1), TEX_W(1),
		opengl_mb_base, opengl_e_base, btrue, VKB_BUTTON_Z_BASE + 1, VKB_In_Game, 0,
		Weapon1_Action},
	{VB_S(1) / 4 * 7 + VB_W(4), VB_H(1), VB_W(1), VB_H(1), 
		VB_S(1) / 4 * 7 + VB_W(4), VB_H(1), VB_W(1), VB_H(1),  
		TEX_W(7), TEX_W(1), TEX_W(1), TEX_W(1),
		TEX_W(7), TEX_W(2), TEX_W(1), TEX_W(1),
		opengl_mb_base, opengl_e_base, btrue, VKB_BUTTON_Z_BASE + 1, VKB_In_Game, 0,
		Weapon2_Action},
	{VB_S(1) / 4 * 5 + VB_W(3), VB_H(1), VB_W(1), VB_H(1), 
		VB_S(1) / 4 * 5 + VB_W(3), VB_H(1), VB_W(1), VB_H(1),  
		TEX_W(7), TEX_W(5), TEX_W(1), TEX_W(1),
		TEX_W(7), TEX_W(6), TEX_W(1), TEX_W(1),
		opengl_mb_base, opengl_e_base, btrue, VKB_BUTTON_Z_BASE + 1, VKB_In_Game, 0,
		Weapon3_Action},
	{VB_S(1) / 4 * 3 + VB_W(2), VB_H(1), VB_W(1), VB_H(1), 
		VB_S(1) / 4 * 3 + VB_W(2), VB_H(1), VB_W(1), VB_H(1),  
		TEX_W(3), TEX_W(5), TEX_W(1), TEX_W(1),
		TEX_W(3), TEX_W(6), TEX_W(1), TEX_W(1),
		opengl_mb_base, opengl_e_base, btrue, VKB_BUTTON_Z_BASE + 1, VKB_In_Game, 0,
		Weapon4_Action},
	{VB_S(1) / 4 + VB_W(1), VB_H(1), VB_W(1), VB_H(1), 
		VB_S(1) / 4 + VB_W(1), VB_H(1), VB_W(1), VB_H(1),  
		TEX_W(1), TEX_W(5), TEX_W(1), TEX_W(1),
		TEX_W(1), TEX_W(6), TEX_W(1), TEX_W(1),
		opengl_mb_base, opengl_e_base, btrue, VKB_BUTTON_Z_BASE + 1, VKB_In_Game, 0,
		Weapon5_Action},
	{VB_S(1) / 4, VB_H(1), VB_W(1), VB_H(1), 
		VB_S(1) / 4, VB_H(1), VB_W(1), VB_H(1),  
		TEX_W(2), TEX_W(5), TEX_W(1), TEX_W(1),
		TEX_W(2), TEX_W(6), TEX_W(1), TEX_W(1),
		opengl_mf_base, opengl_e_base, btrue, VKB_BUTTON_Z_BASE + 1, VKB_In_Game, 0,
		Weapon6_Action},
	{VB_S(1) / 4 * 3 + VB_W(1), VB_H(1), VB_W(1), VB_H(1), 
		VB_S(1) / 4 * 3 + VB_W(1), VB_H(1), VB_W(1), VB_H(1),  
		TEX_W(3), TEX_W(5), TEX_W(1), TEX_W(1),
		TEX_W(3), TEX_W(6), TEX_W(1), TEX_W(1),
		opengl_mf_base, opengl_e_base, btrue, VKB_BUTTON_Z_BASE + 1, VKB_In_Game, 0,
		Weapon7_Action},
	{VB_S(1) / 4 * 5 + VB_W(2), VB_H(1), VB_W(1), VB_H(1), 
		VB_S(1) / 4 * 5 + VB_W(2), VB_H(1), VB_W(1), VB_H(1),  
		TEX_W(0), TEX_W(3), TEX_W(1), TEX_W(1),
		TEX_W(0), TEX_W(4), TEX_W(1), TEX_W(1),
		opengl_mf_base, opengl_e_base, btrue, VKB_BUTTON_Z_BASE + 1, VKB_In_Game, 0,
		Weapon8_Action},
	{VB_S(1) / 4 * 7 + VB_W(3), VB_H(1), VB_W(1), VB_H(1), 
		VB_S(1) / 4 * 7 + VB_W(3), VB_H(1), VB_W(1), VB_H(1),  
		TEX_W(1), TEX_W(3), TEX_W(1), TEX_W(1),
		TEX_W(1), TEX_W(4), TEX_W(1), TEX_W(1),
		opengl_mf_base, opengl_e_base, btrue, VKB_BUTTON_Z_BASE + 1, VKB_In_Game, 0,
		Weapon9_Action},
	{VB_S(1) / 4 * 9 + VB_W(4), VB_H(1), VB_W(1), VB_H(1), 
		VB_S(1) / 4 * 9 + VB_W(4), VB_H(1), VB_W(1), VB_H(1),  
		TEX_W(6), TEX_W(5), TEX_W(1), TEX_W(1),
		TEX_W(6), TEX_W(6), TEX_W(1), TEX_W(1),
		opengl_mf_base, opengl_e_base, btrue, VKB_BUTTON_Z_BASE + 1, VKB_In_Game, 0,
		Weapon10_Action},

	// menu
	{VB_S(1) + VB_W(1), VB_S(1) / 2, VB_W(1), VB_H(1),
		VB_S(1) + VB_W(1), VB_S(1) / 2, VB_W(1), VB_H(1),
		TEX_W(2), TEX_W(3), TEX_W(1), TEX_W(1),
		TEX_W(2), TEX_W(4), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_mf_base, btrue, VKB_CURSOR_Z_BASE + 100, VKB_In_Menu, 0,
		Enter_Key},
	{VB_S(1) + VB_W(1), VB_H(1) + VB_S(1) / 2, VB_W(1), VB_H(1),
		VB_S(1) + VB_W(1), VB_H(1) + VB_S(1) / 2, VB_W(1), VB_H(1),
		TEX_W(3), TEX_W(3), TEX_W(1), TEX_W(1),
		TEX_W(3), TEX_W(4), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_mb_base, btrue, VKB_CURSOR_Z_BASE + 100, VKB_In_Menu, 0,
		Space_Key}, // space key for stop resfrsh on multiplayer server menu.
	
	// general
	{0, VB_H(1), VB_W(1), VB_H(1), 
		0, VB_H(1), VB_W(1), VB_H(1),  
		TEX_W(7), TEX_W(5), TEX_W(1), TEX_W(1),
		TEX_W(7), TEX_W(6), TEX_W(1), TEX_W(1),
		opengl_s_base, opengl_e_base, btrue, VKB_BUTTON_Z_BASE + 200, VKB_In_Menu | VKB_In_Console | VKB_In_Game | VKB_In_Loading, 0,
		Console_Key},
	{VB_W(1), VB_H(1), VB_W(1), VB_H(1), 
		VB_W(1), VB_H(1), VB_W(1), VB_H(1),  
		TEX_W(1), TEX_W(1), TEX_W(1), TEX_W(1),
		TEX_W(1), TEX_W(2), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_e_base, btrue, VKB_BUTTON_Z_BASE + 500, VKB_In_Menu | VKB_In_Console | VKB_In_Game | VKB_In_Loading, 0,
		Escape_Key},

	// second
	{VB_S(2), VB_S(2) + VB_H(2), VB_W(1), VB_H(1), 
		VB_S(2), VB_S(2) + VB_H(2), VB_W(1), VB_H(1),  
		TEX_W(2), TEX_W(3), TEX_W(1), TEX_W(1),
		TEX_W(2), TEX_W(4), TEX_W(1), TEX_W(1),
		opengl_s_base, opengl_e_base, btrue, VKB_BUTTON_Z_BASE + 200, VKB_In_Game, 0,
		Attack_Action},

	// 1 - 9 - 0
	{VB_W2(1) + VB_S2(1) + VB_W(1), VB_H2(4) + VB_S2(4), VB_W2(1), VB_H2(1), 
		VB_W2(1) + VB_S2(1) + VB_W(1), VB_H2(4) + VB_S2(4), VB_W2(1), VB_H2(1),  
		TEX_W(1), TEX_W(3), TEX_W(1), TEX_W(1),
		TEX_W(1), TEX_W(4), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 200, VKB_In_Console | VKB_In_Message, 3,
		Num0_Key},
	{VB_W2(2) + VB_S2(2) + VB_W(1), VB_H2(4) + VB_S2(4), VB_W2(1), VB_H2(1), 
		VB_W2(2) + VB_S2(2) + VB_W(1), VB_H2(4) + VB_S2(4), VB_W2(1), VB_H2(1),  
		TEX_W(0), TEX_W(3), TEX_W(1), TEX_W(1),
		TEX_W(0), TEX_W(4), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 200, VKB_In_Console | VKB_In_Message, 3,
		Num9_Key},
	{VB_W2(3) + VB_S2(3) + VB_W(1), VB_H2(4) + VB_S2(4), VB_W2(1), VB_H2(1), 
		VB_W2(3) + VB_S2(3) + VB_W(1), VB_H2(4) + VB_S2(4), VB_W2(1), VB_H2(1),  
		TEX_W(7), TEX_W(1), TEX_W(1), TEX_W(1),
		TEX_W(7), TEX_W(2), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 200, VKB_In_Console | VKB_In_Message, 3,
		Num8_Key},
	{VB_W2(4) + VB_S2(4) + VB_W(1), VB_H2(4) + VB_S2(4), VB_W2(1), VB_H2(1), 
		VB_W2(4) + VB_S2(4) + VB_W(1), VB_H2(4) + VB_S2(4), VB_W2(1), VB_H2(1),  
		TEX_W(6), TEX_W(1), TEX_W(1), TEX_W(1),
		TEX_W(6), TEX_W(2), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 200, VKB_In_Console | VKB_In_Message, 3,
		Num7_Key},
	{VB_W2(5) + VB_S2(5) + VB_W(1), VB_H2(4) + VB_S2(4), VB_W2(1), VB_H2(1), 
		VB_W2(5) + VB_S2(5) + VB_W(1), VB_H2(4) + VB_S2(4), VB_W2(1), VB_H2(1),  
		TEX_W(5), TEX_W(1), TEX_W(1), TEX_W(1),
		TEX_W(5), TEX_W(2), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 200, VKB_In_Console | VKB_In_Message, 3,
		Num6_Key},
	{VB_W2(6) + VB_S2(6) + VB_W(1), VB_H2(4) + VB_S2(4), VB_W2(1), VB_H2(1), 
		VB_W2(6) + VB_S2(6) + VB_W(1), VB_H2(4) + VB_S2(4), VB_W2(1), VB_H2(1),  
		TEX_W(4), TEX_W(1), TEX_W(1), TEX_W(1),
		TEX_W(4), TEX_W(2), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 200, VKB_In_Console | VKB_In_Message, 3,
		Num5_Key},
	{VB_W2(7) + VB_S2(7) + VB_W(1), VB_H2(4) + VB_S2(4), VB_W2(1), VB_H2(1), 
		VB_W2(7) + VB_S2(7) + VB_W(1), VB_H2(4) + VB_S2(4), VB_W2(1), VB_H2(1),  
		TEX_W(3), TEX_W(1), TEX_W(1), TEX_W(1),
		TEX_W(3), TEX_W(2), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 200, VKB_In_Console | VKB_In_Message, 3,
		Num4_Key},
	{VB_W2(8) + VB_S2(8) + VB_W(1), VB_H2(4) + VB_S2(4), VB_W2(1), VB_H2(1), 
		VB_W2(8) + VB_S2(8) + VB_W(1), VB_H2(4) + VB_S2(4), VB_W2(1), VB_H2(1),  
		TEX_W(2), TEX_W(1), TEX_W(1), TEX_W(1),
		TEX_W(2), TEX_W(2), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 200, VKB_In_Console | VKB_In_Message, 3,
		Num3_Key},
	{VB_W2(9) + VB_S2(9) + VB_W(1), VB_H2(4) + VB_S2(4), VB_W2(1), VB_H2(1), 
		VB_W2(9) + VB_S2(9) + VB_W(1), VB_H2(4) + VB_S2(4), VB_W2(1), VB_H2(1),  
		TEX_W(1), TEX_W(1), TEX_W(1), TEX_W(1),
		TEX_W(1), TEX_W(2), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 200, VKB_In_Console | VKB_In_Message, 3,
		Num2_Key},
	{VB_W2(10) + VB_S2(10) + VB_W(1), VB_H2(4) + VB_S2(4), VB_W2(1), VB_H2(1), 
		VB_W2(10) + VB_S2(10) + VB_W(1), VB_H2(4) + VB_S2(4), VB_W2(1), VB_H2(1),  
		TEX_W(0), TEX_W(1), TEX_W(1), TEX_W(1),
		TEX_W(0), TEX_W(2), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 200, VKB_In_Console | VKB_In_Message, 3,
		Num1_Key},

		// p - q
	{VB_W2(1) + VB_S2(1) + VB_W(1), VB_H2(3) + VB_S2(3), VB_W2(1), VB_H2(1), 
		VB_W2(1) + VB_S2(1) + VB_W(1), VB_H2(3) + VB_S2(3), VB_W2(1), VB_H2(1),  
		TEX_W(1), TEX_W(3), TEX_W(1), TEX_W(1),
		TEX_W(1), TEX_W(4), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 200, VKB_In_Console | VKB_In_Message, 2,
		P_Key},
	{VB_W2(2) + VB_S2(2) + VB_W(1), VB_H2(3) + VB_S2(3), VB_W2(1), VB_H2(1), 
		VB_W2(2) + VB_S2(2) + VB_W(1), VB_H2(3) + VB_S2(3), VB_W2(1), VB_H2(1),  
		TEX_W(0), TEX_W(3), TEX_W(1), TEX_W(1),
		TEX_W(0), TEX_W(4), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 200, VKB_In_Console | VKB_In_Message, 2,
		O_Key},
	{VB_W2(3) + VB_S2(3) + VB_W(1), VB_H2(3) + VB_S2(3), VB_W2(1), VB_H2(1), 
		VB_W2(3) + VB_S2(3) + VB_W(1), VB_H2(3) + VB_S2(3), VB_W2(1), VB_H2(1),  
		TEX_W(7), TEX_W(1), TEX_W(1), TEX_W(1),
		TEX_W(7), TEX_W(2), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 200, VKB_In_Console | VKB_In_Message, 2,
		I_Key},
	{VB_W2(4) + VB_S2(4) + VB_W(1), VB_H2(3) + VB_S2(3), VB_W2(1), VB_H2(1), 
		VB_W2(4) + VB_S2(4) + VB_W(1), VB_H2(3) + VB_S2(3), VB_W2(1), VB_H2(1),  
		TEX_W(6), TEX_W(1), TEX_W(1), TEX_W(1),
		TEX_W(6), TEX_W(2), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 200, VKB_In_Console | VKB_In_Message, 2,
		U_Key},
	{VB_W2(5) + VB_S2(5) + VB_W(1), VB_H2(3) + VB_S2(3), VB_W2(1), VB_H2(1), 
		VB_W2(5) + VB_S2(5) + VB_W(1), VB_H2(3) + VB_S2(3), VB_W2(1), VB_H2(1),  
		TEX_W(5), TEX_W(1), TEX_W(1), TEX_W(1),
		TEX_W(5), TEX_W(2), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 200, VKB_In_Console | VKB_In_Message, 2,
		Y_Key},
	{VB_W2(6) + VB_S2(6) + VB_W(1), VB_H2(3) + VB_S2(3), VB_W2(1), VB_H2(1), 
		VB_W2(6) + VB_S2(6) + VB_W(1), VB_H2(3) + VB_S2(3), VB_W2(1), VB_H2(1),  
		TEX_W(4), TEX_W(1), TEX_W(1), TEX_W(1),
		TEX_W(4), TEX_W(2), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 200, VKB_In_Console | VKB_In_Message, 2,
		T_Key},
	{VB_W2(7) + VB_S2(7) + VB_W(1), VB_H2(3) + VB_S2(3), VB_W2(1), VB_H2(1), 
		VB_W2(7) + VB_S2(7) + VB_W(1), VB_H2(3) + VB_S2(3), VB_W2(1), VB_H2(1),  
		TEX_W(3), TEX_W(1), TEX_W(1), TEX_W(1),
		TEX_W(3), TEX_W(2), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 200, VKB_In_Console | VKB_In_Message, 2,
		R_Key},
	{VB_W2(8) + VB_S2(8) + VB_W(1), VB_H2(3) + VB_S2(3), VB_W2(1), VB_H2(1), 
		VB_W2(8) + VB_S2(8) + VB_W(1), VB_H2(3) + VB_S2(3), VB_W2(1), VB_H2(1),  
		TEX_W(2), TEX_W(1), TEX_W(1), TEX_W(1),
		TEX_W(2), TEX_W(2), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 200, VKB_In_Console | VKB_In_Message, 2,
		E_Key},
	{VB_W2(9) + VB_S2(9) + VB_W(1), VB_H2(3) + VB_S2(3), VB_W2(1), VB_H2(1), 
		VB_W2(9) + VB_S2(9) + VB_W(1), VB_H2(3) + VB_S2(3), VB_W2(1), VB_H2(1),  
		TEX_W(1), TEX_W(1), TEX_W(1), TEX_W(1),
		TEX_W(1), TEX_W(2), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 200, VKB_In_Console | VKB_In_Message, 2,
		W_Key},
	{VB_W2(10) + VB_S2(10) + VB_W(1), VB_H2(3) + VB_S2(3), VB_W2(1), VB_H2(1), 
		VB_W2(10) + VB_S2(10) + VB_W(1), VB_H2(3) + VB_S2(3), VB_W2(1), VB_H2(1),  
		TEX_W(0), TEX_W(1), TEX_W(1), TEX_W(1),
		TEX_W(0), TEX_W(2), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 200, VKB_In_Console | VKB_In_Message, 2,
		Q_Key},

	// l - a
	{VB_W2(1) + VB_S2(1) + VB_W(1) + VB_W2(1) / 2 + VB_S2(1) / 2, VB_H2(2) + VB_S2(2), VB_W2(1), VB_H2(1), 
		VB_W2(1) + VB_S2(1) + VB_W(1) + VB_W2(1) / 2 + VB_S2(1) / 2, VB_H2(2) + VB_S2(2), VB_W2(1), VB_H2(1),  
		TEX_W(2), TEX_W(5), TEX_W(1), TEX_W(1),
		TEX_W(2), TEX_W(6), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 200, VKB_In_Console | VKB_In_Message, 2,
		L_Key},
	{VB_W2(2) + VB_S2(2) + VB_W(1) + VB_W2(1) / 2 + VB_S2(1) / 2, VB_H2(2) + VB_S2(2), VB_W2(1), VB_H2(1), 
		VB_W2(2) + VB_S2(2) + VB_W(1) + VB_W2(1) / 2 + VB_S2(1) / 2, VB_H2(2) + VB_S2(2), VB_W2(1), VB_H2(1),  
		TEX_W(1), TEX_W(5), TEX_W(1), TEX_W(1),
		TEX_W(1), TEX_W(6), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 200, VKB_In_Console | VKB_In_Message, 2,
		K_Key},
	{VB_W2(3) + VB_S2(3) + VB_W(1) + VB_W2(1) / 2 + VB_S2(1) / 2, VB_H2(2) + VB_S2(2), VB_W2(1), VB_H2(1), 
		VB_W2(3) + VB_S2(3) + VB_W(1) + VB_W2(1) / 2 + VB_S2(1) / 2, VB_H2(2) + VB_S2(2), VB_W2(1), VB_H2(1),  
		TEX_W(0), TEX_W(5), TEX_W(1), TEX_W(1),
		TEX_W(0), TEX_W(6), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 200, VKB_In_Console | VKB_In_Message, 2,
		J_Key},
	{VB_W2(4) + VB_S2(4) + VB_W(1) + VB_W2(1) / 2 + VB_S2(1) / 2, VB_H2(2) + VB_S2(2), VB_W2(1), VB_H2(1), 
		VB_W2(4) + VB_S2(4) + VB_W(1) + VB_W2(1) / 2 + VB_S2(1) / 2, VB_H2(2) + VB_S2(2), VB_W2(1), VB_H2(1),  
		TEX_W(7), TEX_W(3), TEX_W(1), TEX_W(1),
		TEX_W(7), TEX_W(4), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 200, VKB_In_Console | VKB_In_Message, 2,
		H_Key},
	{VB_W2(5) + VB_S2(5) + VB_W(1) + VB_W2(1) / 2 + VB_S2(1) / 2, VB_H2(2) + VB_S2(2), VB_W2(1), VB_H2(1), 
		VB_W2(5) + VB_S2(5) + VB_W(1) + VB_W2(1) / 2 + VB_S2(1) / 2, VB_H2(2) + VB_S2(2), VB_W2(1), VB_H2(1),  
		TEX_W(6), TEX_W(3), TEX_W(1), TEX_W(1),
		TEX_W(6), TEX_W(4), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 200, VKB_In_Console | VKB_In_Message, 2,
		G_Key},
	{VB_W2(6) + VB_S2(6) + VB_W(1) + VB_W2(1) / 2 + VB_S2(1) / 2, VB_H2(2) + VB_S2(2), VB_W2(1), VB_H2(1), 
		VB_W2(6) + VB_S2(6) + VB_W(1) + VB_W2(1) / 2 + VB_S2(1) / 2, VB_H2(2) + VB_S2(2), VB_W2(1), VB_H2(1),  
		TEX_W(5), TEX_W(3), TEX_W(1), TEX_W(1),
		TEX_W(5), TEX_W(4), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 200, VKB_In_Console | VKB_In_Message, 2,
		F_Key},
	{VB_W2(7) + VB_S2(7) + VB_W(1) + VB_W2(1) / 2 + VB_S2(1) / 2, VB_H2(2) + VB_S2(2), VB_W2(1), VB_H2(1), 
		VB_W2(7) + VB_S2(7) + VB_W(1) + VB_W2(1) / 2 + VB_S2(1) / 2, VB_H2(2) + VB_S2(2), VB_W2(1), VB_H2(1),  
		TEX_W(4), TEX_W(3), TEX_W(1), TEX_W(1),
		TEX_W(4), TEX_W(4), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 200, VKB_In_Console | VKB_In_Message, 2,
		D_Key},
	{VB_W2(8) + VB_S2(8) + VB_W(1) + VB_W2(1) / 2 + VB_S2(1) / 2, VB_H2(2) + VB_S2(2), VB_W2(1), VB_H2(1), 
		VB_W2(8) + VB_S2(8) + VB_W(1) + VB_W2(1) / 2 + VB_S2(1) / 2, VB_H2(2) + VB_S2(2), VB_W2(1), VB_H2(1),  
		TEX_W(3), TEX_W(3), TEX_W(1), TEX_W(1),
		TEX_W(3), TEX_W(4), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 200, VKB_In_Console | VKB_In_Message, 2,
		S_Key},
	{VB_W2(9) + VB_S2(9) + VB_W(1) + VB_W2(1) / 2 + VB_S2(1) / 2, VB_H2(2) + VB_S2(2), VB_W2(1), VB_H2(1), 
		VB_W2(9) + VB_S2(9) + VB_W(1) + VB_W2(1) / 2 + VB_S2(1) / 2, VB_H2(2) + VB_S2(2), VB_W2(1), VB_H2(1),  
		TEX_W(2), TEX_W(3), TEX_W(1), TEX_W(1),
		TEX_W(2), TEX_W(4), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 200, VKB_In_Console | VKB_In_Message, 2,
		A_Key},

	// m - z
	{VB_W2(1) + VB_S2(1) + VB_W(1) + VB_W2(3) / 2 + VB_S2(3) / 2, VB_H2(1) + VB_S2(1), VB_W2(1), VB_H2(1), 
		VB_W2(1) + VB_S2(1) + VB_W(1) + VB_W2(3) / 2 + VB_S2(3) / 2, VB_H2(1) + VB_S2(1), VB_W2(1), VB_H2(1),  
		TEX_W(1), TEX_W(7), TEX_W(1), TEX_W(1),
		TEX_W(1), TEX_W(8), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 200, VKB_In_Console | VKB_In_Message, 2,
		M_Key},
	{VB_W2(2) + VB_S2(2) + VB_W(1) + VB_W2(3) / 2 + VB_S2(3) / 2, VB_H2(1) + VB_S2(1), VB_W2(1), VB_H2(1), 
		VB_W2(2) + VB_S2(2) + VB_W(1) + VB_W2(3) / 2 + VB_S2(3) / 2, VB_H2(1) + VB_S2(1), VB_W2(1), VB_H2(1),  
		TEX_W(0), TEX_W(7), TEX_W(1), TEX_W(1),
		TEX_W(0), TEX_W(8), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 200, VKB_In_Console | VKB_In_Message, 2,
		N_Key},
	{VB_W2(3) + VB_S2(3) + VB_W(1) + VB_W2(3) / 2 + VB_S2(3) / 2, VB_H2(1) + VB_S2(1), VB_W2(1), VB_H2(1), 
		VB_W2(3) + VB_S2(3) + VB_W(1) + VB_W2(3) / 2 + VB_S2(3) / 2, VB_H2(1) + VB_S2(1), VB_W2(1), VB_H2(1),  
		TEX_W(7), TEX_W(5), TEX_W(1), TEX_W(1),
		TEX_W(7), TEX_W(6), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 200, VKB_In_Console | VKB_In_Message, 2,
		B_Key},
	{VB_W2(4) + VB_S2(4) + VB_W(1) + VB_W2(3) / 2 + VB_S2(3) / 2, VB_H2(1) + VB_S2(1), VB_W2(1), VB_H2(1), 
		VB_W2(4) + VB_S2(4) + VB_W(1) + VB_W2(3) / 2 + VB_S2(3) / 2, VB_H2(1) + VB_S2(1), VB_W2(1), VB_H2(1),  
		TEX_W(6), TEX_W(5), TEX_W(1), TEX_W(1),
		TEX_W(6), TEX_W(6), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 200, VKB_In_Console | VKB_In_Message, 2,
		V_Key},
	{VB_W2(5) + VB_S2(5) + VB_W(1) + VB_W2(3) / 2 + VB_S2(3) / 2, VB_H2(1) + VB_S2(1), VB_W2(1), VB_H2(1), 
		VB_W2(5) + VB_S2(5) + VB_W(1) + VB_W2(3) / 2 + VB_S2(3) / 2, VB_H2(1) + VB_S2(1), VB_W2(1), VB_H2(1),  
		TEX_W(5), TEX_W(5), TEX_W(1), TEX_W(1),
		TEX_W(5), TEX_W(6), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 200, VKB_In_Console | VKB_In_Message, 2,
		C_Key},
	{VB_W2(6) + VB_S2(6) + VB_W(1) + VB_W2(3) / 2 + VB_S2(3) / 2, VB_H2(1) + VB_S2(1), VB_W2(1), VB_H2(1), 
		VB_W2(6) + VB_S2(6) + VB_W(1) + VB_W2(3) / 2 + VB_S2(3) / 2, VB_H2(1) + VB_S2(1), VB_W2(1), VB_H2(1),  
		TEX_W(4), TEX_W(5), TEX_W(1), TEX_W(1),
		TEX_W(4), TEX_W(6), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 200, VKB_In_Console | VKB_In_Message, 2,
		X_Key},
	{VB_W2(7) + VB_S2(7) + VB_W(1) + VB_W2(3) / 2 + VB_S2(3) / 2, VB_H2(1) + VB_S2(1), VB_W2(1), VB_H2(1), 
		VB_W2(7) + VB_S2(7) + VB_W(1) + VB_W2(3) / 2 + VB_S2(3) / 2, VB_H2(1) + VB_S2(1), VB_W2(1), VB_H2(1),  
		TEX_W(3), TEX_W(5), TEX_W(1), TEX_W(1),
		TEX_W(3), TEX_W(6), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 200, VKB_In_Console | VKB_In_Message, 2,
		Z_Key},
	
	// _ - . / + = ! ?
	{VB_W2(1) + VB_S2(1) + VB_W(1), VB_S2(0), VB_W2(1), VB_H2(1), 
		VB_W2(1) + VB_S2(1) + VB_W(1), VB_S2(0), VB_W2(1), VB_H2(1),  
		TEX_W(0), TEX_W(5), TEX_W(1), TEX_W(1),
		TEX_W(0), TEX_W(6), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 200, VKB_In_Console | VKB_In_Message, 3,
		Question_Key},
	{VB_W2(2) + VB_S2(2) + VB_W(1), VB_S2(0), VB_W2(1), VB_H2(1), 
		VB_W2(2) + VB_S2(2) + VB_W(1), VB_S2(0), VB_W2(1), VB_H2(1),  
		TEX_W(6), TEX_W(7), TEX_W(1), TEX_W(1),
		TEX_W(6), TEX_W(8), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 200, VKB_In_Console | VKB_In_Message, 2,
		Period_Key},
	{VB_W2(3) + VB_S2(3) + VB_W(1), VB_S2(0), VB_W2(1), VB_H2(1), 
		VB_W2(3) + VB_S2(3) + VB_W(1), VB_S2(0), VB_W2(1), VB_H2(1),  
		TEX_W(3), TEX_W(5), TEX_W(1), TEX_W(1),
		TEX_W(3), TEX_W(6), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 200, VKB_In_Console | VKB_In_Message, 3,
		BackSlash_Key},
	{VB_W2(4) + VB_S2(4) + VB_W(1), VB_S2(0), VB_W2(1), VB_H2(1), 
		VB_W2(4) + VB_S2(4) + VB_W(1), VB_S2(0), VB_W2(1), VB_H2(1),  
		TEX_W(2), TEX_W(5), TEX_W(1), TEX_W(1),
		TEX_W(2), TEX_W(6), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 200, VKB_In_Console | VKB_In_Message, 3,
		Slash_Key},

	{VB_W2(6) + VB_S2(6) + VB_W(1), VB_S2(0), VB_W2(2) + VB_S2(1), VB_H2(1), 
		VB_W2(6) + VB_S2(6) + VB_W(1), VB_S2(0), VB_W2(2) + VB_S2(1), VB_H2(1),  
		TEX_W(6), TEX_W(7), TEX_W(1), TEX_W(1),
		TEX_W(6), TEX_W(8), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 200, VKB_In_Console | VKB_In_Message, 4,
		Space_Key},

	{VB_W2(7) + VB_S2(7) + VB_W(1), VB_S2(0), VB_W2(1), VB_H2(1), 
		VB_W2(7) + VB_S2(7) + VB_W(1), VB_S2(0), VB_W2(1), VB_H2(1),  
		TEX_W(3), TEX_W(7), TEX_W(1), TEX_W(1),
		TEX_W(3), TEX_W(8), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 200, VKB_In_Console | VKB_In_Message, 2,
		Underscore_Key},
	{VB_W2(8) + VB_S2(8) + VB_W(1), VB_S2(0), VB_W2(1), VB_H2(1), 
		VB_W2(8) + VB_S2(8) + VB_W(1), VB_S2(0), VB_W2(1), VB_H2(1),  
		TEX_W(2), TEX_W(7), TEX_W(1), TEX_W(1),
		TEX_W(2), TEX_W(8), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 200, VKB_In_Console | VKB_In_Message, 2,
		Minus_Key},
	{VB_W2(9) + VB_S2(9) + VB_W(1), VB_S2(0), VB_W2(1), VB_H2(1), 
		VB_W2(9) + VB_S2(9) + VB_W(1), VB_S2(0), VB_W2(1), VB_H2(1),  
		TEX_W(4), TEX_W(3), TEX_W(1), TEX_W(1),
		TEX_W(4), TEX_W(4), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 200, VKB_In_Console | VKB_In_Message, 3,
		Equal_Key},
	{VB_W2(10) + VB_S2(10) + VB_W(1), VB_S2(0), VB_W2(1), VB_H2(1), 
		VB_W2(10) + VB_S2(10) + VB_W(1), VB_S2(0), VB_W2(1), VB_H2(1),  
		TEX_W(3), TEX_W(3), TEX_W(1), TEX_W(1),
		TEX_W(3), TEX_W(4), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 200, VKB_In_Console | VKB_In_Message, 3,
		Plus_Key},


	// special
	{VB_W(1), VB_H(4) + VB_S(5), VB_W(1), VB_H(1), 
		VB_W(1), VB_H(4) + VB_S(5), VB_W(1), VB_H(1),  
		TEX_W(3), TEX_W(1), TEX_W(1), TEX_W(1),
		TEX_W(3), TEX_W(2), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 200, VKB_In_Console | VKB_In_Message, 0,
		PageUp_Key},
	{VB_W(1), VB_H(3) + VB_S(4), VB_W(1), VB_H(1), 
		VB_W(1), VB_H(3) + VB_S(4), VB_W(1), VB_H(1),  
		TEX_W(5), TEX_W(1), TEX_W(1), TEX_W(1),
		TEX_W(5), TEX_W(2), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 200, VKB_In_Console | VKB_In_Message, 0,
		PageDown_Key},
	
	{VB_W(1), VB_H(2) + VB_S(3), VB_W(1), VB_H(1), 
		VB_W(1), VB_H(2) + VB_S(3), VB_W(1), VB_H(1),  
		TEX_W(5), TEX_W(7), TEX_W(1), TEX_W(1),
		TEX_W(5), TEX_W(8), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 200, VKB_In_Console | VKB_In_Message, 4,
		Backspace_Key},

	{VB_W(1), VB_H(1) + VB_S(2), VB_W(1), VB_H(1), 
		VB_W(1), VB_H(1) + VB_S(2), VB_W(1), VB_H(1),  
		TEX_W(6), TEX_W(6), TEX_W(1), -TEX_W(1),
		TEX_W(6), TEX_W(7), TEX_W(1), -TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 200, VKB_In_Console | VKB_In_Message, 4,
		Tab_Key},
	{VB_W(1), VB_H(0) + VB_S(1), VB_W(1), VB_H(1), 
		VB_W(1), VB_H(0) + VB_S(1), VB_W(1), VB_H(1),  
		TEX_W(7), TEX_W(7), TEX_W(1), TEX_W(1),
		TEX_W(7), TEX_W(8), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 200, VKB_In_Console | VKB_In_Message, 4,
		Enter_Key},
	/*
	{VB_W(1), VB_H(5) + VB_S(6), VB_W(1), VB_H(1), 
		VB_W(1), VB_H(5) + VB_S(6), VB_W(1), VB_H(1),  
		TEX_W(4), TEX_W(5), TEX_W(1), TEX_W(1),
		TEX_W(4), TEX_W(6), TEX_W(1), TEX_W(1),
		opengl_e_base, opengl_s_base, btrue, VKB_BUTTON_Z_BASE + 200, VKB_In_Console | VKB_In_Message, 0,
		Tab_Key},
		*/
};

int karinGetActionData(unsigned action, int *ikeys, unsigned int key_max, unsigned int *key_count, char *cmd, unsigned int cmd_max)
{
	if(!ikeys || key_max == 0 || !key_count || !cmd || cmd_max == 0)
		return -1; // data is null
	if(action >= Total_Key)
		return -2; // action is invalid

	int r = Invalid_Data;

	if(action < Total_Action)
	{
		r = Cmd_Data;
		ikeys[0] = action + 1;
		*key_count = 1;
	}
	else if(action > Total_Action && action < Total_Key)
	{
		ikeys[0] = Key_Defs[action - Up_Key];
		*key_count = 1;
		r = (action >= Mouse1_Button ? Button_Data : (action <= Enter_Key ? Key_Data : Char_Data));
	}
	return r;
}

float karinFormatAngle(float angle)
{
	int i = (int)angle;
	float f = angle - i;
	float r = 0.0;
	if(angle > 360)
	{
		r = i % 360 + f;
	}
	else if(angle < 0)
	{
		r = 360 - abs(i % 360 + f);
	}
	else
		r = angle;
	if(r == 360.0)
		r = 0.0;
	return r;
}

void karinSetClientState(unsigned state)
{
	if(state & VKB_In_Console)
		client_state = VKB_In_Console;
	else if(state & VKB_In_Game)
		client_state = VKB_In_Game;
	else
		client_state = state;
}

