#ifndef _KARIN_GL2_ABI_H
#define _KARIN_GL2_ABI_H

#include <GLES2/gl2.h>

extern GLfloat projection_matrix[16];
extern GLfloat modelview_matrix[16];
extern GLfloat mvp_matrix[16];

void Mesa_matmul4(const float a[16], const float b[16], float product[16]);
void Mesa_glLoadIdentity(float m[16]);
void Mesa_glRotate(const float mat[16], float angle, float x, float y, float z, float r[16]);
void Mesa_glTranslate(const float mat[16], float x, float y, float z, float m[16]);
void Mesa_glOrtho(const float mat[16], float left, float right, float bottom, float top, float nearval, float farval, float r[16]);



void karinSetupMatrix();
void karinSetupTexture(GLuint t);
void karinSetupAlpha(GLclampf f);
void karinSetupVertex(const GLfloat *ptr);
void karinSetupTexCoord(const GLfloat *ptr);
void karinBeginRender2D(int w, int h);
void karinEndRender2D();
void karinInit();
void karinDestroy();

#endif
