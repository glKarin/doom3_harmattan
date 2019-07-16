#ifndef _KARIN_HARM_H
#define _KARIN_HARM_H

#define N950_W 854
#define N950_H 480
#define N950_H_NOT_FULL 376
#define N900_W 800

#define GL_W 640
#define GL_H 480

extern int scr_width;
extern int scr_height;
extern float vb_p;

void karinUpdateP(int w, int h);
void karinSetScrGeometry(int w, int h);

#endif
