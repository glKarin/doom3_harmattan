#include "gl_vkb.h"
#include "../renderer/qgl.h"
#include <GLES/gl.h>

#include "q3_png.h"

#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>

#define FLOAT_TO_INT // round
#define VB_P(p) (int)FLOAT_TO_INT((p) * vb_p)

#define MOUSE_IN_RANGE(b, x, y) \
	((x >= (b).e_min_x && x <= (b).e_max_x ) && (y >= (b).e_min_y && y <= (b).e_max_y))

typedef struct _texture
{
	GLuint imaged;
	sizei width;
	sizei height;
	uenum format;
} texture;

typedef struct _virtual_control_base
{
	vkb_type type;
	float x;
	float y;
	int z;
	float width;
	float height;
	float e_min_x;
	float e_min_y;
	float e_max_x;
	float e_max_y;
	boolean pressed;
	boolean enabled;
	boolean visible;
	uint tex_index;
	GLuint buffers[Total_Coord];
	uint show_mask;
} virtual_control_base;

typedef struct _virtual_cursor
{
	virtual_control_base base;

	Game_Action action;
	Game_Action actions[4];
	uint mask[4];
	float angle;
	float distance;
	float ignore_distance;
	float e_radius;
	float pos_x;
	float pos_y;
	boolean render_bg;
} virtual_cursor;

typedef struct _virtual_swipe
{
	virtual_control_base base;

	Game_Action action;
	Game_Action actions[4];
	uint mask[4];
	float angle;
	float distance;
	float ignore_distance;
} virtual_swipe;

typedef struct _virtual_joystick
{
	virtual_control_base base;

	Game_Action actions[4];
	float e_radius;
	float angle;
	clampf percent;
	float e_ignore_radius;
	float pos_x;
	float pos_y;
} virtual_joystick;

typedef struct _virtual_button
{
	virtual_control_base base;

	Game_Action action;
} virtual_button;

typedef union _virtual_control_item
{
	vkb_type type;
	virtual_control_base base;
	virtual_cursor cursor;
	virtual_swipe swipe;
	virtual_joystick joystick;
	virtual_button button;
} virtual_control_item;

typedef struct _vkb
{
	float x;
	float y;
	float z;
	float width;
	float height;
	boolean inited;
	texture tex[VKB_TEX_COUNT];
	virtual_control_item vb[TOTAL_VKB_COUNT];
} vkb;

static void karinMakeJoystick(virtual_control_item *b, struct vkb_joystick *joystick, unsigned int width, unsigned int height);
static void karinMakeSwipe(virtual_control_item *b, struct vkb_swipe *swipe, unsigned int width, unsigned int height);
static void karinMakeButton(virtual_control_item *b, struct vkb_button *btn, unsigned int width, unsigned int height);
static void karinMakeCursor(virtual_control_item *b, struct vkb_cursor *cursor, unsigned int width, unsigned int height);
static void karinRenderVKBCursor(const virtual_cursor *b, const texture tex[]);
static void karinRenderVKBJoystick(const virtual_joystick *b, const texture tex[]);
static void karinRenderVKBSwipe(const virtual_swipe *b, const texture tex[]);
static void karinRenderVKBButton(const virtual_button *b, const texture tex[]);

static vkb the_vkb;

//void karinResizeVKB(float w, float h);
static mouse_motion_button_status karinButtonMouseMotion(const virtual_control_item *b, int nx, int ny, int lx, int ly)
{
	if(!b)
		return all_out_range_status;
	int	l = MOUSE_IN_RANGE(b->base, lx, ly) ? 1 : 0;
	int	n = MOUSE_IN_RANGE(b->base, nx, ny) ? 1 : 0;
	unsigned s = all_out_range_status;
	if(l)
		s |= last_in_now_out_range_status;
	if(n)
		s |= last_out_now_in_range_status;
	return (mouse_motion_button_status)(s);
}

static circle_direction karinGetJoystickDirection(int x, int y, float cx, float cy, float ir, float _or, float *angle, float *percent)
{
	int oh = _or / 2;
	int ih = ir / 2;
	int xl = x - cx;
	int yl = y - cy;
	int xls = xl >= 0 ? xl : -xl;
	int yls = yl >= 0 ? yl : -yl;
	if(xls > oh || yls > oh)
	{
		if(angle)
			*angle = -1.0f;
		if(percent)
			*percent = 0.0f;
		return circle_outside;
	}
	else if(xls < ih && yls < ih)
	{
		if(angle)
			*angle = -1.0f;
		if(percent)
			*percent = 0.0f;
		return circle_center;
	}

	double a = (double)(xl);
	double b = (double)(yl);
	//float ra = karinFormatAngle(atan2(xl, yl) * (180.0 / M_PI));
	float rb = karinFormatAngle(atan2(b, a) * (180.0 / M_PI));
	if(angle)
		*angle = rb;
	if(percent)
	{
		double c = sqrt(a * a + b * b);
		*percent = (float)c / oh;
	}
	//printf("%f - %f\n", ra, rb);
	if(rb >= 60 && rb <= 120)
		return circle_top_direction;
	else if((rb >= 0 && rb <= 30) || (rb >= 330 && rb <= 360))
		return circle_right_direction;
	else if(rb >= 240 && rb <= 300)
		return circle_bottom_direction;
	else if(rb >= 150 && rb <= 210)
		return circle_left_direction;

	else if(rb > 30 && rb < 60)
		return circle_righttop_direction;
	else if(rb > 300 && rb < 330)
		return circle_rightbottom_direction;
	else if(rb > 210 && rb < 240)
		return circle_leftbottom_direction;
	else
		return circle_lefttop_direction;
}

static circle_direction karinGetSwipeDirection(int dx, int dy, float *angle, float *distance)
{
	if(dx == 0 && dy == 0)
	{
		if(distance)
			*distance = 0.0f;
		if(*angle)
			*angle = -1.0f;
		return circle_center;
	}
	double a = (double)(dx);
	double b = (double)(dy);
	double c = sqrt(a * a + b * b);
	if(distance)
		*distance = (float)c;
	//float ra = karinFormatAngle(atan2(xl, yl) * (180.0 / M_PI));
	float rb = karinFormatAngle(atan2(b, a) * (180.0 / M_PI));
	if(*angle)
		*angle = rb;
	//printf("%f - %f\n", ra, rb);
	if(rb >= 60 && rb <= 120)
		return circle_top_direction;
	else if((rb >= 0 && rb <= 30) || (rb >= 330 && rb <= 360))
		return circle_right_direction;
	else if(rb >= 240 && rb <= 300)
		return circle_bottom_direction;
	else if(rb >= 150 && rb <= 210)
		return circle_left_direction;

	else if(rb > 30 && rb < 60)
		return circle_righttop_direction;
	else if(rb > 300 && rb < 330)
		return circle_rightbottom_direction;
	else if(rb > 210 && rb < 240)
		return circle_leftbottom_direction;
	else
		return circle_lefttop_direction;
}

static int karinGetControlItemOnPosition(int x, int y, int *r)
{
	if(!the_vkb.inited)
		return -1;
	int c = 0;
	int i;
	int count = TOTAL_VKB_COUNT;
	for(i = 0; i < count; i++)
	{
		const virtual_control_item *b = the_vkb.vb + i;
		if((b->base.show_mask & client_state) == 0)
			continue;
		if(!b->base.enabled)
			continue;
		if(MOUSE_IN_RANGE(b->base, x, y))
		{
			if(r)
				r[c] = i;
			c++;
		}
	}
	if(r)
	{
		int j;
		for(j = 0; j < c; j++)
		{
			int k;
			for(k = j + 1; k < c; k++)
			{
				const virtual_control_item *a = the_vkb.vb + r[j];
				const virtual_control_item *b = the_vkb.vb + r[k];
				if(b->base.z > a->base.z)
				{
					int t = r[j];
					r[j] = r[k];
					r[k] = t;
				}
			}
		}
	}
	return c;
}

static int karinGetControlItemOnPosition2(int x, int y, int lx, int ly, int *r, mouse_motion_button_status *s)
{
	if(!the_vkb.inited)
		return -1;
	int c = 0;
	int i;
	int count = TOTAL_VKB_COUNT;
	for(i = 0; i < count; i++)
	{
		const virtual_control_item *b = the_vkb.vb + i;
		if((b->base.show_mask & client_state) == 0)
			continue;
		if(!b->base.enabled)
			continue;
		mouse_motion_button_status status = karinButtonMouseMotion(b, x, y, lx, ly);
		if(status == all_out_range_status)
			continue;
		if(r)
			r[c] = i;
		if(s)
			s[c] = status;
		c++;
	}
	if(r && s)
	{
		int j;
		for(j = 0; j < c; j++)
		{
			int k;
			for(k = j + 1; k < c; k++)
			{
				const virtual_control_item *a = the_vkb.vb + r[j];
				const virtual_control_item *b = the_vkb.vb + r[k];
				if(b->base.z > a->base.z)
				{
					int t = r[j];
					r[j] = r[k];
					r[k] = t;
					mouse_motion_button_status tt = s[j];
					s[j] = s[k];
					s[k] = tt;
				}
			}
		}
	}
	return c;
}

