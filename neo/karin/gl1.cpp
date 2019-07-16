#include "../renderer/qgl.h"
#include "gl1ext.h"
#include "gl1arb.h"

#include <stdio.h>
#include <string.h>
//#include "../idlib/precompiled.h"

/* for port */
#define KARIN_QGL_UNSUPPORT(func) fprintf(stderr, "gl%s is not support on MeeGo Harmattan OpenGL ES1.1.\n", func)

void qglGetCompressedTexImage (GLenum target, GLint level, GLvoid *img)
{
	KARIN_QGL_UNSUPPORT(__func__);
}

void qglGetBufferSubData (GLenum target, GLintptrARB offset, GLsizeiptrARB size, GLvoid *data)
{
	KARIN_QGL_UNSUPPORT(__func__);
}

void qglMultiTexCoord2f(GLenum target, GLfloat s, GLfloat t)
{
	qglMultiTexCoord4f(target, s, t, 0.0, 1.0);
}

void qglMultiTexCoord2fv(GLenum target, const GLfloat *v)
{
	if(v)
		qglMultiTexCoord4f(target, v[0], v[1], 0.0, 1.0);
}

void qglColor3f(GLfloat r, GLfloat g, GLfloat b)
{
	qglColor4f(r, g, b, 1.0);
}

void qglColor3ub(GLubyte r, GLubyte g, GLubyte b)
{
	qglColor4ub(r, g, b, 255);
}

void qglColor3fv(const GLfloat *v)
{
	if(v)
		qglColor4f(v[0], v[1], v[2], 1.0);
}

void qglColor4fv(const GLfloat *v)
{
	if(v)
		qglColor4f(v[0], v[1], v[2], v[3]);
}

void qglColor3ubv(const GLubyte *v)
{
	if(v)
		qglColor4ub(v[0], v[1], v[2], 255);
}

void qglColor4ubv(const GLubyte *v)
{
	if(v)
		qglColor4ub(v[0], v[1], v[2], v[3]);
}

enum {
	KARIN_ARRAY_BUFFER_BINDING,
	KARIN_ELEMENT_ARRAY_BUFFER_BINDING,
	KARIN_BUFFER_BINDING_COUNT,
};
enum {
	KARIN_ARRAY_BINDING,
	KARIN_ARRAY_POINTER,
	KARIN_ARRAY_STRIDE,
	KARIN_ARRAY_TYPE,
	KARIN_ARRAY_SIZE,
	KARIN_ARRAY_COUNT,
};
#define KARIN_MAX_CLIENT_MASK_STACK_DEPTH 16
typedef struct _karin_ClientState {
	unsigned state;
	GLint buffers[KARIN_BUFFER_BINDING_COUNT];
	GLint arrays[4][KARIN_ARRAY_COUNT];
	GLint texture_unit;
} karin_ClientState;

typedef struct _karin_ClientStateStack
{
	unsigned int pointer;
	karin_ClientState stack[KARIN_MAX_CLIENT_MASK_STACK_DEPTH];
} karin_ClientStateStack;

#define STACK_CUR(s) ((s).stack + (s).pointer)

static karin_ClientStateStack csm_stack;

static const GLenum Arrays[4][5] = {
	{ GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING, GL_TEXTURE_COORD_ARRAY_POINTER, GL_TEXTURE_COORD_ARRAY_STRIDE, GL_TEXTURE_COORD_ARRAY_TYPE, GL_TEXTURE_COORD_ARRAY_SIZE }, 
	{ GL_COLOR_ARRAY_BUFFER_BINDING, GL_COLOR_ARRAY_POINTER, GL_COLOR_ARRAY_STRIDE, GL_COLOR_ARRAY_TYPE, GL_COLOR_ARRAY_SIZE }, 
	{ GL_NORMAL_ARRAY_BUFFER_BINDING, GL_NORMAL_ARRAY_POINTER, GL_NORMAL_ARRAY_STRIDE, GL_NORMAL_ARRAY_TYPE, 0 }, 
	{ GL_VERTEX_ARRAY_BUFFER_BINDING, GL_VERTEX_ARRAY_POINTER, GL_VERTEX_ARRAY_STRIDE, GL_VERTEX_ARRAY_TYPE, GL_VERTEX_ARRAY_SIZE }, 
};
static const GLenum States[] = {
	GL_TEXTURE_COORD_ARRAY,
	GL_COLOR_ARRAY,
	GL_NORMAL_ARRAY,
	GL_VERTEX_ARRAY,
};

