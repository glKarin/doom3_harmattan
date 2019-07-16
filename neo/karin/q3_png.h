#ifndef _KARIN_Q3_PNG_H
#define _KARIN_Q3_PNG_H

#ifdef __cplusplus
extern "C" {
#endif

/* ioquake3 code/renderer/tr_image_png.c */
void karinLoadPNG(const char *name, unsigned char **pic, int *width, int *height);

#ifdef __cplusplus
}
#endif

#endif