unsigned karinVKBMouseMotionEvent(int b, int p, int x, int y, int dx, int dy, VKB_Key_Action_Function f)
{
	if(!the_vkb.inited)
		return 0;
	if(!p)
		return 0;
	int midx = dx * 2;
	int midy = dy * 2;
	int nres = 0;
	int last_x = x - dx;
	int last_y = y - dy;
	int last_nz = INT_MIN;
	int clicked[TOTAL_VKB_COUNT];
	mouse_motion_button_status status[TOTAL_VKB_COUNT];
	int count = karinGetControlItemOnPosition2(x, y, last_x, last_y, clicked, status);
	if(count > 0)
	{
		int i;
		for(i = 0; i < count; i++)
		{
			virtual_control_item *b = the_vkb.vb + clicked[i];
			mouse_motion_button_status s = status[i];
			// normal button
			//printf("%d  %d %d\n", i, b->type, b->base.z);
			if(b->type == vkb_button_type)
			{
				/*
					 if(s == last_out_now_in_range_status)
					 {
					 if(nres && b->button.base.z < last_nz)
					 break;
					 last_nz = b->button.base.z;
					 b->base.pressed = btrue;
					 if(f)
					 f(b->button.action, b->button.base.pressed, 0, 0);
					 nres = 1;
					 }
					 else 
					 */
				if(s == last_in_now_out_range_status)
				{
					b->button.base.pressed = bfalse;
					if(f)
						f(b->button.action, bfalse, 0, 0);
				}
				else if(s == all_in_range_status)
				{
					if(nres && b->button.base.z < last_nz)
						break;
					last_nz = b->button.base.z;
					nres = 1;
				}
			}

			// joystick
			else if(b->type == vkb_joystick_type)
			{
				if(s == last_out_now_in_range_status)
				{
					if(nres && b->joystick.base.z < last_nz)
						break;
					last_nz = b->joystick.base.z;
					b->joystick.base.pressed = btrue;
					float cx = b->joystick.base.x + b->joystick.base.width / 2;
					float cy = b->joystick.base.y + b->joystick.base.height / 2;
					circle_direction d = karinGetJoystickDirection(x, y, cx, cy, b->joystick.e_ignore_radius, b->joystick.e_radius, &b->joystick.angle, &b->joystick.percent);
					if(d != circle_outside)
					{
						b->joystick.pos_x = x - cx;
						b->joystick.pos_y = y - cy;
					}
					else
					{
						b->joystick.pos_x = 0;
						b->joystick.pos_y = 0;
					}
					if(f)
					{
						switch(d)
						{
							case circle_top_direction:
								f(b->joystick.actions[0], btrue, midx, midy);
								break;
							case circle_bottom_direction:
								f(b->joystick.actions[1], btrue, midx, midy);
								break;
							case circle_right_direction:
								f(b->joystick.actions[3], btrue, midx, midy);
								break;
							case circle_left_direction:
								f(b->joystick.actions[2], btrue, midx, midy);
								break;
							case circle_lefttop_direction:
								f(b->joystick.actions[0], btrue, midx, midy);
								f(b->joystick.actions[2], btrue, midx, midy);
								break;
							case circle_leftbottom_direction:
								f(b->joystick.actions[1], btrue, midx, midy);
								f(b->joystick.actions[2], btrue, midx, midy);
								break;
							case circle_righttop_direction:
								f(b->joystick.actions[0], btrue, midx, midy);
								f(b->joystick.actions[3], btrue, midx, midy);
								break;
							case circle_rightbottom_direction:
								f(b->joystick.actions[1], btrue, midx, midy);
								f(b->joystick.actions[3], btrue, midx, midy);
								break;
							default:
								break;
						}
					}
					nres = 1;
				}
				else if(s == last_in_now_out_range_status)
				{
					b->joystick.base.pressed = bfalse;
					if(f)
					{
						float cx = b->joystick.base.x + b->joystick.base.width / 2;
						float cy = b->joystick.base.y + b->joystick.base.height / 2;
						circle_direction d = karinGetJoystickDirection(last_x, last_y, cx, cy, b->joystick.e_ignore_radius, b->joystick.e_radius, NULL, NULL);
						switch(d)
						{
							case circle_top_direction:
								f(b->joystick.actions[0], bfalse, midx, midy);
								break;
							case circle_bottom_direction:
								f(b->joystick.actions[1], bfalse, midx, midy);
								break;
							case circle_right_direction:
								f(b->joystick.actions[3], bfalse, midx, midy);
								break;
							case circle_left_direction:
								f(b->joystick.actions[2], bfalse, midx, midy);
								break;
							case circle_lefttop_direction:
								f(b->joystick.actions[0], bfalse, midx, midy);
								f(b->joystick.actions[2], bfalse, midx, midy);
								break;
							case circle_leftbottom_direction:
								f(b->joystick.actions[1], bfalse, midx, midy);
								f(b->joystick.actions[2], bfalse, midx, midy);
								break;
							case circle_righttop_direction:
								f(b->joystick.actions[0], bfalse, midx, midy);
								f(b->joystick.actions[3], bfalse, midx, midy);
								break;
							case circle_rightbottom_direction:
								f(b->joystick.actions[1], bfalse, midx, midy);
								f(b->joystick.actions[3], bfalse, midx, midy);
								break;
							default:
								break;
						}
					}
					b->joystick.angle = -1.0f;
					b->joystick.percent = 0.0f;
					b->joystick.pos_x = 0.0f;
					b->joystick.pos_y = 0.0f;
				}
				else if(s == all_in_range_status)
				{
					if(nres && b->joystick.base.z < last_nz)
					{
						b->joystick.base.pressed = bfalse;
						if(f)
						{
							float cx = b->joystick.base.x + b->joystick.base.width / 2;
							float cy = b->joystick.base.y + b->joystick.base.height / 2;
							circle_direction d = karinGetJoystickDirection(last_x, last_y, cx, cy, b->joystick.e_ignore_radius, b->joystick.e_radius, NULL, NULL);
							switch(d)
							{
								case circle_top_direction:
									f(b->joystick.actions[0], bfalse, midx, midy);
									break;
								case circle_bottom_direction:
									f(b->joystick.actions[1], bfalse, midx, midy);
									break;
								case circle_right_direction:
									f(b->joystick.actions[3], bfalse, midx, midy);
									break;
								case circle_left_direction:
									f(b->joystick.actions[2], bfalse, midx, midy);
									break;
								case circle_lefttop_direction:
									f(b->joystick.actions[0], bfalse, midx, midy);
									f(b->joystick.actions[2], bfalse, midx, midy);
									break;
								case circle_leftbottom_direction:
									f(b->joystick.actions[1], bfalse, midx, midy);
									f(b->joystick.actions[2], bfalse, midx, midy);
									break;
								case circle_righttop_direction:
									f(b->joystick.actions[0], bfalse, midx, midy);
									f(b->joystick.actions[3], bfalse, midx, midy);
									break;
								case circle_rightbottom_direction:
									f(b->joystick.actions[1], bfalse, midx, midy);
									f(b->joystick.actions[3], bfalse, midx, midy);
									break;
								default:
									break;
							}
						}
						b->joystick.angle = -1.0f;
						b->joystick.percent = 0.0f;
						b->joystick.pos_x = 0.0f;
						b->joystick.pos_y = 0.0f;
					}
					else
					{
						last_nz = b->joystick.base.z;
						float cx = b->joystick.base.x + b->joystick.base.width / 2;
						float cy = b->joystick.base.y + b->joystick.base.height / 2;
						circle_direction ld = karinGetJoystickDirection(last_x, last_y, cx, cy, b->joystick.e_ignore_radius, b->joystick.e_radius, NULL, NULL);
						circle_direction d = karinGetJoystickDirection(x, y, cx, cy, b->joystick.e_ignore_radius, b->joystick.e_radius, &b->joystick.angle, &b->joystick.percent);
						if(d != ld)
						{
							if(f)
							{
								int mask[4] = {0, 0, 0, 0};
								switch(ld)
								{
									case circle_top_direction:
										mask[0] -= 1;
										break;
									case circle_bottom_direction:
										mask[1] -= 1;
										break;
									case circle_right_direction:
										mask[3] -= 1;
										break;
									case circle_left_direction:
										mask[2] -= 1;
										break;
									case circle_lefttop_direction:
										mask[0] -= 1;
										mask[2] -= 1;
										break;
									case circle_leftbottom_direction:
										mask[1] -= 1;
										mask[2] -= 1;
										break;
									case circle_righttop_direction:
										mask[0] -= 1;
										mask[3] -= 1;
										break;
									case circle_rightbottom_direction:
										mask[1] -= 1;
										mask[3] -= 1;
										break;
									default:
										break;
								}
								switch(d)
								{
									case circle_top_direction:
										mask[0] += 1;
										break;
									case circle_bottom_direction:
										mask[1] += 1;
										break;
									case circle_right_direction:
										mask[3] += 1;
										break;
									case circle_left_direction:
										mask[2] += 1;
										break;
									case circle_lefttop_direction:
										mask[0] += 1;
										mask[2] += 1;
										break;
									case circle_leftbottom_direction:
										mask[1] += 1;
										mask[2] += 1;
										break;
									case circle_righttop_direction:
										mask[0] += 1;
										mask[3] += 1;
										break;
									case circle_rightbottom_direction:
										mask[1] += 1;
										mask[3] += 1;
										break;
									default:
										break;
								}
								if(mask[0] != 0)
								{
									f(b->joystick.actions[0], mask[0] > 0 ? btrue : bfalse, midx, midy);
								}
								if(mask[1] != 0)
								{
									f(b->joystick.actions[1], mask[1] > 0 ? btrue : bfalse, midx, midy);
								}
								if(mask[2] != 0)
								{
									f(b->joystick.actions[2], mask[2] > 0 ? btrue : bfalse, midx, midy);
								}
								if(mask[3] != 0)
								{
									f(b->joystick.actions[3], mask[3] > 0 ? btrue : bfalse, midx, midy);
								}
							}
						}
						if(d != circle_outside)
						{
							b->joystick.pos_x = x - cx;
							b->joystick.pos_y = y - cy;
						}
						else
						{
							b->joystick.pos_x = 0;
							b->joystick.pos_y = 0;
						}
						nres = 1;
					}
				}
			}

			// swipe
			else if(b->type == vkb_swipe_type)
			{
				if(s == last_out_now_in_range_status)
				{
					if(nres && b->swipe.base.z < last_nz)
						break;
					last_nz = b->swipe.base.z; // ??? down
					/*
					if(!b->swipe.base.pressed) // ???
					{
						if(f)
							f(b->swipe.action, btrue, 0, 0);
					}
					*/
					b->swipe.base.pressed = btrue;
					b->swipe.distance = 0.0f;
					b->swipe.angle = -1.0f;
					nres = 1;
				}
				else if(s == last_in_now_out_range_status)
				{
					/*
					if(b->swipe.base.pressed)
					{
						if(f)
							f(b->swipe.action, bfalse, 0, 0);
					}
					*/
					b->swipe.base.pressed = bfalse;
					if(b->swipe.mask[0] > 0)
					{
						if(f)
							f(b->swipe.actions[0], bfalse, midx, midy);
						b->swipe.mask[0] = 0;
					}
					if(b->swipe.mask[1] > 0)
					{
						if(f)
							f(b->swipe.actions[1], bfalse, midx, midy);
						b->swipe.mask[1] = 0;
					}
					if(b->swipe.mask[2] > 0)
					{
						if(f)
							f(b->swipe.actions[2], bfalse, midx, midy);
						b->swipe.mask[2] = 0;
					}
					if(b->swipe.mask[3] > 0)
					{
						if(f)
							f(b->swipe.actions[3], bfalse, midx, midy);
						b->swipe.mask[3] = 0;
					}
					b->swipe.distance = 0.0f;
					b->swipe.angle = -1.0f;
				}
				else if(s == all_in_range_status)
				{
					if(nres && b->swipe.base.z < last_nz)
					{
						b->swipe.base.pressed = bfalse;
						if(b->swipe.mask[0] > 0)
						{
							if(f)
								f(b->swipe.actions[0], bfalse, midx, midy);
							b->swipe.mask[0] = 0;
						}
						if(b->swipe.mask[1] > 0)
						{
							if(f)
								f(b->swipe.actions[1], bfalse, midx, midy);
							b->swipe.mask[1] = 0;
						}
						if(b->swipe.mask[2] > 0)
						{
							if(f)
								f(b->swipe.actions[2], bfalse, midx, midy);
							b->swipe.mask[2] = 0;
						}
						if(b->swipe.mask[3] > 0)
						{
							if(f)
								f(b->swipe.actions[3], bfalse, midx, midy);
							b->swipe.mask[3] = 0;
						}
						b->swipe.distance = 0.0f;
						b->swipe.angle = -1.0f;
					}
					else
					{
						last_nz = b->swipe.base.z;
						b->swipe.base.pressed = btrue;
						circle_direction d = karinGetSwipeDirection(dx, dy, &b->swipe.angle, &b->swipe.distance);
						if(d != circle_center)
						{
							if(b->swipe.distance < b->swipe.ignore_distance)
								d = circle_center;
						}
						if(f)
						{
							uint mask[4] = {0, 0, 0, 0};
							switch(d)
							{
								case circle_top_direction:
									mask[0] += 1;
									break;
								case circle_bottom_direction:
									mask[1] += 1;
									break;
								case circle_right_direction:
									mask[3] += 1;
									break;
								case circle_left_direction:
									mask[2] += 1;
									break;
								case circle_lefttop_direction:
									mask[0] += 1;
									mask[2] += 1;
									break;
								case circle_leftbottom_direction:
									mask[1] += 1;
									mask[2] += 1;
									break;
								case circle_righttop_direction:
									mask[0] += 1;
									mask[3] += 1;
									break;
								case circle_rightbottom_direction:
									mask[1] += 1;
									mask[3] += 1;
									break;
								default:
									break;
							}
							if(b->swipe.mask[0] != 0 || mask[0] != 0)
							{
								f(b->swipe.actions[0], mask[0] >= b->swipe.mask[0] ? btrue : bfalse, midx, midy);
								b->swipe.mask[0] = mask[0];
								nres = 1;
							}
							if(b->swipe.mask[1] != 0 || mask[1] != 0)
							{
								f(b->swipe.actions[1], mask[1] >= b->swipe.mask[1] ? btrue : bfalse, midx, midy);
								b->swipe.mask[1] = mask[1];
								nres = 1;
							}
							if(b->swipe.mask[2] != 0 || mask[2] != 0)
							{
								f(b->swipe.actions[2], mask[2] >= b->swipe.mask[2] ? btrue : bfalse, midx, midy);
								b->swipe.mask[2] = mask[2];
								nres = 1;
							}
							if(b->swipe.mask[3] != 0 || mask[3] != 0)
							{
								f(b->swipe.actions[3], mask[3] >= b->swipe.mask[3] ? btrue : bfalse, midx, midy);
								b->swipe.mask[3] = mask[3];
								nres = 1;
							}
						}
					}
				}
			}

			// cursor
			else if(b->type == vkb_cursor_type)
			{
				if(s == last_in_now_out_range_status)
				{
					b->cursor.base.pressed = bfalse;
					if(f)
						f(b->cursor.action, bfalse, midx, midy);
					if(b->cursor.mask[0] > 0)
					{
						if(f)
							f(b->cursor.actions[0], bfalse, midx, midy);
						b->cursor.mask[0] = 0;
					}
					if(b->cursor.mask[1] > 0)
					{
						if(f)
							f(b->cursor.actions[1], bfalse, midx, midy);
						b->cursor.mask[1] = 0;
					}
					if(b->cursor.mask[2] > 0)
					{
						if(f)
							f(b->cursor.actions[2], bfalse, midx, midy);
						b->cursor.mask[2] = 0;
					}
					if(b->cursor.mask[3] > 0)
					{
						if(f)
							f(b->cursor.actions[3], bfalse, midx, midy);
						b->cursor.mask[3] = 0;
					}
					b->cursor.distance = 0.0f;
					b->cursor.angle = -1.0f;
					b->cursor.pos_x = 0;
					b->cursor.pos_y = 0;
				}
				else if(s == all_in_range_status)
				{
					if(nres && b->cursor.base.z < last_nz)
					{
						if(b->cursor.mask[0] > 0)
						{
							if(f)
								f(b->cursor.actions[0], bfalse, midx, midy);
							b->cursor.mask[0] = 0;
						}
						if(b->cursor.mask[1] > 0)
						{
							if(f)
								f(b->cursor.actions[1], bfalse, midx, midy);
							b->cursor.mask[1] = 0;
						}
						if(b->cursor.mask[2] > 0)
						{
							if(f)
								f(b->cursor.actions[2], bfalse, midx, midy);
							b->cursor.mask[2] = 0;
						}
						if(b->cursor.mask[3] > 0)
						{
							if(f)
								f(b->cursor.actions[3], bfalse, midx, midy);
							b->cursor.mask[3] = 0;
						}
						b->cursor.distance = 0.0f;
						b->cursor.angle = -1.0f;
						b->cursor.pos_x = 0;
						b->cursor.pos_y = 0;
						b->cursor.base.pressed = bfalse;
						if(f)
							f(b->cursor.action, bfalse, midx, midy);
					}
					else
					{
						if(b->cursor.base.pressed)
						{
							last_nz = b->cursor.base.z;
							circle_direction d = karinGetSwipeDirection(dx, dy, &b->cursor.angle, &b->cursor.distance);
							if(d != circle_center)
							{
								if(b->cursor.distance < b->cursor.ignore_distance)
									d = circle_center;
							}
							if(f)
							{
								uint mask[4] = {0, 0, 0, 0};
								switch(d)
								{
									case circle_top_direction:
										mask[0] += 1;
										break;
									case circle_bottom_direction:
										mask[1] += 1;
										break;
									case circle_right_direction:
										mask[3] += 1;
										break;
									case circle_left_direction:
										mask[2] += 1;
										break;
									case circle_lefttop_direction:
										mask[0] += 1;
										mask[2] += 1;
										break;
									case circle_leftbottom_direction:
										mask[1] += 1;
										mask[2] += 1;
										break;
									case circle_righttop_direction:
										mask[0] += 1;
										mask[3] += 1;
										break;
									case circle_rightbottom_direction:
										mask[1] += 1;
										mask[3] += 1;
										break;
									default:
										break;
								}
								// glBegin(change to !=)
								if(b->cursor.mask[0] != mask[0])
								{
									if(f)
										f(b->cursor.actions[0], mask[0] > b->cursor.mask[0] ? btrue : bfalse, midx, midy);
									b->cursor.mask[0] = mask[0];
									nres = 1;
								}
								if(b->cursor.mask[1] != mask[1])
								{
									if(f)
										f(b->cursor.actions[1], mask[1] > b->cursor.mask[1] ? btrue : bfalse, midx, midy);
									b->cursor.mask[1] = mask[1];
									nres = 1;
								}
								if(b->cursor.mask[2] != mask[2])
								{
									if(f)
										f(b->cursor.actions[2], mask[2] > b->cursor.mask[2] ? btrue : bfalse, midx, midy);
									b->cursor.mask[2] = mask[2];
									nres = 1;
								}
								if(b->cursor.mask[3] != mask[3])
								{
									if(f)
										f(b->cursor.actions[3], mask[3] > b->cursor.mask[3] ? btrue : bfalse, midx, midy);
									b->cursor.mask[3] = mask[3];
									nres = 1;
								}
								// glEnd(change to !=)
							}
							float cx = b->cursor.base.x + b->cursor.base.width / 2;
							float cy = b->cursor.base.y + b->cursor.base.height / 2;
							b->cursor.pos_x = x - cx;
							b->cursor.pos_y = y - cy;
						}
					}
				}
			}
		}
		if(nres)
			return 1;
	}
	return 0;
}

