#ifndef _KARIN_GL1_ARB_H
#define _KARIN_GL1_ARB_H

typedef GLfloat GLdouble;
typedef GLclampf GLclampd;

// compressed texture
// #define qglCompressedTexImage2DARB qglCompressedTexImage2D
// #define qglCompressedTexSubImage2DARB qglCompressedSubTexImage2D

// buffer

#define GL_ARRAY_BUFFER_ARB GL_ARRAY_BUFFER
#define GL_ELEMENT_ARRAY_BUFFER_ARB GL_ELEMENT_ARRAY_BUFFER

#define GL_ARRAY_BUFFER_BINDING_ARB GL_ARRAY_BUFFER_BINDING
#define GL_ELEMENT_ARRAY_BUFFER_BINDING_ARB GL_ELEMENT_ARRAY_BUFFER_BINDING

#define GL_STATIC_DRAW_ARB GL_STATIC_DRAW
#define GL_DYNAMIC_DRAW_ARB GL_DYNAMIC_DRAW

#define GL_BUFFER_SIZE_ARB GL_BUFFER_SIZE
#define GL_BUFFER_USAGE_ARB GL_BUFFER_USAGE

/*
#define qglBindBufferARB qglBindBuffer
#define qglBufferDataARB qglBufferData
#define qglBufferSubDataARB qglBufferSubData
#define qglDeleteBuffersARB qglDeleteBuffers
#define qglGetBufferParameterivARB qglGetBufferParameteriv
#define qglGenBuffersARB qglGenBuffers
#define qglIsBufferARB qglIsBuffer
*/

// texenv
#define GL_SUBTRACT_ARB GL_SUBTRACT
#define GL_COMBINE_ARB GL_COMBINE
#define GL_COMBINE_RGB_ARB GL_COMBINE_RGB
#define GL_COMBINE_ALPHA_ARB GL_COMBINE_ALPHA
#define GL_RGB_SCALE_ARB GL_RGB_SCALE
#define GL_ADD_SIGNED_ARB GL_ADD_SIGNED
#define GL_INTERPOLATE_ARB GL_INTERPOLATE
#define GL_CONSTANT_ARB GL_CONSTANT
#define GL_PRIMARY_COLOR_ARB GL_PRIMARY_COLOR
#define GL_PREVIOUS_ARB GL_PREVIOUS
#define GL_OPERAND0_RGB_ARB GL_OPERAND0_RGB
#define GL_OPERAND1_RGB_ARB GL_OPERAND1_RGB
#define GL_OPERAND2_RGB_ARB GL_OPERAND2_RGB
#define GL_OPERAND0_ALPHA_ARB GL_OPERAND0_ALPHA
#define GL_OPERAND1_ALPHA_ARB GL_OPERAND1_ALPHA
#define GL_OPERAND2_ALPHA_ARB GL_OPERAND2_ALPHA

#define GL_ALPHA_SCALE_ARB GL_ALPHA_SCALE

#define GL_SOURCE0_RGB_ARB GL_SRC0_RGB
#define GL_SOURCE1_RGB_ARB GL_SRC1_RGB
#define GL_SOURCE2_RGB_ARB GL_SRC2_RGB
#define GL_SOURCE0_ALPHA_ARB GL_SRC0_ALPHA
#define GL_SOURCE1_ALPHA_ARB GL_SRC1_ALPHA
#define GL_SOURCE2_ALPHA_ARB GL_SRC2_ALPHA

#define GL_DOT3_RGB_ARB GL_DOT3_RGB
#define GL_DOT3_RGBA_ARB GL_DOT3_RGBA

// multi texture
#define GL_TEXTURE0_ARB GL_TEXTURE0
/*
#define qglActiveTextureARB qglActiveTexture
#define qglClientActiveTextureARB qglClientActiveTexture
*/

// IMG vertex program
typedef void (APIENTRYP PFNGLVERTEXATTRIBPOINTERARBPROC) (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer);
typedef void (APIENTRYP PFNGLENABLEVERTEXATTRIBARRAYARBPROC) (GLuint index);
typedef void (APIENTRYP PFNGLDISABLEVERTEXATTRIBARRAYARBPROC) (GLuint index);
typedef void (APIENTRYP PFNGLPROGRAMSTRINGARBPROC) (GLenum target, GLenum format, GLsizei len, const GLvoid *string);
typedef void (APIENTRYP PFNGLBINDPROGRAMARBPROC) (GLenum target, GLuint program);
typedef void (APIENTRYP PFNGLDELETEPROGRAMSARBPROC) (GLsizei n, const GLuint *programs);
typedef void (APIENTRYP PFNGLGENPROGRAMSARBPROC) (GLsizei n, GLuint *programs);
typedef void (APIENTRYP PFNGLPROGRAMENVPARAMETER4FARBPROC) (GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
typedef void (APIENTRYP PFNGLPROGRAMENVPARAMETER4FVARBPROC) (GLenum target, GLuint index, const GLfloat *params);
typedef void (APIENTRYP PFNGLPROGRAMLOCALPARAMETER4FVARBPROC) (GLenum target, GLuint index, const GLfloat *params);

// cube map
#define GL_NORMAL_MAP_EXT GL_NORMAL_MAP_OES
#define GL_REFLECTION_MAP_EXT GL_REFLECTION_MAP_OES
#define GL_TEXTURE_CUBE_MAP_EXT GL_TEXTURE_CUBE_MAP_OES
#define GL_TEXTURE_BINDING_CUBE_MAP_EXT GL_TEXTURE_BINDING_CUBE_MAP_OES
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X_EXT GL_TEXTURE_CUBE_MAP_POSITIVE_X_OES
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X_EXT GL_TEXTURE_CUBE_MAP_NEGATIVE_X_OES
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y_EXT GL_TEXTURE_CUBE_MAP_POSITIVE_Y_OES
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_EXT GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_OES
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z_EXT GL_TEXTURE_CUBE_MAP_POSITIVE_Z_OES
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_EXT GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_OES
#define GL_MAX_CUBE_MAP_TEXTURE_SIZE_EXT GL_MAX_CUBE_MAP_TEXTURE_SIZE_OES
#define GL_TEXTURE_GEN_MODE_ARB GL_TEXTURE_GEN_MODE_OES
#define GL_TEXTURE_GEN_STR_ARB GL_TEXTURE_GEN_STR_OES

// std OpenGL
#ifndef qglOrtho
#define qglOrtho qglOrthof
#endif
#ifndef qglDepthRange
#define qglDepthRange qglDepthRangef
#endif
#ifndef qglClearDepth
#define qglClearDepth qglClearDepthf
#endif

// my function
#define R_BEGIN_ARRAYS karinPushClientState();
#define R_END_ARRAYS karinPopClientState();
// control client state
// if(set < 0) get client state
// else set client state by `set'
unsigned karinClientState(int set);
void karinPushClientState();
void karinPopClientState();

#endif
