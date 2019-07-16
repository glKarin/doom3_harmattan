#include "harm.h"

float vb_p = 1.0f;
int scr_width = N950_W;
int scr_height = N950_H;

void karinSetScrGeometry(int w, int h)
{
	scr_width = w > 0 ? w : N950_W;
	scr_height = h > 0 ? h : N950_H;
}

void karinUpdateP(int w, int h)
{
	float wp;
	float hp;

	wp = hp = 1.0f;

	if(w > 0)
		wp = (float)w / (float)N950_W;
	if(h > 0)
		hp = (float)h / (float)N950_H;

	vb_p = wp > hp ? hp : wp;
}