unsigned karinVKBMouseEvent(int b, int p, int x, int y, VKB_Key_Action_Function f)
{
	if(!the_vkb.inited)
		return 0;
	int last_z = INT_MIN;
	int res = 0;
	int clicked[TOTAL_VKB_COUNT];
	int count = karinGetControlItemOnPosition(x, y, clicked);
	if(count > 0)
	{
		int i;
		for(i = 0; i < count; i++)
		{
			virtual_control_item *b = the_vkb.vb + clicked[i];
			if(b->type == vkb_joystick_type)
			{
				boolean pr = bfalse;
				if(res && b->base.z < last_z)
				{
					pr = bfalse;
				}
				else
				{
					pr = p ? btrue : bfalse;
					last_z = b->base.z;
					res = 1;
				}
				b->joystick.base.pressed = pr;
				float cx = b->joystick.base.x + b->joystick.base.width / 2;
				float cy = b->joystick.base.y + b->joystick.base.width / 2;
				circle_direction d = karinGetJoystickDirection(x, y, cx, cy, b->joystick.e_ignore_radius, b->joystick.e_radius, &b->joystick.angle, &b->joystick.percent);
				if(f)
				{
					switch(d)
					{
						case circle_top_direction:
							f(b->joystick.actions[0], b->joystick.base.pressed, 0, 0);
							break;
						case circle_bottom_direction:
							f(b->joystick.actions[1], b->joystick.base.pressed, 0, 0);
							break;
						case circle_right_direction:
							f(b->joystick.actions[3], b->joystick.base.pressed, 0, 0);
							break;
						case circle_left_direction:
							f(b->joystick.actions[2], b->joystick.base.pressed, 0, 0);
							break;
						case circle_lefttop_direction:
							f(b->joystick.actions[0], b->joystick.base.pressed, 0, 0);
							f(b->joystick.actions[2], b->joystick.base.pressed, 0, 0);
							break;
						case circle_leftbottom_direction:
							f(b->joystick.actions[1], b->joystick.base.pressed, 0, 0);
							f(b->joystick.actions[2], b->joystick.base.pressed, 0, 0);
							break;
						case circle_righttop_direction:
							f(b->joystick.actions[0], b->joystick.base.pressed, 0, 0);
							f(b->joystick.actions[3], b->joystick.base.pressed, 0, 0);
							break;
						case circle_rightbottom_direction:
							f(b->joystick.actions[1], b->joystick.base.pressed, 0, 0);
							f(b->joystick.actions[3], b->joystick.base.pressed, 0, 0);
							break;
						default:
							break;
					}
				}
				if(!pr)
				{
					b->joystick.angle = -1.0f;
					b->joystick.percent = 0.0f;
					b->joystick.pos_x = 0.0f;
					b->joystick.pos_y = 0.0f;
				}
				else
				{
					if(d != circle_outside)
					{
						b->joystick.pos_x = x - cx;
						b->joystick.pos_y = y - cy;
					}
					else
					{
						b->joystick.pos_x = 0;
						b->joystick.pos_y = 0;
					}
				}
			}
			else if(b->type == vkb_button_type)
			{
				boolean pr = bfalse;
				if(res && b->base.z < last_z)
				{
					pr = bfalse;
				}
				else
				{
					pr = p ? btrue : bfalse;
					last_z = b->base.z;
					res = 1;
				}
				b->button.base.pressed = pr;
				if(f)
					f(b->button.action, b->button.base.pressed, 0, 0);
			}
			else if(b->type == vkb_swipe_type)
			{
				boolean pr = bfalse;
				if(res && b->base.z < last_z)
				{
					pr = bfalse;
				}
				else
				{
					pr = p ? btrue : bfalse;
				}
				if(!pr)
				{
					res = 1;
					/*
					if(b->swipe.base.pressed)
					{
						if(f)
							f(b->swipe.action, bfalse, 0, 0);
					}
					*/
					b->swipe.base.pressed = pr;
					if(b->swipe.mask[0] > 0)
					{
						if(f)
							f(b->swipe.actions[0], bfalse, 0, 0);
						b->swipe.mask[0] = 0;
					}
					if(b->swipe.mask[1] > 0)
					{
						if(f)
							f(b->swipe.actions[1], bfalse, 0, 0);
						b->swipe.mask[1] = 0;
					}
					if(b->swipe.mask[2] > 0)
					{
						if(f)
							f(b->swipe.actions[2], bfalse, 0, 0);
						b->swipe.mask[2] = 0;
					}
					if(b->swipe.mask[3] > 0)
					{
						if(f)
							f(b->swipe.actions[3], bfalse, 0, 0);
						b->swipe.mask[3] = 0;
					}
					b->swipe.distance = 0.0f;
					b->swipe.angle = -1.0f;
				}
				else
				{
					/*
					if(!b->swipe.base.pressed)
					{
						if(f)
							f(b->swipe.action, btrue, 0, 0);
					}
					*/
					b->swipe.base.pressed = pr;
					//res = 1;
				}
			}
			else if(b->type == vkb_cursor_type)
			{
				boolean pr = bfalse;
				if(res && b->base.z < last_z)
				{
					pr = bfalse;
				}
				else
				{
					pr = p ? btrue : bfalse;
				}
				float cx = b->cursor.base.x + b->cursor.base.width / 2;
				float cy = b->cursor.base.y + b->cursor.base.height / 2;
				circle_direction d = karinGetJoystickDirection(x, y, cx, cy, 0, b->cursor.e_radius, NULL, NULL);
				if(!pr)
				{
					if(b->cursor.mask[0] > 0)
					{
						if(f)
							f(b->cursor.actions[0], bfalse, 0, 0);
						b->cursor.mask[0] = 0;
					}
					if(b->cursor.mask[1] > 0)
					{
						if(f)
							f(b->cursor.actions[1], bfalse, 0, 0);
						b->cursor.mask[1] = 0;
					}
					if(b->cursor.mask[2] > 0)
					{
						if(f)
							f(b->cursor.actions[2], bfalse, 0, 0);
						b->cursor.mask[2] = 0;
					}
					if(b->cursor.mask[3] > 0)
					{
						if(f)
							f(b->cursor.actions[3], bfalse, 0, 0);
						b->cursor.mask[3] = 0;
					}
					b->cursor.distance = 0.0f;
					b->cursor.angle = -1.0f;
					b->cursor.pos_x = 0.0f;
					b->cursor.pos_y = 0.0f;
					b->cursor.base.pressed = bfalse;
					if(f)
						f(b->cursor.action, bfalse, 0, 0);
				}
				else
				{
					if(d != circle_outside)
					{
						b->cursor.base.pressed = btrue;
						last_z = b->base.z;
						res = 1;
						if(f)
							f(b->cursor.action, btrue, 0, 0);
						b->cursor.distance = 0.0f;
						b->cursor.angle = -1.0f;
						b->cursor.pos_x = x - cx;
						b->cursor.pos_y = y - cy;
					}
				}
			}
		}
	}
	return res;
}

