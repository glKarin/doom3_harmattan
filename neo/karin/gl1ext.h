#ifndef _KARIN_GL1_EXT_H
#define _KARIN_GL1_EXT_H

#ifdef _HARMATTAN

#include <GLES/glext.h>
#ifndef APIENTRYP
#define APIENTRYP APIENTRY *
#endif

#include "gl1arb.h"

#include <stddef.h>

typedef ptrdiff_t GLsizeiptrARB;
typedef ptrdiff_t GLintptrARB;

// compressed texture
typedef void (APIENTRYP PFNGLCOMPRESSEDTEXIMAGE2DARBPROC) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data);
typedef void (APIENTRYP PFNGLGETCOMPRESSEDTEXIMAGEARBPROC) (GLenum target, GLint level, GLvoid *img);

// buffer
typedef void (APIENTRYP PFNGLBINDBUFFERARBPROC) (GLenum target, GLuint buffer);
typedef void (APIENTRYP PFNGLDELETEBUFFERSARBPROC) (GLsizei n, const GLuint *buffers);
typedef void (APIENTRYP PFNGLGENBUFFERSARBPROC) (GLsizei n, GLuint *buffers);
typedef GLboolean (APIENTRYP PFNGLISBUFFERARBPROC) (GLuint buffer);
typedef void (APIENTRYP PFNGLBUFFERDATAARBPROC) (GLenum target, GLsizeiptrARB size, const GLvoid *data, GLenum usage);
typedef void (APIENTRYP PFNGLBUFFERSUBDATAARBPROC) (GLenum target, GLintptrARB offset, GLsizeiptrARB size, const GLvoid *data);
typedef void (APIENTRYP PFNGLGETBUFFERSUBDATAARBPROC) (GLenum target, GLintptrARB offset, GLsizeiptrARB size, GLvoid *data);
typedef GLvoid* (APIENTRYP PFNGLMAPBUFFERARBPROC) (GLenum target, GLenum access);
typedef GLboolean (APIENTRYP PFNGLUNMAPBUFFERARBPROC) (GLenum target);
typedef void (APIENTRYP PFNGLGETBUFFERPARAMETERIVARBPROC) (GLenum target, GLenum pname, GLint *params);
typedef void (APIENTRYP PFNGLGETBUFFERPOINTERVARBPROC) (GLenum target, GLenum pname, GLvoid* *params);

#endif

void qglGetCompressedTexImage (GLenum target, GLint level, GLvoid *img);
void qglGetBufferSubData (GLenum target, GLintptrARB offset, GLsizeiptrARB size, GLvoid *data);
void qglMultiTexCoord2f(GLenum target, GLfloat s, GLfloat t);
void qglMultiTexCoord2fv(GLenum target, const GLfloat *v);
void qglColor3f(GLfloat r, GLfloat g, GLfloat b);
void qglColor3ub(GLubyte r, GLubyte g, GLubyte b);
void qglColor3fv(const GLfloat *v);
void qglColor4fv(const GLfloat *v);
void qglColor3ubv(const GLubyte *v);
void qglColor4ubv(const GLubyte *v);

//#include "gl1_arb.h"

#endif