unsigned karinClientState(int set)
{
	static const unsigned int Count = 3;

	int i;
	unsigned m;
	unsigned mask = 0;
	if(set < 0)
	{
		for(i = 0; i < Count; i++)
		{
			m = qglIsEnabled(States[i]);
			if(m)
			{
				m <<= i;
				mask |= m;
			}
		}
		return mask;
	}
	else
	{
		for(i = 0; i < Count; i++)
		{
			m = 1;
			m <<= i;
			if(set & m)
			{
				qglEnableClientState(States[i]);
			}
			else
			{
				qglDisableClientState(States[i]);
			}
		}
		return set;
	}
}

void karinPushClientState()
{
	if(csm_stack.pointer >= KARIN_MAX_CLIENT_MASK_STACK_DEPTH)
	{
		fprintf(stderr, "Client state stack is overflow.\n");
		return;
	}
	csm_stack.pointer++;

	karin_ClientState *cs = STACK_CUR(csm_stack);
	int i;
	int j;
	GLenum e;
	unsigned mask = karinClientState(-1);
	cs->state = mask;
	qglGetIntegerv(GL_ARRAY_BUFFER_BINDING, (GLint *)(cs->buffers + KARIN_ARRAY_BUFFER_BINDING));
	qglGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, (GLint *)(cs->buffers + KARIN_ELEMENT_ARRAY_BUFFER_BINDING));
	for(i = 0; i < 4; i++)
	{
		for(j = 0; j < KARIN_ARRAY_COUNT; j++)
		{
			e = Arrays[i][j];
			if(e == 0)
				continue;
			if(j == 1)
				qglGetPointerv(e, reinterpret_cast<void **>(cs->arrays[i] + j));
			else
				qglGetIntegerv(e, cs->arrays[i] + j);
		}
	}
	qglGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &cs->texture_unit);
	qglClientActiveTexture(GL_TEXTURE0);
	qglBindBuffer(GL_ARRAY_BUFFER, 0);
	qglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	//printf("[push %d]: %d %d\n", csm_stack.pointer, csm_stack.stack[csm_stack.pointer].buffers[KARIN_ARRAY_BUFFER_BINDING], csm_stack.stack[csm_stack.pointer].buffers[KARIN_ELEMENT_ARRAY_BUFFER_BINDING]);
}

void karinPopClientState()
{
	if(csm_stack.pointer == 0)
	{
		fprintf(stderr, "Client state stack is underflow.\n");
		return;
	}

	karin_ClientState *cs = STACK_CUR(csm_stack);
	int i;
	GLint *p;

	karinClientState(cs->state);
	for(i = 0; i < 4; i++)
	{
		p = cs->arrays[i];
		if(p[0] != 0)
			qglBindBuffer(GL_ARRAY_BUFFER, p[0]);
		switch(i)
		{
			case 0:
				qglTexCoordPointer(p[4], p[3], p[2], reinterpret_cast<void *>(p[1]));
				break;
			case 1:
				qglColorPointer(p[4], p[3], p[2], reinterpret_cast<void *>(p[1]));
				break;
			case 2:
				qglNormalPointer(p[3], p[2], reinterpret_cast<void *>(p[1]));
				break;
			case 3:
				qglVertexPointer(p[4], p[3], p[2], reinterpret_cast<void *>(p[1]));
				break;
			default:
				break;
		}

		if(p[0] != 0)
			qglBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	qglClientActiveTexture(cs->texture_unit);
	qglBindBuffer(GL_ARRAY_BUFFER, cs->buffers[KARIN_ARRAY_BUFFER_BINDING]);
	qglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cs->buffers[KARIN_ELEMENT_ARRAY_BUFFER_BINDING]);
	//printf("[pop %d]: %d %d\n", csm_stack.pointer, csm_stack.stack[csm_stack.pointer].buffers[KARIN_ARRAY_BUFFER_BINDING], csm_stack.stack[csm_stack.pointer].buffers[KARIN_ELEMENT_ARRAY_BUFFER_BINDING]);
	
	memset(cs, 0, sizeof(karin_ClientState));
	cs->state = -1;

	csm_stack.pointer--;
}