// OpenGL ES has not glPushAttrib/glPopAttrib
// Cull Face
static struct OpenGL_CullFace_Attrib
{
	GLboolean cull_face;
	GLint front_face;
	GLint cull_face_mode;
} cull_face_attrib;
// Texture 2D
static struct OpenGL_Texture2D_Attrib
{
	GLint active_texture;
	GLboolean texture2d;
	GLint bind_texture2d;
} texture2d_attrib;
// Tex Env
static struct OpenGL_TexEnv_Attrib
{
	GLint tex_env_mode;
} tex_env_attrib;
// Alpha Test
static struct OpenGL_AlphaTest_Attrib
{
	GLfloat alpha_ref;
	GLint alpha_func;
	GLboolean alpha_test;
} alpha_test_attrib;
// Blend
static struct OpenGL_Blend_Attrib
{
	GLint blend_src;
	GLint blend_dst;
	GLboolean blend;
} blend_attrib;
// Depth Test
static struct OpenGL_DepthTest_Attrib
{
	GLboolean depth_test;
} depth_test_attrib;
// Transform(Matrix and Viewport)
static struct OpenGL_Transform_Attrib
{
	GLint matrix_mode;
	GLint viewport[4];
} transform_attrib;
// Current
static struct OpenGL_Current_Attrib
{
	GLfloat current_color[4];
} current_attrib;
// Stencil Test
static struct OpenGL_StencilTest_Attrib
{
	GLboolean stencil_test;
} stencil_test_attrib;
// Client State
static struct OpenGL_ClientState_Attrib
{
	GLboolean vertex_array;
	GLboolean texcoord_array;
	GLboolean color_array;
	GLint client_active_texture;
} client_state_attrib;
/*
// Scissor
static struct OpenGL_Scissor_Attrib
{
	GLint scissor_box[4];
	GLboolean scissor_test;
} scissor_attrib;
*/

static void karinBeginRender2D(void)
{
	if(render_lock)
		return;
	// Get
	{
		qglGetIntegerv(GL_TEXTURE_BINDING_2D, &texture2d_attrib.bind_texture2d);
		texture2d_attrib.texture2d = qglIsEnabled(GL_TEXTURE_2D);
		qglGetIntegerv(GL_ACTIVE_TEXTURE, &texture2d_attrib.active_texture);

		blend_attrib.blend = qglIsEnabled(GL_BLEND);
		qglGetIntegerv(GL_BLEND_SRC, &blend_attrib.blend_src);
		qglGetIntegerv(GL_BLEND_DST, &blend_attrib.blend_dst);

		alpha_test_attrib.alpha_test = qglIsEnabled(GL_ALPHA_TEST);
		qglGetFloatv(GL_ALPHA_TEST_REF, &alpha_test_attrib.alpha_ref);
		qglGetIntegerv(GL_ALPHA_TEST_FUNC, &alpha_test_attrib.alpha_func);

		depth_test_attrib.depth_test = qglIsEnabled(GL_DEPTH_TEST);

		qglGetIntegerv(GL_MATRIX_MODE, &transform_attrib.matrix_mode);
		qglGetIntegerv(GL_VIEWPORT, transform_attrib.viewport);

		client_state_attrib.vertex_array = qglIsEnabled(GL_VERTEX_ARRAY);
		client_state_attrib.texcoord_array = qglIsEnabled(GL_TEXTURE_COORD_ARRAY);
		client_state_attrib.color_array = qglIsEnabled(GL_COLOR_ARRAY);
		qglGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &client_state_attrib.client_active_texture);

		stencil_test_attrib.stencil_test = qglIsEnabled(GL_STENCIL_TEST);

		qglGetFloatv(GL_CURRENT_COLOR, current_attrib.current_color);

		cull_face_attrib.cull_face = qglIsEnabled(GL_CULL_FACE);
		qglGetIntegerv(GL_FRONT_FACE, &cull_face_attrib.front_face);
		qglGetIntegerv(GL_CULL_FACE_MODE, &cull_face_attrib.cull_face_mode);

		qglGetTexEnviv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, &tex_env_attrib.tex_env_mode);

		//qglGetIntegerv(GL_SCISSOR_BOX, scissor_attrib.scissor_box);
		//scissor_attrib.scissor_test = qglIsEnabled(GL_SCISSOR_TEST);
	}
	// Set
	{
		qglEnable(GL_ALPHA_TEST);
		qglAlphaFunc(GL_GREATER, 0.1);

		qglEnable(GL_BLEND);
		qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		qglEnable(GL_TEXTURE_2D);
		qglActiveTexture(GL_TEXTURE0);

		qglDisable(GL_DEPTH_TEST);

		qglDisable(GL_STENCIL_TEST);

		qglColor4f(1.0, 1.0, 1.0, 1.0);

		qglDisableClientState(GL_COLOR_ARRAY);
		qglClientActiveTexture(GL_TEXTURE0);

		qglEnable(GL_CULL_FACE);
		qglFrontFace(GL_CCW);
		qglCullFace(GL_BACK);

		//qglDisable(GL_SCISSOR_TEST);

		qglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	}
	// Matrix
	{
		qglViewport(0, 0, the_vkb.height, the_vkb.width);
		qglMatrixMode(GL_PROJECTION);
		qglPushMatrix();
		qglLoadIdentity();
		qglOrthof(0.0, the_vkb.height, 0.0, the_vkb.width, -1.0, 1.0);
#ifdef _HARMATTAN_SAILFISH
		qglTranslatef(0, the_vkb.width, 0);
    qglRotatef (-90, 0, 0, 1);	    // for sailfish
#endif
		qglMatrixMode(GL_MODELVIEW);
		qglPushMatrix();
		qglLoadIdentity();
	}
	render_lock = btrue;
}

static void karinEndRender2D(void)
{
	if(!render_lock)
		return;
	{
		qglMatrixMode(GL_MODELVIEW);
		qglPopMatrix();
		qglMatrixMode(GL_PROJECTION);
		qglPopMatrix();
	}
	{
		qglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, tex_env_attrib.tex_env_mode);

		qglBindTexture(GL_TEXTURE_2D, texture2d_attrib.bind_texture2d);
		if(!texture2d_attrib.texture2d)
			qglDisable(GL_TEXTURE_2D);
		qglActiveTexture(texture2d_attrib.active_texture);

		qglAlphaFunc(alpha_test_attrib.alpha_func, alpha_test_attrib.alpha_ref);
		if(!alpha_test_attrib.alpha_test)
			qglDisable(GL_ALPHA_TEST);

		if(!blend_attrib.blend)
			qglDisable(GL_BLEND);
		qglBlendFunc(blend_attrib.blend_src, blend_attrib.blend_dst);

		if(depth_test_attrib.depth_test)
			qglEnable(GL_DEPTH_TEST);

		qglMatrixMode(transform_attrib.matrix_mode);
		qglViewport(transform_attrib.viewport[0], transform_attrib.viewport[1], transform_attrib.viewport[2], transform_attrib.viewport[3]);

		if(stencil_test_attrib.stencil_test)
			qglEnable(GL_STENCIL_TEST);

		qglClientActiveTexture(client_state_attrib.client_active_texture);
		if(client_state_attrib.vertex_array)
			qglEnableClientState(GL_VERTEX_ARRAY);
		if(client_state_attrib.texcoord_array)
			qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
		if(client_state_attrib.color_array)
			qglEnableClientState(GL_COLOR_ARRAY);

		qglColor4f(current_attrib.current_color[0], current_attrib.current_color[1], current_attrib.current_color[2], current_attrib.current_color[3]);

		if(!cull_face_attrib.cull_face)
			qglDisable(GL_CULL_FACE);
		qglFrontFace(cull_face_attrib.front_face);
		qglCullFace(cull_face_attrib.cull_face_mode);

		//if(scissor_attrib.scissor_test)
			//qglEnable(GL_SCISSOR_TEST);
	}
	render_lock = bfalse;
}

static texture karinNewTexture2D(const char *file)
{
	texture tex;
	memset(&tex, 0, sizeof(texture));
	if(!file)
		return tex;
	int width = 0;
	int height = 0;
	unsigned char *data = NULL;
	karinLoadPNG(file, &data, &width, &height);
	//printf("%s %p %d %d\n", file, data, width, height);
	if(!data)
		return tex;
	GLint active_texture;
	qglGetIntegerv(GL_ACTIVE_TEXTURE, &active_texture);
	qglActiveTexture(GL_TEXTURE0);

	qglGenTextures(1, &tex.imaged);
	qglBindTexture(GL_TEXTURE_2D, tex.imaged);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	qglBindTexture(GL_TEXTURE_2D, 0);
	free(data);

	tex.width = width;
	tex.height = height;
	tex.format = GL_RGBA;

	qglActiveTexture(active_texture);
	return tex;
}

static GLuint karinNewBuffer(uenum buffer, sizei size, const void *data, uenum solution)
{
	GLuint bufid;
	qglGenBuffers(1, &bufid);
	qglBindBuffer(GL_ARRAY_BUFFER, bufid);
	qglBufferData(buffer, size, data, solution);
	qglBindBuffer(GL_ARRAY_BUFFER, 0);
	return bufid;
}

void karinNewVKB(float x, float y, float z, float w, float h)
{
	if(the_vkb.inited)
		return;
	the_vkb.x = x;
	the_vkb.y = y;
	the_vkb.z = z;
	the_vkb.width = w;
	the_vkb.height = h;
	karinUpdateP(w, h);
	int k;
	for(k = 0; k < VKB_TEX_COUNT; k++)
		the_vkb.tex[k] = karinNewTexture2D(Tex_Files[k]);

	int i;
	int j = 0;

	// btn
	for(i = 0; i < VKB_COUNT; i++)
	{
		karinMakeButton(the_vkb.vb + j, VKB_Button + i, the_vkb.width, the_vkb.height);
		j++;
	}

	// joy
	for(i = 0; i < JOYSTICK_COUNT; i++)
	{
		karinMakeJoystick(the_vkb.vb + j, VKB_Joystick + i, the_vkb.width, the_vkb.height);
		j++;
	}

	// cursor
	for(i = 0; i < CURSOR_COUNT; i++)
	{
		karinMakeCursor(the_vkb.vb + j, VKB_Cursor + i, the_vkb.width, the_vkb.height);
		j++;
	}

	// swipe
	for(i = 0; i < SWIPE_COUNT; i++)
	{
		karinMakeSwipe(the_vkb.vb + j, VKB_Swipe + i, the_vkb.width, the_vkb.height);
		j++;
	}

	the_vkb.inited = btrue;
}

void karinDeleteVKB(void)
{
	if(!the_vkb.inited)
		return;
	int count = TOTAL_VKB_COUNT;
	int i;
	for(i = 0; i < count; i++)
	{
		int j;
		for(j = Position_Coord; j < Total_Coord; j++)
		{
			if(qglIsBuffer(the_vkb.vb[i].base.buffers[j]))
				qglDeleteBuffers(1, the_vkb.vb[i].base.buffers + j);
			the_vkb.vb[i].base.buffers[j] = 0;
		}
	}
	for(i = 0; i < VKB_TEX_COUNT; i++)
	{
		if(qglIsTexture(the_vkb.tex[i].imaged))
			qglDeleteTextures(1, &the_vkb.tex[i].imaged);
		memset(the_vkb.tex + i, 0, sizeof(texture));
	}
	the_vkb.inited = bfalse;
	render_lock = bfalse;
}

void karinRenderVKB(void)
{
	if(!the_vkb.inited)
	{
		return;
	}
	karinBeginRender2D();
	{
		qglEnableClientState(GL_VERTEX_ARRAY);
		qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
		int i;
		int count = TOTAL_VKB_COUNT;
		for(i = 0; i < count; i++)
		{
			const virtual_control_item *b = the_vkb.vb + i;
			if((b->base.show_mask & client_state) == 0)
				continue;
			switch(b->type)
			{
				case vkb_button_type:
					karinRenderVKBButton(&b->button, the_vkb.tex);
					break;
				case vkb_joystick_type:
					karinRenderVKBJoystick(&b->joystick, the_vkb.tex);
					break;
				case vkb_swipe_type:
					karinRenderVKBSwipe(&b->swipe, the_vkb.tex);
					break;
				case vkb_cursor_type:
					karinRenderVKBCursor(&b->cursor, the_vkb.tex);
					break;
				default:
					continue;
			}
			//#define RENDER_CI_RANGE
#if 0
			qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
			float vv[] = {
				(b->cursor.base.x), (b->cursor.base.y),
				(b->cursor.base.x + b->cursor.base.width), (b->cursor.base.y),
				(b->cursor.base.x + b->cursor.base.width), (b->cursor.base.y + b->cursor.base.height),
				(b->cursor.base.x), (b->cursor.base.y + b->cursor.base.height),

				(b->cursor.base.e_min_x), (b->cursor.base.e_min_y),
				(b->cursor.base.e_max_x), (b->cursor.base.e_min_y),
				(b->cursor.base.e_max_x), (b->cursor.base.e_max_y),
				(b->cursor.base.e_min_x), (b->cursor.base.e_max_y),
			};
			qglBindTexture(GL_TEXTURE_2D, 0);
			qglVertexPointer(2, GL_FLOAT, 0, vv);
			qglColor4ub(255,0,0,128);
			qglDrawArrays(GL_TRIANGLE_FAN, 0, 4);
			qglColor4ub(0,255,0,64);
			qglDrawArrays(GL_TRIANGLE_FAN, 4, 4);
			qglColor4ub(255,255,255,255);
			qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
#endif
		}
		qglBindTexture(GL_TEXTURE_2D, 0);
		qglDisableClientState(GL_VERTEX_ARRAY);
		qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}
	karinEndRender2D();
}

#define GET_JOY_XY(r, v, s, w, b) \
	switch(b) \
{ \
	case opengl_mf_base : \
	case opengl_s_base : \
											 r = v + (1.0 - s) * w / 2.0; \
	break; \
	case opengl_mb_base : \
	case opengl_e_base : \
											 r = v - (1.0 - s) * w / 2.0; \
	break; \
	default : \
						r = v; \
	break; \
}

#define GET_VB_XY(r, t, v, b) \
	switch(b) \
{ \
	case opengl_mf_base : \
												r = t / 2 + v; \
	break; \
	case opengl_mb_base : \
												r = t / 2 - v; \
	break; \
	case opengl_e_base : \
											 r = t - v; \
	break; \
	case opengl_s_base : \
	default : \
						r = v; \
	break; \
}

#define GET_TEX_S(t, v, w) (float)(v + w) / (float)t
#define GET_TEX_T(t, v, h) (float)(v - h) / (float)t

void karinMakeCursor(virtual_control_item *b, struct vkb_cursor *d, unsigned int width, unsigned int height)
{
	if(!b || !d)
		return;
	b->cursor.base.type = vkb_cursor_type;
	b->cursor.base.pressed = bfalse;
	b->cursor.base.enabled = d->ava;
	b->cursor.base.tex_index = d->tex_index;
	b->cursor.base.z = d->z;
	b->cursor.ignore_distance = d->ignore_distance;
	b->cursor.angle = -1.0f;
	b->cursor.distance = 0.0f;
	b->cursor.render_bg = d->render;
	b->cursor.pos_x = 0.0f;
	b->cursor.pos_y = 0.0f;
	b->cursor.base.visible = d->ava;
	b->cursor.actions[0] = d->action0;
	b->cursor.actions[1] = d->action1;
	b->cursor.actions[2] = d->action2;
	b->cursor.actions[3] = d->action3;
	b->cursor.action = d->action;
	b->cursor.base.show_mask = d->mask;
	if(!b->cursor.base.enabled)
	{
		b->cursor.actions[0] = Total_Action;
		b->cursor.actions[1] = Total_Action;
		b->cursor.actions[2] = Total_Action;
		b->cursor.actions[3] = Total_Action;
		b->cursor.action = Total_Action;
		return;
	}

	GET_VB_XY(b->cursor.base.x, width, VB_P(d->x), d->x_base)
		GET_VB_XY(b->cursor.base.y, height, VB_P(d->y), d->y_base)
		b->cursor.base.width = VB_P(d->r);
	b->cursor.base.height = VB_P(d->r);
	b->cursor.e_radius = VB_P(d->r) * d->eminr;
	float ex;
	float ey;
	GET_JOY_XY(ex, VB_P(d->x), d->emaxr, VB_P(d->r), d->x_base)
		GET_JOY_XY(ey, VB_P(d->y), d->emaxr, VB_P(d->r), d->y_base)
		GET_VB_XY(b->cursor.base.e_min_x, width, ex, d->x_base)
		GET_VB_XY(b->cursor.base.e_min_y, height, ey, d->y_base)
		b->cursor.base.e_max_x = b->cursor.base.e_min_x + VB_P(d->r) * d->emaxr;
	b->cursor.base.e_max_y = b->cursor.base.e_min_y + VB_P(d->r) * d->emaxr;

	float jw = b->cursor.base.width * d->joy_r / 2.0;
	float jh = b->cursor.base.height * d->joy_r / 2.0;
	float vertex[] = {
		// joy
		-jw, -jh,
		jw, -jh,
		-jw, jh,
		jw, jh,
		// bg
		b->cursor.base.x, b->cursor.base.y,
		b->cursor.base.x + b->cursor.base.width, b->cursor.base.y,
		b->cursor.base.x, b->cursor.base.y + b->cursor.base.height,
		b->cursor.base.x + b->cursor.base.width, b->cursor.base.y + b->cursor.base.height
	};
	float texcoord[] = {
		// joy
		GET_TEX_S(TEX_FULL_WIDTH, d->joy_tx, 0), GET_TEX_T(TEX_FULL_HEIGHT, d->joy_ty, 0),
		GET_TEX_S(TEX_FULL_WIDTH, d->joy_tx, d->joy_tw), GET_TEX_T(TEX_FULL_HEIGHT, d->joy_ty, 0),
		GET_TEX_S(TEX_FULL_WIDTH, d->joy_tx, 0), GET_TEX_T(TEX_FULL_HEIGHT, d->joy_ty, d->joy_tw),
		GET_TEX_S(TEX_FULL_WIDTH, d->joy_tx, d->joy_tw), GET_TEX_T(TEX_FULL_HEIGHT, d->joy_ty, d->joy_tw),

		// joy pressed
		GET_TEX_S(TEX_FULL_WIDTH, d->joy_ptx, 0), GET_TEX_T(TEX_FULL_HEIGHT, d->joy_pty, 0),
		GET_TEX_S(TEX_FULL_WIDTH, d->joy_ptx, d->joy_ptw), GET_TEX_T(TEX_FULL_HEIGHT, d->joy_pty, 0),
		GET_TEX_S(TEX_FULL_WIDTH, d->joy_ptx, 0), GET_TEX_T(TEX_FULL_HEIGHT, d->joy_pty, d->joy_ptw),
		GET_TEX_S(TEX_FULL_WIDTH, d->joy_ptx, d->joy_ptw), GET_TEX_T(TEX_FULL_HEIGHT, d->joy_pty, d->joy_ptw),

		// bg
		GET_TEX_S(TEX_FULL_WIDTH, d->tx, 0), GET_TEX_T(TEX_FULL_HEIGHT, d->ty, 0),
		GET_TEX_S(TEX_FULL_WIDTH, d->tx, d->tw), GET_TEX_T(TEX_FULL_HEIGHT, d->ty, 0),
		GET_TEX_S(TEX_FULL_WIDTH, d->tx, 0), GET_TEX_T(TEX_FULL_HEIGHT, d->ty, d->tw),
		GET_TEX_S(TEX_FULL_WIDTH, d->tx, d->tw), GET_TEX_T(TEX_FULL_HEIGHT, d->ty, d->tw)
	};
	b->cursor.base.buffers[Position_Coord] = karinNewBuffer(GL_ARRAY_BUFFER, sizeof(float) * 16, vertex, GL_STATIC_DRAW);
	b->cursor.base.buffers[Texture_Coord] = karinNewBuffer(GL_ARRAY_BUFFER, sizeof(float) * 24, texcoord, GL_STATIC_DRAW);
}

void karinMakeJoystick(virtual_control_item *b, struct vkb_joystick *d, unsigned int width, unsigned int height)
{
	if(!b || !d)
		return;
	b->joystick.base.type = vkb_joystick_type;
	b->joystick.base.pressed = bfalse;
	b->joystick.base.enabled = d->ava;
	b->joystick.base.tex_index = 1;
	b->joystick.base.z = d->z;
	b->joystick.angle = -1.0f;
	b->joystick.percent = 0.0f;
	b->joystick.pos_x = 0.0f;
	b->joystick.pos_y = 0.0f;
	b->joystick.base.visible = d->ava;
	b->joystick.actions[0] = d->action0;
	b->joystick.actions[1] = d->action1;
	b->joystick.actions[2] = d->action2;
	b->joystick.actions[3] = d->action3;
	b->joystick.base.show_mask = d->mask;
	if(!b->joystick.base.enabled)
	{
		b->joystick.actions[0] = Total_Action;
		b->joystick.actions[1] = Total_Action;
		b->joystick.actions[2] = Total_Action;
		b->joystick.actions[3] = Total_Action;
		return;
	}

	GET_VB_XY(b->joystick.base.x, width, VB_P(d->x), d->x_base)
		GET_VB_XY(b->joystick.base.y, height, VB_P(d->y), d->y_base)
		b->joystick.base.width = VB_P(d->r);
	b->joystick.base.height = VB_P(d->r);
	b->joystick.e_ignore_radius = VB_P(d->r) * d->eminr;
	b->joystick.e_radius = VB_P(d->r) * d->emaxr;
	float ex;
	float ey;
	GET_JOY_XY(ex, VB_P(d->x), d->emaxr, VB_P(d->r), d->x_base)
		GET_JOY_XY(ey, VB_P(d->y), d->emaxr, VB_P(d->r), d->y_base)
		GET_VB_XY(b->joystick.base.e_min_x, width, ex, d->x_base)
		GET_VB_XY(b->joystick.base.e_min_y, height, ey, d->y_base)
		b->joystick.base.e_max_x = b->joystick.base.e_min_x + VB_P(d->r) * d->emaxr;
	b->joystick.base.e_max_y = b->joystick.base.e_min_y + VB_P(d->r) * d->emaxr;

	float jw = b->joystick.base.width * d->joy_r / 2.0;
	float jh = b->joystick.base.height * d->joy_r / 2.0;
	float vertex[] = {
		b->joystick.base.x, b->joystick.base.y,
		b->joystick.base.x + b->joystick.base.width, b->joystick.base.y,
		b->joystick.base.x, b->joystick.base.y + b->joystick.base.height,
		b->joystick.base.x + b->joystick.base.width, b->joystick.base.y + b->joystick.base.height,
		// joy
		-jw, -jh,
		jw, -jh,
		-jw, jh,
		jw, jh
	};
	float texcoord[] = {
		GET_TEX_S(TEX_FULL_WIDTH, d->tx, 0), GET_TEX_T(TEX_FULL_HEIGHT, d->ty, 0),
		GET_TEX_S(TEX_FULL_WIDTH, d->tx, d->tw), GET_TEX_T(TEX_FULL_HEIGHT, d->ty, 0),
		GET_TEX_S(TEX_FULL_WIDTH, d->tx, 0), GET_TEX_T(TEX_FULL_HEIGHT, d->ty, d->tw),
		GET_TEX_S(TEX_FULL_WIDTH, d->tx, d->tw), GET_TEX_T(TEX_FULL_HEIGHT, d->ty, d->tw),

		// joy
		GET_TEX_S(TEX_FULL_WIDTH, d->joy_tx, 0), GET_TEX_T(TEX_FULL_HEIGHT, d->joy_ty, 0),
		GET_TEX_S(TEX_FULL_WIDTH, d->joy_tx, d->joy_tw), GET_TEX_T(TEX_FULL_HEIGHT, d->joy_ty, 0),
		GET_TEX_S(TEX_FULL_WIDTH, d->joy_tx, 0), GET_TEX_T(TEX_FULL_HEIGHT, d->joy_ty, d->joy_tw),
		GET_TEX_S(TEX_FULL_WIDTH, d->joy_tx, d->joy_tw), GET_TEX_T(TEX_FULL_HEIGHT, d->joy_ty, d->joy_tw),
	};
	b->joystick.base.buffers[Position_Coord] = karinNewBuffer(GL_ARRAY_BUFFER, sizeof(float) * 16, vertex, GL_STATIC_DRAW);
	b->joystick.base.buffers[Texture_Coord] = karinNewBuffer(GL_ARRAY_BUFFER, sizeof(float) * 16, texcoord, GL_STATIC_DRAW);
}

void karinMakeSwipe(virtual_control_item *b, struct vkb_swipe *d, unsigned int width, unsigned int height)
{
	if(!b || !d)
		return;
	b->swipe.base.type = vkb_swipe_type;
	b->swipe.base.pressed = bfalse;
	b->swipe.base.tex_index = 1;
	b->swipe.base.enabled = d->ava;
	b->swipe.base.visible = d->ava;
	b->swipe.base.z = d->z;
	b->swipe.ignore_distance = 1.5f;
	b->swipe.angle = -1.0f;
	b->swipe.distance = 0.0f;
	b->swipe.action = d->action;
	b->swipe.actions[0] = d->action0;
	b->swipe.actions[1] = d->action1;
	b->swipe.actions[2] = d->action2;
	b->swipe.actions[3] = d->action3;
	b->swipe.base.show_mask = d->mask;
	if(!b->swipe.base.enabled)
	{
		b->swipe.actions[0] = Total_Action;
		b->swipe.actions[1] = Total_Action;
		b->swipe.actions[2] = Total_Action;
		b->swipe.actions[3] = Total_Action;
		return;
	}

	GET_VB_XY(b->swipe.base.x, width, VB_P(d->x), d->x_base)
		GET_VB_XY(b->swipe.base.y, height, VB_P(d->y), d->y_base)
		b->swipe.base.width = VB_P(d->w);
	b->swipe.base.height = VB_P(d->h);
	GET_VB_XY(b->swipe.base.e_min_x, width, VB_P(d->ex), d->x_base)
		GET_VB_XY(b->swipe.base.e_min_y, height, VB_P(d->ey), d->y_base)
		b->swipe.base.e_max_x = b->swipe.base.e_min_x + VB_P(d->ew);
	b->swipe.base.e_max_y = b->swipe.base.e_min_y + VB_P(d->eh);
	if(d->render)
	{
		float vertex[] = {
			b->swipe.base.x, b->swipe.base.y,
			b->swipe.base.x + b->swipe.base.width, b->swipe.base.y,
			b->swipe.base.x, b->swipe.base.y + b->swipe.base.height,
			b->swipe.base.x + b->swipe.base.width, b->swipe.base.y + b->swipe.base.height
		};
		float texcoord[] = {
			GET_TEX_S(TEX_FULL_WIDTH, d->tx, 0), GET_TEX_T(TEX_FULL_HEIGHT, d->ty, 0),
			GET_TEX_S(TEX_FULL_WIDTH, d->tx, d->tw), GET_TEX_T(TEX_FULL_HEIGHT, d->ty, 0),
			GET_TEX_S(TEX_FULL_WIDTH, d->tx, 0), GET_TEX_T(TEX_FULL_HEIGHT, d->ty, d->tw),
			GET_TEX_S(TEX_FULL_WIDTH, d->tx, d->tw), GET_TEX_T(TEX_FULL_HEIGHT, d->ty, d->tw),
		};
		b->swipe.base.buffers[Position_Coord] = karinNewBuffer(GL_ARRAY_BUFFER, sizeof(float) * 8, vertex, GL_STATIC_DRAW);
		b->swipe.base.buffers[Texture_Coord] = karinNewBuffer(GL_ARRAY_BUFFER, sizeof(float) * 8, texcoord, GL_STATIC_DRAW);
	}
}

void karinMakeButton(virtual_control_item *b, struct vkb_button *d, unsigned int width, unsigned int height)
{
	if(!b || !d)
		return;
	b->button.base.type = vkb_button_type;
	b->button.base.pressed = bfalse;
	b->button.base.enabled = d->ava;
	b->button.base.visible = d->ava;
	b->button.base.z = d->z;
	b->button.action = d->action;
	b->button.base.show_mask = d->mask;
	b->button.base.tex_index = d->tex_index;
	if(!b->button.base.enabled)
	{
		b->button.action = Total_Action;
		return;
	}

	GET_VB_XY(b->button.base.x, width, VB_P(d->x), d->x_base)
		GET_VB_XY(b->button.base.y, height, VB_P(d->y), d->y_base)
		b->button.base.width = VB_P(d->w);
	b->button.base.height = VB_P(d->h);
	GET_VB_XY(b->button.base.e_min_x, width, VB_P(d->ex), d->x_base)
		GET_VB_XY(b->button.base.e_min_y, height, VB_P(d->ey), d->y_base)
		b->button.base.e_max_x = b->button.base.e_min_x + VB_P(d->ew);
	b->button.base.e_max_y = b->button.base.e_min_y + VB_P(d->eh);
	float vertex[] = {
		b->button.base.x, b->button.base.y,
		b->button.base.x + b->button.base.width, b->button.base.y,
		b->button.base.x, b->button.base.y + b->button.base.height,
		b->button.base.x + b->button.base.width, b->button.base.y + b->button.base.height
	};
	float texcoord[] = {
		GET_TEX_S(TEX_FULL_WIDTH, d->tx, 0), GET_TEX_T(TEX_FULL_HEIGHT, d->ty, 0),
		GET_TEX_S(TEX_FULL_WIDTH, d->tx, d->tw), GET_TEX_T(TEX_FULL_HEIGHT, d->ty, 0),
		GET_TEX_S(TEX_FULL_WIDTH, d->tx, 0), GET_TEX_T(TEX_FULL_HEIGHT, d->ty, d->th),
		GET_TEX_S(TEX_FULL_WIDTH, d->tx, d->tw), GET_TEX_T(TEX_FULL_HEIGHT, d->ty, d->th),

		GET_TEX_S(TEX_FULL_WIDTH, d->ptx, 0), GET_TEX_T(TEX_FULL_HEIGHT, d->pty, 0),
		GET_TEX_S(TEX_FULL_WIDTH, d->ptx, d->ptw), GET_TEX_T(TEX_FULL_HEIGHT, d->pty, 0),
		GET_TEX_S(TEX_FULL_WIDTH, d->ptx, 0), GET_TEX_T(TEX_FULL_HEIGHT, d->pty, d->pth),
		GET_TEX_S(TEX_FULL_WIDTH, d->ptx, d->ptw), GET_TEX_T(TEX_FULL_HEIGHT, d->pty, d->pth)
	};
	b->button.base.buffers[Position_Coord] = karinNewBuffer(GL_ARRAY_BUFFER, sizeof(float) * 8, vertex, GL_STATIC_DRAW);
	b->button.base.buffers[Texture_Coord] = karinNewBuffer(GL_ARRAY_BUFFER, sizeof(float) * 16, texcoord, GL_STATIC_DRAW);
}
#undef GET_VB_XY
#undef GET_TEX_S
#undef GET_TEX_T

void karinRenderVKBButton(const virtual_button *b, const texture tex[])
{
	if(!b)
		return;
	if(b->base.type != vkb_button_type)
		return;
	if(!qglIsTexture(tex[b->base.tex_index].imaged))
		return;
	if(!b->base.visible)
		return;
	if(!qglIsBuffer(b->base.buffers[Texture_Coord]) || !qglIsBuffer(b->base.buffers[Position_Coord]))
		return;
	qglBindTexture(GL_TEXTURE_2D, tex[b->base.tex_index].imaged);
	qglBindBuffer(GL_ARRAY_BUFFER, b->base.buffers[Texture_Coord]);
	GLvoid *ptr = (b->base.pressed && b->base.enabled) ? (float *)NULL + 8 : NULL;
	qglTexCoordPointer(2, GL_FLOAT, 0, ptr);
	qglBindBuffer(GL_ARRAY_BUFFER, b->base.buffers[Position_Coord]);
	qglVertexPointer(2, GL_FLOAT, 0, NULL);
	qglDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	qglBindBuffer(GL_ARRAY_BUFFER, 0);
	qglBindTexture(GL_TEXTURE_2D, 0);
}

void karinRenderVKBSwipe(const virtual_swipe *b, const texture tex[])
{
	if(!b)
		return;
	if(b->base.type != vkb_swipe_type)
		return;
	if(!qglIsTexture(tex[b->base.tex_index].imaged))
		return;
	if(!b->base.visible)
		return;
	if(!qglIsBuffer(b->base.buffers[Texture_Coord]) || !qglIsBuffer(b->base.buffers[Position_Coord]))
		return;
	qglBindTexture(GL_TEXTURE_2D, tex[b->base.tex_index].imaged);
	qglBindBuffer(GL_ARRAY_BUFFER, b->base.buffers[Texture_Coord]);
	qglTexCoordPointer(2, GL_FLOAT, 0, NULL);
	qglBindBuffer(GL_ARRAY_BUFFER, b->base.buffers[Position_Coord]);
	qglVertexPointer(2, GL_FLOAT, 0, NULL);
	qglDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	qglBindBuffer(GL_ARRAY_BUFFER, 0);
	qglBindTexture(GL_TEXTURE_2D, 0);
}

void karinRenderVKBJoystick(const virtual_joystick *b, const texture tex[])
{
	if(!b)
		return;
	if(b->base.type != vkb_joystick_type)
		return;
	if(!qglIsTexture(tex[b->base.tex_index].imaged))
		return;
	if(!b->base.visible)
		return;
	if(!qglIsBuffer(b->base.buffers[Texture_Coord]) || !qglIsBuffer(b->base.buffers[Position_Coord]))
		return;
	qglBindTexture(GL_TEXTURE_2D, tex[b->base.tex_index].imaged);
	qglBindBuffer(GL_ARRAY_BUFFER, b->base.buffers[Texture_Coord]);
	qglTexCoordPointer(2, GL_FLOAT, 0, NULL);
	qglBindBuffer(GL_ARRAY_BUFFER, b->base.buffers[Position_Coord]);
	qglVertexPointer(2, GL_FLOAT, 0, NULL);
	qglDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	float cx = b->base.x + b->base.width / 2;
	float cy = b->base.y + b->base.height / 2;
	qglPushMatrix();
	{
		qglTranslatef(cx + b->pos_x, cy + b->pos_y, 0.0f);
		qglDrawArrays(GL_TRIANGLE_STRIP, 4, 4);
	}
	qglPopMatrix();

	qglBindBuffer(GL_ARRAY_BUFFER, 0);
	qglBindTexture(GL_TEXTURE_2D, 0);
}

void karinRenderVKBCursor(const virtual_cursor *b, const texture tex[])
{
	if(!b)
		return;
	if(b->base.type != vkb_cursor_type)
		return;
	if(!qglIsTexture(tex[b->base.tex_index].imaged))
		return;
	if(!b->base.visible)
		return;
	if(!qglIsBuffer(b->base.buffers[Texture_Coord]) || !qglIsBuffer(b->base.buffers[Position_Coord]))
		return;
	qglBindTexture(GL_TEXTURE_2D, tex[b->base.tex_index].imaged);
	if(b->render_bg)
	{
		qglBindBuffer(GL_ARRAY_BUFFER, b->base.buffers[Texture_Coord]);
		qglTexCoordPointer(2, GL_FLOAT, 0, (float *)NULL + 16);
		qglBindBuffer(GL_ARRAY_BUFFER, b->base.buffers[Position_Coord]);
		qglVertexPointer(2, GL_FLOAT, 0, (float *)NULL + 8);

		qglDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}

	qglBindBuffer(GL_ARRAY_BUFFER, b->base.buffers[Texture_Coord]);
	GLvoid *ptr = (b->base.pressed && b->base.enabled) ? (float *)NULL + 8 : NULL;
	qglTexCoordPointer(2, GL_FLOAT, 0, ptr);
	qglBindBuffer(GL_ARRAY_BUFFER, b->base.buffers[Position_Coord]);
	qglVertexPointer(2, GL_FLOAT, 0, NULL);

	float cx = b->base.x + b->base.width / 2;
	float cy = b->base.y + b->base.height / 2;
	qglPushMatrix();
	{
		qglTranslatef(cx + b->pos_x, cy + b->pos_y, 0.0f);
		qglDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}
	qglPopMatrix();

	qglBindBuffer(GL_ARRAY_BUFFER, 0);
	qglBindTexture(GL_TEXTURE_2D, 0);
}

void karinResizeVKB(float w, float h)
{
	if(!the_vkb.inited)
		return;
	if(the_vkb.width == w && the_vkb.height == h)
		return;

	the_vkb.width = w;
	the_vkb.height = h;

	karinUpdateP(w, h);
	
	int count = TOTAL_VKB_COUNT;
	int i;
	for(i = 0; i < count; i++)
	{
		int j;
		for(j = Position_Coord; j < Total_Coord; j++)
		{
			if(qglIsBuffer(the_vkb.vb[i].base.buffers[j]))
				qglDeleteBuffers(1, the_vkb.vb[i].base.buffers + j);
			the_vkb.vb[i].base.buffers[j] = 0;
		}
	}

	int j = 0;

	// btn
	for(i = 0; i < VKB_COUNT; i++)
	{
		karinMakeButton(the_vkb.vb + j, VKB_Button + i, the_vkb.width, the_vkb.height);
		j++;
	}

	// joy
	for(i = 0; i < JOYSTICK_COUNT; i++)
	{
		karinMakeJoystick(the_vkb.vb + j, VKB_Joystick + i, the_vkb.width, the_vkb.height);
		j++;
	}

	// cursor
	for(i = 0; i < CURSOR_COUNT; i++)
	{
		karinMakeCursor(the_vkb.vb + j, VKB_Cursor + i, the_vkb.width, the_vkb.height);
		j++;
	}

	// swipe
	for(i = 0; i < SWIPE_COUNT; i++)
	{
		karinMakeSwipe(the_vkb.vb + j, VKB_Swipe + i, the_vkb.width, the_vkb.height);
		j++;
	}

	the_vkb.inited = btrue;
}
